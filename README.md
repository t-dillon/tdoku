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
supported for benchmarking. For a glimpse of comparative performance at opposite ends of the difficulty 
spectrum, here are results for a range of benchmarked solvers on a dataset of ~49,000 very
difficult puzzles with Sudoku Explainer ratings of 11 or higher:

<pre>
|data/puzzles4_forum_hardest_1905_11+  |  puzzles/sec|  usec/puzzle|   %no_guess|  guesses/puzzle|
|--------------------------------------|------------:|------------:|-----------:|---------------:|
|norvig                                |       284.7 |      3512.8 |       0.0% |         185.64 |
|minisat_augmented_01 *          (sat) |       680.4 |      1469.8 |       0.0% |          63.29 |
|fast_solv_9r2                   (dlx) |      1989.5 |       502.6 |       0.0% |         172.32 |
|kudoku                          (cov) |      2034.6 |       491.5 |        N/A |            N/A |
|bb_sudoku                             |      4839.0 |       206.7 |       0.0% |         200.41 |
|jsolve                                |      5317.5 |       188.1 |       0.0% |         213.38 |
|fsss2                                 |      9331.6 |       107.2 |       0.0% |         139.23 |
|jczsolve                              |      9377.1 |       106.6 |       0.0% |         171.20 |
|sk_bforce2                            |     10849.4 |        92.2 |       0.0% |         122.64 |
|rust_sudoku                           |     11506.4 |        86.9 |        N/A |            N/A |
|<b>tdoku                                 |     19716.8 |        50.7 |       0.0% |          64.95 </b>|
</pre>
<small>* Note: minisat appears faster than it really is in this comparison because the minisat-based solver 
is only looking for the first solution, while the others are all finding the solution <b>and</b> 
confirming that the solution is unique.</small>

And here are results on the well-known and commonly-benchmarked dataset of ~49,000 generally very easy 17-clue puzzles:

<pre>
|data/puzzles1_17_clue                 |  puzzles/sec|  usec/puzzle|   %no_guess|  guesses/puzzle|
|--------------------------------------|------------:|------------:|-----------:|---------------:|
|minisat_augmented_01 *          (sat) |      4032.7 |       248.0 |      76.3% |           0.84 |
|norvig                                |      6388.9 |       156.5 |      44.6% |           4.84 |
|fast_solv_9r2                   (dlx) |     33502.9 |        29.8 |      44.6% |           4.47 |
|kudoku                          (cov) |     35380.9 |        28.3 |        N/A |            N/A |
|bb_sudoku                             |    111598.7 |         9.0 |      76.0% |           1.55 |
|jsolve                                |    121918.5 |         8.2 |      49.9% |           3.25 |
|fsss2                                 |    211892.2 |         4.7 |      76.0% |           0.95 |
|jczsolve                              |    219325.6 |         4.6 |      70.5% |           1.76 |
|sk_bforce2                            |    287232.6 |         3.5 |      74.1% |           1.02 |
|rust_sudoku                           |    310515.0 |         3.2 |        N/A |            N/A |
|<b>tdoku                                 |    316959.4 |         3.2 |      78.7% |           0.61 </b>|
</pre>

For configuration and full details of the runs used for this comparison, [see here](https://github.com/t-dillon/tdoku/tree/master/benchmarks/GCE-c2-standard-4_clang-8_O3_native_pgo).

Here is a chart comparing a narrower set of the fastest solvers on a wider range of datasets 
ordered roughly from easiest to hardest, and for each solver using the results from its most 
favorable tested compiler and compiler options:

![](https://docs.google.com/spreadsheets/d/e/2PACX-1vTo3FphfVi9gixAs4nX4nNvLl_sgOZ4lgrqSly32jkGUOBWM92IpYaDg4H7R_3dpo-R3VRl5Ei9DnEE/pubchart?oid=1180131374&format=image)

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
gcc example/solve.c build/libtdoku.a -o solve
# count solutions:
./solve < data/puzzles0_kaggle
# find single solution:
./solve 1 < data/puzzles0_kaggle
```

#### Benchmarking Other Solvers

This project is set up to facilitate benchmarking against a number of the fastest known solvers, as
well as some solvers of historical interest. Also included is a solver based on minisat and able to
test several different SAT encodings. 
Follow the [instructions here](https://github.com/t-dillon/tdoku/blob/master/other/README.md) to find
and set up sources where necessary.

With sources set up, the benchmarks [found here](https://github.com/t-dillon/tdoku/tree/master/benchmarks) were run 
using [BENCH.sh](https://github.com/t-dillon/tdoku/blob/master/BENCH.sh) by specifying an architecture, taskset CPU mask, and list of compiler/flag specifications as
in the following example:

```bash
./BENCH.sh i7-4930k 0x8 gcc-6_O3_native clang-8_O3_native clang-8_O3_native_pgo ...
```

See CMakeLists.txt for the set of -D options to pass to BUILD.sh if you want to build and benchmark
all supported solvers. See the benchmark program's help for details of how to run, solvers that
have been built, and build info:

```
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
 sk_bforce2 rust_sudoku tdoku gss gss[1-8]
build info: Clang 8.0.1 -O3 -march=native
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
