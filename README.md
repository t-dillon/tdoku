## Tdoku: A fast Sudoku Solver

#### Overview
This project contains an optimized Sudoku solver for conventional 9x9 puzzles (as well as Sukaku
"pencilmark" puzzles with clues given as negative instead of positive literals). It also contains two 
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

See [here](https://github.com/t-dillon/tdoku/blob/master/other/README.md) for details on third party solvers
supported for benchmarking. For a glimpse of comparative performance at opposite ends of the difficulty 
spectrum, here are results for a range of benchmarked solvers on a dataset of ~49,000 very
difficult puzzles with Sudoku Explainer ratings of 11 or higher:

<pre>
|forum_hardest_1905_11+        |  puzzles/sec|  usec/puzzle|   %no_guess|  guesses/puzzle|
|------------------------------|------------:| -----------:| ----------:| --------------:|
|minisat[*]               (sat)|       693.9 |      1441.1 |       0.0% |          63.75 |
|norvig                        |       356.4 |      2805.8 |       0.0% |         359.81 |
|kudoku                   (cov)|     2,022.0 |       494.6 |        N/A |            N/A |
|fast_solv_9r2            (dlx)|     2,068.4 |       483.5 |       0.0% |         171.34 |
|bb_sudoku                     |     4,871.6 |       205.3 |       0.0% |         200.59 |
|jsolve                        |     6,372.2 |       156.9 |       0.0% |         213.47 |
|jczsolve                      |     9,670.7 |       103.4 |       0.0% |         171.17 |
|fsss2                         |    10,384.1 |        96.3 |       0.0% |         139.19 |
|rust_sudoku                   |    11,841.2 |        84.5 |        N/A |            N/A |
|sk_bforce2                    |    12,418.1 |        80.5 |       0.0% |         122.63 |
|<b>tdoku                         |    18,906.2 |        52.9 |       0.0% |          64.93 </b>|
</pre>
<small>[*] Note: minisat appears ~2x as fast as it really is in this comparison because the minisat-based solver 
is only looking for the first solution, while the others are all finding the solution <b>and</b> 
confirming that the solution is unique.</small>

And here are results on the well-known and commonly-benchmarked dataset of ~49,000 generally very easy 17-clue puzzles:

<pre>
|17_clue                       |  puzzles/sec|  usec/puzzle|   %no_guess|  guesses/puzzle|
|------------------------------|------------:| -----------:| ----------:| --------------:|
|minisat[*]               (sat)|     4,133.0 |       242.0 |      76.2% |           0.83 |
|norvig                        |     6,599.1 |       151.5 |      44.7% |           9.83 |
|kudoku                   (cov)|    34,126.0 |        29.3 |        N/A |            N/A |
|fast_solv_9r2            (dlx)|    34,213.1 |        29.2 |      44.6% |           4.62 |
|bb_sudoku                     |   111,377.3 |         9.0 |      76.0% |           1.56 |
|jsolve                        |   145,103.4 |         6.9 |      50.1% |           3.21 |
|jczsolve                      |   224,303.9 |         4.5 |      69.6% |           1.90 |
|fsss2                         |   262,448.4 |         3.8 |      72.5% |           1.31 |
|<b>tdoku                         |   292,388.6 |         3.4 |      78.7% |           0.61 </b>|
|rust_sudoku                   |   315,924.3 |         3.2 |        N/A |            N/A |
|sk_bforce2                    |   329,052.3 |         3.0 |      73.8% |           1.01 |
</pre>

And here is a chart comparing a narrower set of the fastest solvers on a wider range of datasets 
ordered roughly from easiest to hardest:

![](https://docs.google.com/spreadsheets/d/e/2PACX-1vRL2K3hr9Eku5tN6qMFyLuRjXMrnHyo88J_HT1Pq9r-C5pFj2OfaDaoMOs3V9JA6eGSR7jWcRo1OIQJ/pubchart?oid=1180131374&format=image)

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
well as some solvers of historical interest. Also included is a solver based on minisat and able to
test several different SAT encodings. 
Follow the [instructions here](https://github.com/t-dillon/tdoku/blob/master/other/README.md) to find
and set up sources where necessary.

With sources set up, the benchmarks [found here](https://github.com/t-dillon/tdoku/tree/master/benchmarks) were run as follows:

```bash
$ CC=clang-8 CXX=clang++-8 ./BUILD.sh -DFSSS2=on -DJCZSOLVE=on -DSK_BFORCE2=on -DRUST_SUDOKU=on
$ benchmarks/bench.sh | tee benchmarks/benchmark-clang8.log
$ CC=gcc-6 CXX=g++-6 ./BUILD.sh -DFSSS2=on -DJCZSOLVE=on -DSK_BFORCE2=on -DRUST_SUDOKU=on
$ benchmarks/bench.sh | tee benchmarks/benchmark-gcc6.log
```

See CMakeLists.txt for the set of -D options to pass to BUILD.sh if you want to build and benchmark
all supported solvers. See the benchmark program's help for details of how to run, solvers that
have been built, and build info:

<pre>
$ build/run_benchmark -h
usage: run_benchmark <options> puzzle_file_1 [...] 
options:
  -c [0|1]            // output csv instead of table [default 0]
  -e <seed>           // random seed [default random_device{}()]
  -n <size>           // test set size [default 2500000]
  -r [0|1]            // randomly permute puzzles [default 1]
  -s solver_1,...     // which solvers to run [default all]
  -t <secs>           // target test time [default 20]
  -v [0|1]            // validate during warmup [default 1]
  -w <secs>           // target warmup time [default 10]
solvers: 
 _tdev_basic _tdev_basic_heuristic minisat_minimal_01 minisat_natural_01 minisat_complete_01 
 minisat_augmented_01 _tdev_dpll_triad _tdev_dpll_triad_scc_i _tdev_dpll_triad_scc_h 
 _tdev_dpll_triad_scc_ih norvig fast_solv_9r2 kudoku bb_sudoku jsolve fsss2 fsss2_locked jczsolve 
 sk_bforce2 rust_sudoku tdoku
build info: Clang 8.0.1 -O3 -march=native
</pre>

If you'd like to add another solver to the benchmark suite, see [this commit](https://github.com/t-dillon/tdoku/commit/98b599074a00f15b7a13761053b984e237b8511a) for an example of
how to do so. If you have another interesting comparison I'd love to hear from you!


