## Tdoku: A fast Sudoku Solver

#### Overview
This project actually contains three Sudoku solvers with variations:

Solver | Description
-------|------------
[src/solver_basic.cc](https://github.com/t-dillon/tdoku/blob/master/src/solver_basic.cc) | A very simple solver. Fast as simple solvers go, but mainly here to illustrate how poorly such solvers handle hard puzzles. 
[src/solver_dpll_triad_scc.cc](https://github.com/t-dillon/tdoku/blob/master/src/solver_dpll_triad_scc.cc) | A DPLL-based solver for exploring how the puzzle representation can be optimized and strongly connected components can be exploited to reduce backtracking.| 
[src/solver_dpll_triad_simd.cc](https://github.com/t-dillon/tdoku/blob/master/src/solver_dpll_triad_simd.cc) | A solver optimized strictly for speed in solving hard Sudoku instances, and by this measure the fastest solver I'm aware of.

The fast SIMD solver solves 17-clue puzzles at 300,000 puzzles/second, 3.3 usec/puzzle (not that these are hard, but they are a common benchmark).

#### Discussion

See https://t-dillon.github.io/tdoku for a long discussion of the thinking behind these solvers.

#### Building and Running

Build the solvers listed above and run them through benchmarks as follows:

```bash
$ unzip data.zip
$ ./BUILD.sh
$ ./build/run_tests
$ ./build/run_benchmark data/*
```
Building the project also produces a library containing the fast simd solver.  You can build a 
simple test program that reads Sudoku from stdin and displays the solution count and solution (if
unique) like so:

```bash
$ gcc example/solve.c build/libtdoku.a -o solve
$ ./solve < data/puzzles0_kaggle
```

#### Benchmarking Other Solvers

This project is set up to facilitate benchmarking against a number of the fastest known solvers.
Follow the [instructions here](https://github.com/t-dillon/tdoku/blob/master/other/README) to find
and set up sources for FSSS2, JSolve, JCZSolve, and SK_BFORCE2. Also included is the Kudoku solver
discussed [here](https://attractivechaos.wordpress.com/2011/06/19/an-incomplete-review-of-sudoku-solver-implementations/)
and a solver based on Minisat.

With sources set up, the benchmarks [found here](https://github.com/t-dillon/tdoku/tree/master/benchmarks) were run as follows:

```bash
$ CC=clang-8 CXX=clang++-8 ./BUILD.sh -DFSSS2=on -DJCZSOLVE=on -DSKBFORCE=on
$ benchmarks/bench.sh benchmarks/benchmark-clang8.log
$ CC=gcc-6 CXX=g++-6 ./BUILD.sh -DFSSS2=on -DJCZSOLVE=on -DSKBFORCE=on
$ benchmarks/bench.sh benchmarks/benchmark-gcc6.log
```

If you'd like to add another solver to the benchmark suite, see [this commit](https://github.com/t-dillon/tdoku/commit/98b599074a00f15b7a13761053b984e237b8511a) for an example of
how to do so.

#### Results

Here are results comparing the Tdoku fast simd solver with other state of the art solvers on various
datasets:

![](https://docs.google.com/spreadsheets/d/e/2PACX-1vR58Y3aNoz57dQJYDh37c4gvKU_I2E8uOllVHs8xrr0sC52GoUTaC2AXKwUpySG-zFIKynjiM-wN3cX/pubchart?oid=1929162374&format=image)
