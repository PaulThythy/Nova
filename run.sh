#!/bin/bash

# Multi-config: one Build/ tree, binaries in Bin/<Config>/
# Switch Debug/Release without wiping object files.
cmake --preset ninja-multi
cmake --build --preset ninja-multi-debug --parallel 6
./Bin/Debug/Nova-App
