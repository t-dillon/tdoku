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

# which solvers to compile
BUILD_SOLVER_ARGS="-DALL=on"
# which solvers to run for profile generation
PGO_EVAL_SOLVERS="norvig,fast_solv_9r2,kudoku,bb_sudoku,jsolve,fsss2,jczsolve,sk_bforce2,tdoku"

ARCH_NAME=$1
TASKSET_CPU=$2
shift 2

if [ "$1" == "mini" ]; then
    MINI=1
    shift
    BUILD_SOLVER_ARGS="-DFSSS2=on -DJCZSOLVE=on -DSK_BFORCE2=on -DRUST_SUDOKU=on"
    PGO_EVAL_SOLVERS="fsss2,jczsolve,sk_bforce2,tdoku"
fi

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
    if [[ "$TARGET" == "msse4.2" ]]; then
        SSEFLAG="-DSSE4_2=on"
    fi
    if [[ "$TARGET" == "avx" ]]; then
        SSEFLAG="-DAVX=on"
    fi
    PGO=""
    if [[ "${TOKS[3]}" == "pgo" ]]; then
        # build for profile generation, profile a test load, move or merge profile, build using profile
        PGO="_pgo"
        rm -rf build/pgodata*
        ./BUILD.sh run_benchmark -DOPT="${OPT}" "${SSEFLAG}" -DARGS="-fprofile-generate=build/pgodata.gen" $BUILD_SOLVER_ARGS
        build/run_benchmark -t15 -w15 -s${PGO_EVAL_SOLVERS} data/puzzles2_17_clue
        if echo "${CC}" | grep -q gcc; then
            mv build/pgodata.gen build/pgodata.use
        else
            "${CC/clang/llvm-profdata}" merge build/pgodata.gen -output build/pgodata.use
        fi
        ./BUILD.sh run_benchmark -DOPT="${OPT}" "${SSEFLAG}" -DARGS="-fprofile-use=pgodata.use" $BUILD_SOLVER_ARGS
    else
        # build without pgo
        ./BUILD.sh run_benchmark -DOPT="${OPT}" "${SSEFLAG}" $BUILD_SOLVER_ARGS
    fi

    # run benchmarks for this spec
    if [ "${MINI}" == "1" ]; then
        setarch $(uname -m) -R taskset ${TASKSET_CPU} build/run_benchmark -t8 -w2 -n100000 -e1 -v0 data/puzzles4* | tee benchmarks/mini_${ARCH_NAME}_${CC}_${OPT}_${TARGET}${PGO}
    else
        mkdir -p benchmarks/results_${ARCH_NAME}
        benchmarks/bench.sh setarch $(uname -m) -R taskset ${TASKSET_CPU} | tee benchmarks/results_${ARCH_NAME}/${ARCH_NAME}_${CC}_${OPT}_${TARGET}${PGO}
    fi
done
