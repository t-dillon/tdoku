## Tdoku: A fast Sudoku Solver

#### Overview
This project contains an optimized Sudoku solver for conventional 9x9 puzzles. It also contains two 
other solvers with several variations exploring different ideas for optimization visited during
development. Lastly, it contains a benchmarking framework for comparing performance among these 
solvers and among several third party solvers of current or historical interest using a variety of
datasets. 

See https://t-dillon.github.io/tdoku for a long discussion of the thinking behind these solvers.

This project's solvers include:

Solver | Description
-------|------------
[src/solver_dpll_triad_simd.cc](https://github.com/t-dillon/tdoku/blob/master/src/solver_dpll_triad_simd.cc) | The fast solver, optimized with particular focus on speed of solving hard Sudoku instances, and by this measure the fastest of any solver I'm aware of. I'll refer to this one as "Tdoku".
[src/solver_dpll_triad_scc.cc](https://github.com/t-dillon/tdoku/blob/master/src/solver_dpll_triad_scc.cc) | A DPLL-based solver for exploring how the puzzle representation can be optimized and strongly connected components can be exploited to reduce backtracking.| 
[src/solver_basic.cc](https://github.com/t-dillon/tdoku/blob/master/src/solver_basic.cc) | A very simple solver. Fast as simple solvers go, but mainly here to illustrate how poorly such solvers handle hard puzzles. 

See [here](https://github.com/t-dillon/tdoku/blob/master/other/README) for details on third party solvers 
supported for benchmarking. For a glimpse of comparative performance at opposite ends of the difficulty 
spectrum, here are results for the full set of benchmarked solvers on a dataset of ~49,000 very
difficult puzzles with Sudoku Explainer ratings of 11 or higher:

<pre>
|forum_hardest_1905_11+        |  puzzles/sec|  usec/puzzle|   %no_guess|  guesses/puzzle|
|------------------------------|------------:| -----------:| ----------:| --------------:|
|minisat[*]                    |       713.2 |     1,402.2 |       0.0% |          65.31 |
|kudoku                        |     2,255.6 |       443.3 |        N/A |            N/A |
|bb_sudoku                     |     6,318.4 |       158.3 |       0.0% |         200.49 |
|jsolve                        |     7,589.9 |       131.8 |       0.0% |         213.24 |
|fsss2                         |    11,765.5 |        85.0 |       0.0% |         139.30 |
|jczsolve                      |    12,599.9 |        79.4 |       0.0% |         171.21 |
|sk_bforce2                    |    14,334.1 |        69.8 |       0.0% |         122.66 |
|<b>tdoku                         |    20,295.9 |        49.3 |       0.0% |          64.96 </b>|
</pre>
<small>[*] Note: minisat has an unfair advantage in this comparison because the minisat-based solver 
is only looking for the first solution, while the others are all finding the solution <b>and</b> 
confirming that the solution is unique.</small>

And here are results on the well-known and commonly-benchmarked dataset of ~49,000 generally very easy 17-clue puzzles:

<pre>
|17_clue                       |  puzzles/sec|  usec/puzzle|   %no_guess|  guesses/puzzle|
|------------------------------|------------:| -----------:| ----------:| --------------:|
|minisat[*]                    |     4,443.4 |       225.1 |      76.2% |           0.83 |
|kudoku                        |    37,930.2 |        26.4 |        N/A |            N/A |
|bb_sudoku                     |   145,132.1 |         6.9 |      76.1% |           1.54 |
|jsolve                        |   173,017.6 |         5.8 |      50.2% |           3.18 |
|fsss2                         |   300,984.5 |         3.3 |      72.5% |           1.30 |
|jczsolve                      |   291,835.7 |         3.4 |      69.6% |           1.84 |
|sk_bforce2                    |   379,375.8 |         2.6 |      73.8% |           1.00 |
|<b>tdoku                         |   328,803.6 |         3.0 |      78.7% |           0.62 </b>|
</pre>

And here is a chart comparing a narrower set of the fastest solvers on a wider range of datasets 
ordered roughly from easiest to hardest:

![](https://docs.google.com/spreadsheets/d/e/2PACX-1vQGsldjnZhAU0XB-lAcmwQD8PpWMNIkAT0tumcPeS61Fft3sMKs0e9X22LRT26ij07c5U5GwqslnlSX/pubchart?oid=1180131374&format=image)

#### Building and Running

Build this project's solvers and run them through benchmarks as follows:

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

This project is set up to facilitate benchmarking against a number of the fastest known solvers, as
well as some solvers of historical interest. Also included is a solver based on minisat. 
Follow the [instructions here](https://github.com/t-dillon/tdoku/blob/master/other/README) to find
and set up sources where necessary.

With sources set up, the benchmarks [found here](https://github.com/t-dillon/tdoku/tree/master/benchmarks) were run as follows:

```bash
$ CC=clang-8 CXX=clang++-8 ./BUILD.sh -DFSSS2=on -DJCZSOLVE=on -DSK_BFORCE2=on
$ benchmarks/bench.sh benchmarks/benchmark-clang8.log
$ CC=gcc-6 CXX=g++-6 ./BUILD.sh -DFSSS2=on -DJCZSOLVE=on -DSK_BFORCE2=on
$ benchmarks/bench.sh benchmarks/benchmark-gcc6.log
```

See CMakeLists.txt for the set of -D options to pass to BUILD.sh if you want to build and benchmark
all supported solvers. See the benchmark program's help for details of how to run, solvers that
have been built, and build info:

<pre>
$ build/run_benchmark -h
usage: run_benchmark <options> puzzle_file_1 [...] 
options:
  -c [0|1]            // output csv instead of table [default 0]
  -n size             // test set size [default 2500000]
  -r [0|1]            // randomly permute puzzles [default 1]
  -s solver_1,...     // which solvers to run [default all]
  -t secs             // target test time [default 20]
  -v [0|1]            // validate solutions [default 1]
  -w secs             // target warmup time [default 10]
solvers: 
bb_sudoku,jsolve,kudoku,fsss2,fsss2:1,jczsolve,sk_bforce2,minisat,tdoku_basic:0,tdoku_basic:1,
tdoku_dpll_triad_scc:0,tdoku_dpll_triad_scc:1,tdoku_dpll_triad_scc:2,tdoku_dpll_triad_scc:3,
tdoku_dpll_triad_simd
build info: Clang 8.0.1 -O3 -march=native
</pre>

If you'd like to add another solver to the benchmark suite, see [this commit](https://github.com/t-dillon/tdoku/commit/98b599074a00f15b7a13761053b984e237b8511a) for an example of
how to do so. If you have another interesting comparison I'd love to hear from you!


