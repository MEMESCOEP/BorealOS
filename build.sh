#!/bin/bash

mkdir -p cmake-build-debug
cd cmake-build-debug || exit
cmake -DCMAKE_BUILD_TYPE=Debug ..
make build-iso -j$(nproc)
cd ..