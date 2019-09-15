#!/bin/bash
#
# Usage:
# benchmark/bench.sh | tee benchmark.log
#
# For increased benchmark stability you may prefer to run on a reserved core. For example, to
# prevent linux from scheduling other tasks on core 3, and to prevent the scheduler itself from
# running on core 3, modify the CMDLINE in /etc/default/grub as below, update-grub, and reboot:
#
# GRUB_CMDLINE_LINUX_DEFAULT="quiet splash isolcpus=3 nohz_full=3"
#
# Then run the benchmark as:
# benchmark/bench.sh taskset 0x8 | tee benchmark.log
#
prefix="$*"
# Rust builds are always LLVM, so exclude rust_sudoku by default.
solver_arg="-s$(build/run_benchmark -h | grep tdoku | tr ' ' '\n' | grep -v rust_sudoku | xargs | tr ' ' ,)"
if build/run_benchmark -h | grep build.info | grep -q Clang; then
    # If clang, pass no -s arg, so we'll run everything
    solver_arg=""
fi
cmd="build/run_benchmark -t25 -w5 -n500000 -e1 ${solver_arg}"

echo "###########################################"
echo "# BUILD INFO"
echo "###########################################"
build/run_benchmark -h | grep build.info

echo
echo "###########################################"
echo "# CPU INFO"
echo "###########################################"
lscpu

echo
echo "###########################################"
echo "# BENCHMARK"
echo "###########################################"
echo "${prefix} ${cmd} data/*"
for f in data/*; do ${prefix} ${cmd} ${f}; done

