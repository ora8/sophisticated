#!/bin/bash

rm -rf build
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

cp com.example.sophisticatedgtk4.ini build/

#gtk-lauunch build/sophisticated