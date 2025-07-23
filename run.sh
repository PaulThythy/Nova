#!/bin/bash

build_type="Debug"

cmake -S . -B build/$build_type -DCMAKE_BUILD_TYPE=$build_type
cmake --build build/$build_type --config $build_type
./build/$build_type/bin/Nova

#cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE=Debug
#cmake --build build/Debug --config Debug
#./build/Debug/bin/Nova

#cmake -S . -B build/Release -DCMAKE_BUILD_TYPE=Release
#cmake --build build/Release --config Release
#./build/Release/bin/Nova

#cmake -S . -B build/Dist -DCMAKE_BUILD_TYPE=Dist
#cmake --build build/Dist --config Dist
#./build/Dist/bin/Nova