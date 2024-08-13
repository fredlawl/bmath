#!/usr/bin/bash -e

# perf record -F max --call-graph=dwarf -g ./bin/usr/bin/bmath -d "0x1890"
perf record -F max --call-graph=dwarf -g --all-user ./bin/usr/bin/bmath --unicode -b < test-input
perf script > out.perf
~/Projects/FlameGraph/stackcollapse-perf.pl out.perf > perf.folded
~/Projects/FlameGraph/flamegraph.pl perf.folded  > flamegraph.svg
# perf script report flamegraph --template /home/fred/Projects/bmath/node_modules/d3-flame-graph/dist/templates/d3-flamegraph-base.html
