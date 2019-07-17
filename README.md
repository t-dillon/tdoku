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
|forum_hardest_1905_11+      |  puzzles/sec|  usec/puzzle|   %no_guess|  guesses/puzzle|
|----------------------------|------------:| -----------:| ----------:| --------------:|
|minisat                     |       712.1 |     1,404.3 |       0.0% |          65.13 |
|kudoku                      |     2,246.3 |       445.2 |        N/A |            N/A |
|bb_sudoku                   |     6,202.2 |       161.2 |       0.0% |         200.78 |
|jsolve                      |     7,557.6 |       132.3 |       0.0% |         213.29 |
|fsss2                       |    11,848.0 |        84.4 |       0.0% |         139.37 |
|jczsolve                    |    12,678.8 |        78.9 |       0.0% |         170.88 |
|skbforce                    |    14,321.2 |        69.8 |       0.0% |         122.59 |
|<b>tdoku                       |    20,401.7 |        49.0 |       0.0% |          64.81 </b>|
</pre>


And here are results on the well-known and commonly-benchmarked dataset of ~49,000 generally very easy 17-clue puzzles:

<pre>
|17_clue                     |  puzzles/sec|  usec/puzzle|   %no_guess|  guesses/puzzle|
|----------------------------|------------:| -----------:| ----------:| --------------:|
|minisat                     |     4,460.5 |       224.2 |      76.2% |           0.83 |
|kudoku                      |    37,772.5 |        26.5 |        N/A |            N/A |
|bb_sudoku                   |   145,576.7 |         6.9 |      76.1% |           1.55 |
|jsolve                      |   171,279.2 |         5.8 |      50.2% |           3.20 |
|fsss2                       |   301,726.3 |         3.3 |      72.6% |           1.31 |
|jczsolve                    |   290,428.9 |         3.4 |      69.5% |           1.90 |
|skbforce                    |   377,337.9 |         2.7 |      73.7% |           0.98 |
|<b>tdoku                       |   304,720.0 |         3.3 |      78.7% |           0.61 </b>|
</pre>

And here is a chart comparing a narrower set of the fastest solvers on a wider range of datasets 
ordered roughly from easiest to hardest:

![](https://docs.google.com/spreadsheets/d/e/2PACX-1vR58Y3aNoz57dQJYDh37c4gvKU_I2E8uOllVHs8xrr0sC52GoUTaC2AXKwUpySG-zFIKynjiM-wN3cX/pubchart?oid=1929162374&format=image)

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
$ CC=clang-8 CXX=clang++-8 ./BUILD.sh -DFSSS2=on -DJCZSOLVE=on -DSKBFORCE=on
$ benchmarks/bench.sh benchmarks/benchmark-clang8.log
$ CC=gcc-6 CXX=g++-6 ./BUILD.sh -DFSSS2=on -DJCZSOLVE=on -DSKBFORCE=on
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
  -m [0|1]            // check for multiple solutions [default 0]
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


