#!/bin/sh -e

# rm CMakeCache.txt
# debuild -us -uc

fpm \
    -s dir \
    -t deb \
    -v 1.0.0 \
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
