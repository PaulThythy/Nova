#!/bin/bash

cmake -S . -B Build -G "Ninja"
cmake --build Build -- -j 6
./Build/Nova-App/Nova-App