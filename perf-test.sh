#!/usr/bin/env -S bash -e

if [ ! -e test-input ]; then
  echo "generating test input"
  ./gen.py >test-input
fi

perf record -F max --call-graph=dwarf -g --all-user ./bin/usr/bin/bmath --unicode -b <test-input >/dev/null
perf script >out.perf
~/Projects/FlameGraph/stackcollapse-perf.pl out.perf >perf.folded
~/Projects/FlameGraph/flamegraph.pl perf.folded >flamegraph.svg
