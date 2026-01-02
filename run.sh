#!/bin/bash

cmake -S . -B build -G "Ninja"
cmake --build build -- -j 6
./build/Nova-App/Nova-App