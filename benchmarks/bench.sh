#!/bin/bash
(
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
    set -x
    taskset 0x20 build/run_benchmark -t30 -w10 -n500000 -sfsss2,fsss2:1,jczsolve,skbforce,tdoku data/*

) | tee $1
