#!/bin/bash -e
set -x
mkdir -p build
pushd build
cmake ..
make
popd
