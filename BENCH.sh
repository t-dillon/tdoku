#!/bin/bash
#
# Run multiple benchmark configurations
#
# BENCH.sh <arch_name> <taskset_cpu_mask> <spec_1> ... <spec_n>
#
# Where a spec is compiler_optlevel_{native|sse4.2}[_pgo]
#
# e.g.,
# BENCH.sh i7-4930k 0x20 gcc-6_O3_native gcc-6_O3_sse4.2 clang-8_Ofast_native_pgo
#

# flags to tell BUILD.sh to include all solvers
ALL_SOLVERS="-DMINISAT=on -DBB_SUDOKU=on -DFAST_SOLV_9R2=on -DKUDOKU=on -DNORVIG=on \
             -DJSOLVE=on -DFSSS2=on -DJCZSOLVE=on -DSK_BFORCE2=on -DRUST_SUDOKU=on"
# which solvers to run for profile generation
PGO_SOLVERS="norvig,fast_solv_9r2,kudoku,bb_sudoku,jsolve,fsss2,jczsolve,sk_bforce2,tdoku"

PLATFORM=$1
TASKSET_CPU=$2
shift 2

for spec in "$@"
do
    # extract compiler options from spec
    IFS='_'
    read -ra TOKS <<< "${spec}"
    IFS=' '
    export CC=${TOKS[0]}
    export CXX=$(echo ${CC} | sed -e "s/gcc/g++/" -e "s/clang/clang++/")
    OPT=${TOKS[1]}
    TARGET=${TOKS[2]}
    SSEFLAG=""
    if [ "$TARGET" == "msse4.2" ]; then
        SSEFLAG="-DSSE4_2=on"
    fi
    PGO=""
    if [ "${TOKS[3]}" == "pgo" ]; then
        # build for profile generation, profile a test load, move or merge profile, build using profile
        PGO="_pgo"
        rm -rf build/pgodata*
        ./BUILD.sh -DOPT=${OPT} ${SSEFLAG} -DARGS="-fprofile-generate=build/pgodata.gen" ${ALL_SOLVERS}
        build/run_benchmark -t15 -w15 -s${PGO_SOLVERS} data/puzzles1_17_clue
        if echo $CC | grep -q gcc; then
            mv build/pgodata.gen build/pgodata.use
        else
            $(echo $CC | sed -e "s/clang/llvm-profdata/") merge build/pgodata.gen -output build/pgodata.use
        fi
        ./BUILD.sh -DOPT=${OPT} ${SSEFLAG} -DARGS="-fprofile-use=pgodata.use" ${ALL_SOLVERS}
    else
        # build witout pgo
        ./BUILD.sh -DOPT=${OPT} ${SSEFLAG} ${ALL_SOLVERS}        
    fi

    # run benchmarks for this spec
    benchmarks/bench.sh setarch `uname -m` -R taskset ${TASKSET_CPU} | tee benchmarks/${PLATFORM}_${CC}_${OPT}_${TARGET}${PGO}
done
