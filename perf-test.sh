#!/usr/bin/env -S bash -e

if [ ! -e test-input ]; then
  echo "generating test input"
  ./gen.py -i 1000000 -s 0 >test-input
fi

perf record -F max --call-graph=dwarf -g --all-user ./bin/release/bmath -b --unicode <test-input >/dev/null
perf script >out.perf
~/Projects/FlameGraph/stackcollapse-perf.pl out.perf >perf.folded
~/Projects/FlameGraph/flamegraph.pl perf.folded >flamegraph.svg
