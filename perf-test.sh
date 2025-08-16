#!/usr/bin/env -S bash -e

a_dir=./a_perf_test

# b_dir is our current changes
b_dir=./

function perf_test() {
  d=$1
  binary=$d/build/bmath
  report_dir=$d/"perf_report"

  echo "testing: $d"

  mkdir -p $report_dir
  meson test -C $d/build
  meson compile -C $d/build

  set -x
  perf record -o $report_dir/perf.data -F max --call-graph=dwarf -g --user-callchains $binary <test-input &>/dev/null
  perf stat -r 9 -o $report_dir/stat.data -- $binary <test-input &>/dev/null
  perf script --input $report_dir/perf.data >$report_dir/out.perf
  ./FlameGraph/stackcollapse-perf.pl $report_dir/out.perf >$report_dir/perf.folded
  ./FlameGraph/flamegraph.pl $report_dir/perf.folded >$report_dir/flamegraph.svg
  ./FlameGraph/flamegraph.pl --reverse $report_dir/perf.folded >$report_dir/flamegraph-reverse.svg
  set +x
}

# Tools setup
if [ ! -d FlameGraph ]; then
  echo "cloning flame graph tools"
  git clone --depth=1 https://github.com/brendangregg/FlameGraph.git
fi

if [ ! -e test-input ]; then
  echo "generating test input"
  ./gen-testdata.py -i 100000 -s 0 >test-input
fi

if [ ! -d $a_dir ]; then
  echo "setup A test"
  git clone --depth=1 https://github.com/fredlawl/bmath.git $a_dir
fi

meson setup --buildtype=debugoptimized $a_dir/build
perf_test $a_dir

meson setup --buildtype=debugoptimized $b_dir/build
perf_test $b_dir

set -x
diff -u --color <(cat $a_dir/perf_report/stat.data) <(cat $b_dir/perf_report/stat.data)
set +x
