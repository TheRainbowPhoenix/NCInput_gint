#!/bin/bash
mkdir -p build_web
cd build_web
emcmake cmake ..
emmake make
