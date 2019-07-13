## Tdoku: A fast Sudoku Solver

#### Overview
This project actually contains three Sudoku solvers with variations:

Solver | Description
-------|------------
[src/solver_basic.cc](https://github.com/t-dillon/tdoku/blob/master/src/solver_basic.cc) | A very simple solver. Fast as simple solvers go, but mainly here to illustrate how poorly such solvers handle hard puzzles. 
[src/solver_dpll_triad_scc.cc](https://github.com/t-dillon/tdoku/blob/master/src/solver_dpll_triad_scc.cc) | A DPLL-based solver for exploring how the puzzle representation can be optimized and strongly connected components can be exploited to reduce backtracking.| 
[src/solver_dpll_triad_simd.cc](https://github.com/t-dillon/tdoku/blob/master/src/solver_dpll_triad_simd.cc) | A solver optimized strictly for speed in solving hard Sudoku instances, and by this measure the fastest solver I'm currently aware of.

#### Discussion

See https://t-dillon.github.io/tdoku for a long discussion of the thinking behind these solvers.

#### Building and Running

Build the included solvers and run benchmarks as follows:

```bash
$ unzip data.zip
$ ./BUILD.sh
$ ./build/run_benchmark data/*
```

If you'd like to benchmark against other solvers follow the
[instructions here](https://github.com/t-dillon/tdoku/blob/master/other/README) to find and set up 
sources for FSSS2, JCZSolve, and SK_BFORCE2.

The benchmarks [found here](https://github.com/t-dillon/tdoku/tree/master/benchmarks) were then run as follows:

```bash
$ CC=clang-8 CXX=clang++-8 ./BUILD.sh -DFSSS2=on -DJCZSOLVE=on -DSKBFORCE=on
$ benchmarks/bench.sh benchmarks/benchmark-clang8.log
$ CC=gcc-6 CXX=g++-6 ./BUILD.sh -DFSSS2=on -DJCZSOLVE=on -DSKBFORCE=on
$ benchmarks/bench.sh benchmarks/benchmark-gcc6.log
```

Building the project also produces a gcc-compatible static library containing the fast simd solver.
You can build a simple test program that reads Sudoku from stdin and displays the solution count and
solution (if unique) like so:

```bash
$ gcc example/solve.c build/libtdoku.a -o solve
```

#### Results

Here are results comparing the Tdoku fast simd solver with other state of the art solvers on various
datasets:

![](https://docs.google.com/spreadsheets/d/e/2PACX-1vR58Y3aNoz57dQJYDh37c4gvKU_I2E8uOllVHs8xrr0sC52GoUTaC2AXKwUpySG-zFIKynjiM-wN3cX/pubchart?oid=1929162374&format=image)