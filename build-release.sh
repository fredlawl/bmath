#!/usr/bin/env -S bash -xe

VERSION=$(grep -Po "^project\(.*VERSION\s\K[0-9]+\.[0-9]+\.[0-9]+" CMakeLists.txt | tr -d "\n")

./build.sh

PKG=$(realpath "bmath_$VERSION"*.deb)
PKG_DBG=$(realpath "bmath-dbg_$VERSION"*.deb)
BINARY="bin/usr/bin/bmath"
BUILD_ID=$(readelf -n "$BINARY" | grep -Po 'Build ID:\s\K[0-9a-zA-Z]+')
BUILD_ID_START=${BUILD_ID:0:2}
BUILD_ID=${BUILD_ID:2}
DEBUG_BINARY="dbg/usr/lib/debug/.build-id/$BUILD_ID_START/$BUILD_ID.debug"

rm -rf dbg
mkdir -p $(dirname "$DEBUG_BINARY")

objcopy --only-keep-debug "$BINARY" "$DEBUG_BINARY"
strip --strip-debug --strip-unneeded "$BINARY"
objcopy --add-gnu-debuglink="$DEBUG_BINARY" "$BINARY"
chmod -x "$DEBUG_BINARY"
rm -f "$PKG" "$PKG_DBG"

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

fpm \
  -s dir \
  -t deb \
  -v $VERSION \
  -n bmath-dbg \
  -a amd64 \
  -m "Frederick Lawler <me@fred.software>" \
  --url "https://github.com/fredlawl/bmath" \
  --description "64bit big-endian bitwise operation calculator" \
  --license "MIT" \
  -d "bmath = $VERSION" \
  --deb-priority optional \
  -C dbg \
  .
