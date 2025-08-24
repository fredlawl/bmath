#!/usr/bin/env -S bash -e

# master
a_dir=./a_perf_test
control_dir=./.bmath_master

# current changes
b_dir=./b_perf_test

function perf_test() {
  report_dir=$1
  build_dir=$2
  binary=$build_dir/bmath

  echo "testing: $report_dir"

  mkdir -p "$report_dir"
  meson setup --wipe --buildtype=debugoptimized "$build_dir"
  meson test -C "$build_dir"
  meson compile -C "$build_dir"

  set -x
  perf record -o "$report_dir"/perf.data -F max --call-graph=dwarf -g --user-callchains "$binary" <test-input &>/dev/null
  perf stat -r 100 -o "$report_dir"/stat.data -- "$binary" <test-input &>/dev/null
  perf script --input "$report_dir"/perf.data >"$report_dir"/out.perf
  ./FlameGraph/stackcollapse-perf.pl "$report_dir"/out.perf >"$report_dir"/perf.folded
  ./FlameGraph/flamegraph.pl "$report_dir"/perf.folded >"$report_dir"/flamegraph.svg
  ./FlameGraph/flamegraph.pl --reverse "$report_dir"/perf.folded >"$report_dir"/flamegraph-reverse.svg
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

if [ ! -d $control_dir ]; then
  echo "setup A test"
  git clone --depth=1 https://github.com/fredlawl/bmath.git $control_dir
fi

# Ensure clean master
(
  cd $control_dir
  git reset --hard origin/master
)

perf_test $a_dir $control_dir/build
perf_test $b_dir ./build

set -x
diff -u --color <(cat $a_dir/stat.data) <(cat $b_dir/stat.data)
set +x
