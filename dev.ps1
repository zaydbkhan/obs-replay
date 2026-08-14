param(
    [Parameter(Position = 0)]
    [ValidateSet("build", "install", "run")]
    [string] $Action = "run",

    [ValidateSet("Debug", "RelWithDebInfo", "Release")]
    [string] $Configuration = "RelWithDebInfo",

    [string] $ObsPath,

    [switch] $Reconfigure,

    [Parameter(DontShow)]
    [switch] $InstallOnly
)

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"

function Invoke-Native {
    param(
        [Parameter(Mandatory)]
        [string] $FilePath,

        [Parameter(Mandatory)]
        [string[]] $Arguments
    )

    Write-Host "> $FilePath $($Arguments -join ' ')" -ForegroundColor DarkGray
    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "Command failed with exit code $LASTEXITCODE`: $FilePath"
    }
}

function Find-VisualStudio {
    $vswhere = Join-Path ([Environment]::GetFolderPath("ProgramFilesX86")) `
        "Microsoft Visual Studio\Installer\vswhere.exe"
    if (-not (Test-Path -LiteralPath $vswhere)) {
        throw "Visual Studio Installer (vswhere.exe) was not found. Install Visual Studio with Desktop development with C++."
    }

    $json = & $vswhere -latest -products * `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -format json -utf8
    if ($LASTEXITCODE -ne 0) {
        throw "Visual Studio detection failed."
    }

    $instances = $json | ConvertFrom-Json
    if (-not $instances -or -not $instances[0]) {
        throw "No Visual Studio installation with the C++ build tools was found."
    }

    $instance = $instances[0]
    $major = ([version] $instance.installationVersion).Major
    $generator = switch ($major) {
        18 { "Visual Studio 18 2026" }
        17 { "Visual Studio 17 2022" }
        default { throw "Visual Studio $major is not supported by this development script." }
    }

    return [PSCustomObject]@{
        Generator = $generator
        Major = $major
        Path = $instance.installationPath
    }
}

function Find-Obs {
    param([string] $RequestedPath)

    if ($RequestedPath) {
        $resolved = Resolve-Path -LiteralPath $RequestedPath -ErrorAction SilentlyContinue
        if (-not $resolved) {
            throw "OBS was not found at '$RequestedPath'."
        }
        return $resolved.Path
    }

    $candidates = @(
        (Join-Path $env:ProgramFiles "obs-studio\bin\64bit\obs64.exe"),
        (Join-Path $env:ProgramFiles "OBS Studio\bin\64bit\obs64.exe")
    )
    foreach ($candidate in $candidates) {
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }

    $command = Get-Command obs64.exe -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    throw "OBS Studio was not found. Pass its executable with -ObsPath."
}

function Test-Administrator {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    $principal = [Security.Principal.WindowsPrincipal]::new($identity)
    return $principal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

# Some hosted shells expose both Path and PATH. MSBuild treats those as duplicate
# environment variables, so normalize them before invoking the compiler.
$currentPath = $env:Path
[Environment]::SetEnvironmentVariable("PATH", $null, "Process")
[Environment]::SetEnvironmentVariable("Path", $currentPath, "Process")

$repoRoot = $PSScriptRoot
$cmake = (Get-Command cmake.exe -ErrorAction SilentlyContinue).Source
if (-not $cmake) {
    throw "CMake was not found on PATH. Install CMake 3.28 or newer."
}

$visualStudio = Find-VisualStudio
$buildDirectory = Join-Path $repoRoot "build_dev_vs$($visualStudio.Major)"
$cmakeCache = Join-Path $buildDirectory "CMakeCache.txt"

if (-not $env:ProgramData) {
    throw "ProgramData is not defined, so the OBS plugin directory cannot be located."
}
$pluginRoot = Join-Path $env:ProgramData "obs-studio\plugins"

function Install-Plugin {
    Write-Host "Installing into $pluginRoot..." -ForegroundColor Cyan
    Invoke-Native $cmake @(
        "--install", $buildDirectory,
        "--config", $Configuration,
        "--prefix", $pluginRoot
    )
    Write-Host "Installed: $(Join-Path $pluginRoot 'replay-source')" -ForegroundColor Green
}

if ($InstallOnly) {
    if (-not (Test-Path -LiteralPath $cmakeCache)) {
        throw "The build directory is not configured. Run '.\dev.cmd build' first."
    }
    Install-Plugin
    exit 0
}

if ($Action -ne "build" -and (Get-Process obs64 -ErrorAction SilentlyContinue)) {
    throw "OBS is running. Close OBS before installing the rebuilt plugin."
}

if ($Reconfigure -or -not (Test-Path -LiteralPath $cmakeCache)) {
    Write-Host "Configuring Replay Source with $($visualStudio.Generator)..." -ForegroundColor Cyan
    Invoke-Native $cmake @(
        "-S", $repoRoot,
        "-B", $buildDirectory,
        "-G", $visualStudio.Generator,
        "-A", "x64",
        "-DCMAKE_GENERATOR_INSTANCE=$($visualStudio.Path)",
        "-DENABLE_FRONTEND_API=ON",
        "-DENABLE_QT=ON"
    )
} else {
    Write-Host "Reusing configured build directory: $buildDirectory" -ForegroundColor DarkGray
}

Write-Host "Building Replay Source ($Configuration)..." -ForegroundColor Cyan
Invoke-Native $cmake @(
    "--build", $buildDirectory,
    "--config", $Configuration,
    "--target", "replay-source",
    "--parallel"
)

$artifact = Join-Path $buildDirectory "$Configuration\replay-source.dll"
Write-Host "Built: $artifact" -ForegroundColor Green

if ($Action -eq "build") {
    exit 0
}

if (Test-Administrator) {
    Install-Plugin
} else {
    Write-Host "OBS loads Windows plugins from ProgramData; requesting permission to install..." -ForegroundColor Cyan
    $scriptArgument = '"' + $PSCommandPath + '"'
    $elevated = Start-Process powershell.exe -Verb RunAs -Wait -PassThru -ArgumentList @(
        "-NoLogo",
        "-NoProfile",
        "-ExecutionPolicy", "Bypass",
        "-File", $scriptArgument,
        "install",
        "-Configuration", $Configuration,
        "-InstallOnly"
    )
    if ($elevated.ExitCode -ne 0) {
        throw "Plugin installation failed with exit code $($elevated.ExitCode)."
    }
}

if ($Action -eq "install") {
    exit 0
}

$resolvedObsPath = Find-Obs $ObsPath
Write-Host "Launching OBS Studio..." -ForegroundColor Cyan
Start-Process -FilePath $resolvedObsPath -WorkingDirectory (Split-Path $resolvedObsPath)
