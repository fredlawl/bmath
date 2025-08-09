#!/usr/bin/env -S bash -e
rm -rf build obj-x86_64-linux-gnu debian/bmath
exec debuild -uc -us -i
