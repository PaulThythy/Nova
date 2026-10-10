# Nova-Tests
Test module of Nova game engine

## Build & run

Executables go to `Bin/<Config>/` (e.g. `Bin/Debug/`, `Bin/Release/`).

Prefer a **multi-config** generator so Debug and Release object files coexist in `Build/` — switching config does not recompile everything.

### Recommended (Ninja Multi-Config)

```bash
cmake --preset ninja-multi
cmake --build --preset ninja-multi-debug --target Nova-Tests-GraphicLayers
./Bin/Debug/Nova-Tests-GraphicLayers

cmake --build --preset ninja-multi-release --target Nova-Tests-GraphicLayers
./Bin/Release/Nova-Tests-GraphicLayers
```

Visual Studio equivalent: `cmake --preset vs2022` then build with `--config Debug` / `--config Release`.

### Classic Ninja (separate trees per config)

If you use single-config Ninja, each build type needs its own directory — otherwise changing `CMAKE_BUILD_TYPE` rebuilds the whole tree:

```bash
cmake --preset ninja-debug
cmake --build --preset ninja-debug --target Nova-Tests-BuildTypes
./Bin/Debug/Nova-Tests-BuildTypes

cmake --preset ninja-release
cmake --build --preset ninja-release --target Nova-Tests-BuildTypes
./Bin/Release/Nova-Tests-BuildTypes
```

### BuildTypes — compare configs / log stripping

```bash
cmake --preset ninja-multi
cmake --build --preset ninja-multi-debug --target Nova-Tests-BuildTypes
./Bin/Debug/Nova-Tests-BuildTypes

cmake --build --preset ninja-multi-release --target Nova-Tests-BuildTypes
./Bin/Release/Nova-Tests-BuildTypes
```

Log visibility by build type:
| Level | Debug | RelWithDebInfo | Release | MinSizeRel |
|-------|-------|----------------|---------|------------|
| TRACE / DEBUG | yes | no | no | no |
| INFO | yes | yes | no | no |
| WARN | yes | yes | yes | no |
| ERROR / FATAL | yes | yes | yes | yes |
