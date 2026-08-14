# Replay Source for OBS Studio

Replay Source captures recent audio and video from an OBS source in memory and plays it back at normal speed, in slow motion, or in reverse. This fork adds a native Replay Controls dock to the original Replay Source plugin.

## Replay Controls dock

After adding at least one **Replay Source** in OBS, open **Docks → Replay Controls**. The dock lets you select a replay source and control it without reopening source properties:

- Load, save, remove, and clear replays
- Navigate first, previous, next, and last replays
- Play, pause, stop, restart, and seek
- Change speed and direction
- Step one or N frames
- Trim the front or end of a replay
- Enable or disable capture

Capture source, duration, output path, filename, and automatic scene behavior remain in the Replay Source properties.

## Building

The project uses the current OBS plugin template and targets OBS Studio 31.1.1 with Qt 6.

### Windows development

Close OBS, then build, install to OBS's shared Windows plugin directory, and launch OBS with one command:

```powershell
.\dev.cmd
```

The script detects Visual Studio 2022 or 2026 and reuses an incremental build directory. Individual stages are also available:

```powershell
.\dev.cmd build
.\dev.cmd install
.\dev.cmd run
```

Windows asks for administrator approval only for the install step because OBS scans `%ProgramData%\obs-studio\plugins`. Building and launching OBS continue as the normal desktop user.

Use `-Configuration Debug`, `RelWithDebInfo`, or `Release` to select a build configuration. If OBS is installed in a nonstandard location, pass `-ObsPath C:\path\to\obs64.exe`.

Configuration is reused after the first build. Pass `-Reconfigure` after changing dependency versions or other CMake configuration inputs.

### Platform presets

The standard CMake presets remain available for CI and other platforms:

```powershell
cmake --preset windows-x64
cmake --build --preset windows-x64
```

Equivalent `macos` and `ubuntu-x86_64` presets are included. Build dependencies are downloaded by the template bootstrap when they are not already present.

## Compatibility and attribution

Source IDs, setting keys, and hotkey IDs are retained from Replay Source 1.8.1 so existing OBS scene collections remain compatible. See [UPSTREAM.md](UPSTREAM.md) for the imported revision and file-level provenance.

This project is licensed under GPL-2.0.
