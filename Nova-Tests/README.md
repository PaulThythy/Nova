# Nova-Tests
Test module of Nova game engine

## Build & run

From the Nova project root:

```bash
cmake -S . -B Build -G "Ninja" -DNOVA_BUILD_TESTS=ON
cmake --build Build --target <Executable name of the test project>
./Bin/<Executable name of the test project>
```

Example (`GraphicLayers`):

```bash
cmake -S . -B Build -G "Ninja" -DNOVA_BUILD_TESTS=ON
cmake --build Build --target Nova-Tests-GraphicLayers
./Bin/Nova-Tests-GraphicLayers
```