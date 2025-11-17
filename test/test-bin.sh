#!/bin/bash -e

echo -n "execution directory: "
pwd

../test/test-bin1.bmath 1>/dev/null
../test/test-bin2.bmath 1>/dev/null
./bmath -e all --fmt-justify --fmt-human < ../test/test-bin3.bmath 1>/dev/null
./bmath -e all --fmt-justify --fmt-human ../test/test-bin3.bmath 1>/dev/null
./bmath -e all --fmt-justify --fmt-human "1+1" 1>/dev/null
