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
# To reduce benchmark variance still further, consider niceing & disabling ASLR:
# sudo nice -n -20 benchmarks/bench.sh setarch `uname -m` -R taskset 0x8 | tee benchmark.log
#
prefix="$*"
# Rust builds are always LLVM, so exclude rust_sudoku by default.
solver_arg="-s$(build/run_benchmark -h | grep tdoku | tr ' ' '\n' | grep -v rust_sudoku | xargs | tr ' ' ,)"
if build/run_benchmark -h | grep build.info | grep -q Clang; then
    # If clang, pass no -s arg, so we'll run everything
    solver_arg=""
fi
cmd="build/run_benchmark -t5 -w1 -n250000 -e1 ${solver_arg}"

echo "##################################################################################################"
echo "# BUILD INFO"
echo "##################################################################################################"
build/run_benchmark -h | grep build.info

echo
echo "##################################################################################################"
echo "# CPU INFO"
echo "##################################################################################################"
lscpu

echo
echo "##################################################################################################"
echo "# SOLVER DESCRIPTION KEY"
echo "##################################################################################################"
echo "The first character indicates the solver's primary representation:"
echo "   B: Band-oriented"
echo "   C: Cell-oriented"
echo "   D: Digit-oriented"
echo "   E: Exact-cover"
echo "   G: Group-oriented"
echo "   I: Integer-MIP"
echo "   S: propositional-SAT"
echo "   T: Tdoku-box-band"
echo "The middle section describes the solver's forward propagation capabilities:"
echo "   s: Singles within a cell"
echo "   h: Hidden singles"
echo "   r: Row-oriented locked candidates"
echo "   c: Col-oriented locked candidates"
echo "   -: Something falling short of full propagation of the above"
echo "   +: Something extra (++ two or more extras)"
echo "The last section describes the solver's heuristics:"
echo "   m: Min-candidates branching (generally finding a bivalue)"
echo "   +: Extra considerations to choose between bivalues"

echo
echo "##################################################################################################"
echo "# BENCHMARK"
echo "##################################################################################################"
echo "${prefix} ${cmd} data/*"
for f in data/*; do ${prefix} ${cmd} ${f}; done

