#!/usr/bin/env -S bash -e

if [ ! -e test-input ]; then
  echo "generating test input"
  ./gen.py -i 1000000 -s 0 >test-input
fi

if [ ! -d FlameGraph ]; then
  git clone --depth=1 https://github.com/brendangregg/FlameGraph.git
fi

perf record -F max --call-graph=dwarf -g --all-user ./build/bmath <test-input >/dev/null
perf script >out.perf
./FlameGraph/stackcollapse-perf.pl out.perf >perf.folded
./FlameGraph/flamegraph.pl --reverse perf.folded >flamegraph.svg
