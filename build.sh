#!/bin/bash -e
set -x
rm -rf build
mkdir -p build
pushd build
cmake -DCMAKE_BUILD_TYPE=Release ../CMakeLists.txt
make -j$(nproc)
popd
