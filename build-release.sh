#!/bin/sh -e

VERSION=$(grep -Po "^project\(.*VERSION\s\K[0-9]+\.[0-9]+\.[0-9]+" CMakeLists.txt | tr -d "\n")

./build.sh

fpm \
    -s dir \
    -t deb \
    -v $VERSION \
    -n bmath \
    -a amd64 \
    -m "Frederick Lawler <me@fred.software>" \
    --url "https://github.com/fredlawl/bmath" \
    --description "64bit big-endian bitwise operation calculator" \
    --license "MIT" \
    -d "libncurses6" \
    -d "libc6 >= 2.15" \
    -d "libtinfo6" \
    -C bin \
    .

mv *.deb ./bin
