#!/bin/bash

cmake -S . -B Build -G "Ninja"
cmake --build Build -- -j 6
./Bin/Nova-App