## Tdoku: A fast Sudoku Solver and Generator

#### Overview
This project contains an optimized Sudoku solver and puzzle generator for conventional 9x9 puzzles (as well as Sukaku
"pencilmark" puzzles with clues given as negative instead of positive literals). It also contains two 
other solvers with several variations exploring different ideas for optimization visited during
development. Lastly, it contains a benchmarking framework for comparing performance among these 
solvers and among several third party solvers of current or historical interest using a variety of
datasets. 

See [https://t-dillon.github.io/tdoku](https://t-dillon.github.io/tdoku) for a long discussion of the thinking behind these solvers.

This project's solvers include:

Solver | Description
-------|------------
[src/solver_dpll_triad_simd.cc](https://github.com/t-dillon/tdoku/blob/master/src/solver_dpll_triad_simd.cc) | The fast solver, optimized with particular focus on speed of solving hard Sudoku instances, and by this measure the fastest of any solver I'm aware of. I'll refer to this one as "Tdoku".
[src/solver_dpll_triad_scc.cc](https://github.com/t-dillon/tdoku/blob/master/src/solver_dpll_triad_scc.cc) | A DPLL-based solver for exploring how the puzzle representation can be optimized and strongly connected components can be exploited to reduce backtracking.
[src/solver_basic.cc](https://github.com/t-dillon/tdoku/blob/master/src/solver_basic.cc) | A very simple solver. Fast as simple solvers go, but mainly here to illustrate how poorly such solvers handle hard puzzles.

See [here](https://github.com/t-dillon/tdoku/blob/master/other/README.md) for details on third party solvers
supported for benchmarking. For a glimpse of comparative performance (on modern hardware) at opposite ends of the difficulty
spectrum, here are results for a range of benchmarked solvers on a dataset of ~49,000 very
difficult puzzles with Sudoku Explainer ratings of 11 or higher:

<pre>
|data/puzzles5_forum_hardest_1905_11+  |  puzzles/sec|  usec/puzzle|   %no_guess|  guesses/puzzle|
|--------------------------------------|------------:|------------:|-----------:|---------------:|
|minisat_augmented          S/shrc+./m+|       517.9 |     1,930.8 |       0.0% |         104.36 |
|fast_solv_9r2              E/sh..../m.|     2,256.3 |       443.2 |       0.0% |         171.66 |
|kudoku                     E/sh..../m.|     2,492.3 |       401.2 |       0.0% |         142.13 |
|norvig                     C/sh..../m.|       403.8 |     2,476.5 |       0.0% |         178.93 |
|bb_sudoku                  C/shrc../m.|     6,310.4 |       158.5 |       0.0% |         200.41 |
|fsss                       C/shrc../m.|     6,653.9 |       150.3 |       0.0% |         117.52 |
|jsolve                     C/shrc../m.|     7,480.5 |       133.7 |       0.0% |         100.21 |
|fsss2                      D/sh..../m.|    11,272.3 |        88.7 |       0.0% |         139.23 |
|jczsolve                   B/shr.../m.|    12,162.5 |        82.2 |       0.0% |         171.20 |
|sk_bforce2                 B/shrc-./m+|    13,454.2 |        74.3 |       0.0% |         122.64 |
|rust_sudoku                B/shr.../m.|    14,353.8 |        69.7 |       0.0% |         161.94 |
|<b>tdoku                      T/shrc+./m+|    23,229.9 |        43.0 |       0.0% |          64.95</b> |
</pre>

And here are results on the well-known and commonly-benchmarked dataset of ~49,000 generally very easy 17-clue puzzles:

<pre>
|data/puzzles2_17_clue                 |  puzzles/sec|  usec/puzzle|   %no_guess|  guesses/puzzle|
|--------------------------------------|------------:|------------:|-----------:|---------------:|
|minisat_augmented          S/shrc+./m+|     5,052.6 |       197.9 |      76.0% |           1.06 |
|fast_solv_9r2              E/sh..../m.|    37,301.2 |        26.8 |      44.6% |           4.47 |
|kudoku                     E/sh..../m.|    40,634.7 |        24.6 |      44.6% |           4.57 |
|norvig                     C/sh..../m.|     8,696.8 |       115.0 |      44.6% |           4.84 |
|bb_sudoku                  C/shrc../m.|   148,555.5 |         6.7 |      76.0% |           1.55 |
|fsss                       C/shrc../m.|   194,872.7 |         5.1 |      76.0% |           0.94 |
|jsolve                     C/shrc../m.|   193,453.2 |         5.2 |      76.0% |           0.77 |
|fsss2_locked               D/shrc../m.|   274,000.0 |         3.6 |      76.0% |           0.95 |
|jczsolve                   B/shr.../m.|   281,424.2 |         3.6 |      70.5% |           1.76 |
|sk_bforce2                 B/shrc-./m+|   355,984.3 |         2.8 |      74.1% |           1.02 |
|rust_sudoku                B/shr.../m.|   400,162.5 |         2.5 |      70.5% |           1.74 |
|<b>tdoku                      T/shrc+./m+|   359,167.2 |         2.8 |      78.7% |           0.61 </b>|
</pre>

And here is a chart comparing a narrower set of the fastest solvers on a wider range of datasets
ordered roughly from easiest to hardest, and for each solver using the results from its most 
favorable tested compiler and compiler options:

![](https://docs.google.com/spreadsheets/d/e/2PACX-1vRrWT05pUsB0LRS8ZR-j7WNvoUIpX6TDHBGeWhJnd7bRedgNn-a60TLVIRYO9A51yUZuXo-ugWx-ibK/pubchart?oid=1741583019&format=image)

Note: Tdoku makes heavy use of SIMD instructions up to various flavors of AVX-512 when available. As a result
it achieves its best performance on recent Intel hardware like the Ice Lake laptop that produced the tables
and chart above. With older processors there are moderate declines in performance down to SSSE3, and
precipitous declines with SSE2. See the comment [here](https://github.com/t-dillon/tdoku/blob/master/src/simd_vectors.h)
for stats on the availability of SIMD instructions from the Steam hardware survey.


For configuration and full details of the runs used for these comparisons, [see here](https://github.com/t-dillon/tdoku/tree/master/benchmarks/results_i7-1065G7/i7-1065G7_clang-11_O3_native).


#### Building and Running

Build this project's solvers and run them through benchmarks as follows:

```bash
unzip data.zip
./BUILD.sh
./build/run_tests
./build/run_benchmark data/*
```
Building the project also produces a library containing the fast simd solver.  You can build a 
simple test program that reads Sudoku (or 729-character pencilmark Sudoku) from stdin and displays 
the solution count and solution (if unique) like so:

```bash
gcc example/solve.c build/libtdoku.a -O3 -o solve -lstdc++ -lm
# count solutions:
./solve < data/puzzles0_kaggle
# find single solution:
./solve 1 < data/puzzles0_kaggle
```

#### Benchmarking Other Solvers

This project is set up to facilitate benchmarking against a number of the fastest known solvers, as
well as some solvers of historical interest. Also included is a solver based on minisat and able to
test several different SAT encodings. See [here](https://github.com/t-dillon/tdoku/blob/master/other/README.md)
for descriptions of these solvers. So set them up for benchmarking run the following commands:

```bash
git submodule update --init --recursive
other/setup_jczsolve.sh 
other/setup_rust_sudoku.sh 
```

With sources set up, the benchmarks [found here](https://github.com/t-dillon/tdoku/tree/master/benchmarks) were run 
using [BENCH.sh](https://github.com/t-dillon/tdoku/blob/master/BENCH.sh) by specifying an architecture, taskset CPU mask, and list of compiler/flag specifications as
in the following example:

```bash
./BENCH.sh i7-4930k 0x8 gcc-6_O3_native clang-8_O3_native clang-8_O3_native_pgo ...
```

See CMakeLists.txt for the set of -D options to pass to BUILD.sh. If you want to build and benchmark
all supported solvers just pass -DALL=on. See the benchmark program's help for details of how to run, solvers that
have been built, and build info:

```
$ build/run_benchmark -h
usage: run_benchmark <options> puzzle_file_1 [...] 
options:
  -a                  // do rating
  -b                  // rate by backtracks
  -c [0|1]            // output csv instead of table [default 0]
  -e <seed>           // random seed [default random_device{}()]
  -h                  // display this help message
  -n <size>           // test set size [default 2500000]
  -p                  // expect 729 character pencilmark sudoku
  -r [0|1]            // randomly permute puzzles [default 1]
  -s solver_1,...     // which solvers to run [default all]
  -t <secs>           // target test time [default 20]
  -v [0|1]            // validate during warmup [default 1]
  -w <secs>           // target warmup time [default 10]
solvers: 
 minisat_minimal minisat_natural minisat_complete minisat_augmented _tdev_dpll_triad
 _tdev_dpll_triad_scc_i _tdev_dpll_triad_scc_h _tdev_dpll_triad_scc_ih _tdev_basic
 _tdev_basic_heuristic lhl_sudoku zerodoku fast_solv_9r2 kudoku norvig bb_sudoku
 fsss jsolve fsss2 fsss2_locked jczsolve sk_bforce2 tdoku
build info: Clang 8.0.1 -O3  -march=native
```

If you'd like to add another solver to the benchmark suite, see [this commit](https://github.com/t-dillon/tdoku/commit/98b599074a00f15b7a13761053b984e237b8511a) for an example of
how to do so. If you have another interesting comparison I'd love to hear from you!

#### Generating Puzzles

Tdoku includes a program for generating both conventional and pencilmark Sudoku. The process is
governed by a customizable loss function that can drive towards low or high clue puzzles, hard
or easy puzzles, or some combination of goals.
```
$ build/generate -h
usage: generate <options> <pattern_file>

options:
  -c <clue_weight>    scoring weight for number of clues
  -g <guess weight>   scoring weight(exponent) for geo mean guesses
  -r <random weight>  scoring weight for uniform noise
  -d <drop>           number of clues to drop before re-completing
  -e <num_evals>      number of permutations to eval for guess estimate
  -n <pool size>      number of top scored puzzles to keep in pool
  -a [0|1]            display all puzzles (not just top scored)
  -m [0|1]            use minisat instead of tdoku for eval
  -p [0|1]            generate pencilmark puzzles
  -h                  display this help message
```
Examples:
```bash
# starting from 100 random conventional Sudoku puzzle seeds perform {-1+?} generation driving 
# towards hard puzzles (according to minisat evaluation of 50 random puzzle permutations). 
$ build/generate -p0 -c0 -g1 -d1 -n100 -e50

# starting from 100 random pencilmark Sudoku puzzle seeds perform {-2+?} generation driving  
# towards low-clue puzzles with a secondary emphasis on puzzle difficulty (according to minisat 
# evaluation of 5 random puzzle permutations). 
build/generate -c1 -g0.01 -r0 -d2 -n100 -e5
```
