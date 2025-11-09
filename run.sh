#!/bin/bash

cmake -S . -B build
cmake --build build -- -j
./build/Nova-App/Nova-App