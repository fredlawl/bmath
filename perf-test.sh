#!/usr/bin/bash -e

# perf record -F max --call-graph=dwarf -g ./bin/usr/bin/bmath -d "0x1890"
perf record -F max --call-graph=dwarf -g ./bin/usr/bin/bmath --unicode -d "0x1890"
perf script report flamegraph --template /home/fred/Projects/bmath/node_modules/d3-flame-graph/dist/templates/d3-flamegraph-base.html
