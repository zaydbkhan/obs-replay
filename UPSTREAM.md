# Upstream provenance

The replay engine in this repository was imported from:

- Project: `exeldro/obs-replay-source`
- Repository: https://github.com/exeldro/obs-replay-source
- Release: `1.8.1`
- Commit: `18874c465b363355045f6077326f63f65454e186`
- License: GPL-2.0

Imported engine files:

- `replay.c` (integrated into `src/plugin-main.c`)
- `replay-source.c`
- `replay-filter.c`
- `replay-filter-audio.c`
- `replay-filter-async.c`
- `replay.h`
- `data/locale/*.ini`

The upstream build system, CI configuration, installers, generated version header, nested Git metadata, and unused `win-dshow-replay.cpp` experiment were intentionally not imported. The outer repository's OBS plugin template remains the canonical build and packaging system.
