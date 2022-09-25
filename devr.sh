#!/bin/bash
export CC=/usr/bin/clang
export CXX=/usr/bin/clang++
cd build
cmake ..
cmake --build . 
./cpu-raytracer
