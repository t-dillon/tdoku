
# Benchmarked Solvers

The table below lists benchmarked solvers along with several of their characteristics that influence performance
in order to facilitate comparisons between solvers with similar implementations, inference capabilities, and
heuristics. This information is also compactly summarized in solver descriptions in the detailed result files.

| solver                   |primary<br>representation | naked<br>singles | hidden<br>singles | locked<br>cands:row | locked<br>cands:col | extra | heuristics |
|--------------------------|:-------------:|:-------:|:------:|:---------:|:---------:|:------:|:-----------:|
| minisat_minimal          | SAT           |  Y      | -      | -         | -         | CDCL     | Min+         |
| minisat_natural          | SAT           |  Y      | -      | -         | -         | CDCL     | Min+         |
| minisat_complete         | SAT           |  Y      | Y      | -         | -         | CDCL     | Min+         |
| minisat_augmented        | SAT           |  Y      | Y      | Y         | Y         | CDCL     | Min+         |
| _tdev_dpll_triad         | SAT           |  Y      | Y      | Y         | Y         | Triad      | Min        |
| _tdev_dpll_triad_scc_i   | SAT           |  Y      | Y      | Y         | Y         | Triad, SCC | Min+       |
| _tdev_dpll_triad_scc_h   | SAT           |  Y      | Y      | Y         | Y         | Triad      | Min        |
| _tdev_dpll_triad_scc_ih  | SAT           |  Y      | Y      | Y         | Y         | Triad, SCC | Min+       |
| _tdev_basic              | Group         |  -      | -      | -         | -         |          | -            |
| _tdev_basic_heuristic    | Group         |  Y      | -      | -         | -         |          | Min          |
| lhl_sudoku               | Group         |  Y      | -      | -         | -         |          | Min          |
| zerodoku                 | Group         |  Y      | Y      | -         | -         |          | Min          |
| fast_solv_9r2            | Exact Cover Matrix  |  Y| Y      | -         | -         |          | Min          |
| kudoku                   | Exact Cover Matrix  |  Y| Y      | -         | -         |          | Min          |
| norvig                   | Cell          |  Y      | Y      | -         | -         |          | Min          |
| bb_sudoku                | Cell          |  Y      | Y      | Y         | Y         |          | Min          |
| fsss                     | Cell          |  Y      | Y      | Y         | Y         |          | Min          |
| jsolve                   | Cell          |  Y      | Y      | Y         | Y         |          | Min          |
| fsss2                    | Digit         |  Y      | Y      | -         | -         |          | Min          |
| fsss2_locked             | Digit         |  Y      | Y      | Y         | Y         |          | Min          |
| jczsolve                 | Band          |  Y      | Y      | Y         | -         |          | Min          |
| sk_bforce2               | Band          |  Y      | Y      | Y         | Y         |          | Min+         |
| rust_sudoku              | Band          |  Y      | Y      | Y         | -         |          | Min          |
| tdoku                    | Tdoku Box/Band  |  Y    | Y      | Y         | Y         | Triad    | Min+         |


# Benchmarked Data Sets
Solvers were benchmarked using the following data sets.

1. [[source](http://www.kaggle.com/bryanpark/sudoku)] **kaggle** \
A dataset of 100000 puzzles sampled from a larger dataset created for ML experiments and 
hosted on Kaggle. The Kaggle page has a link to the code used for puzzle generation. These
puzzles are *extremely easy*. Trivial even. Each has a large number of clues with lots of
redundancy. 100% of the puzzles are solved with naked singles only. This is  more a test
of a parsing and initialization than actual solving speed.

1. [[source](https://github.com/t-dillon/tdoku/blob/master/src/grid_tools.cc)] **unbiased** \
A dataset of 1 million puzzles sampled with uniform probability (conditional on the number of
clues) from the set of all minimal Sudoku puzzles. This dataset was generated using the grid_tools
puzzle sampler in this project. This sampler over-samples low-clue puzzles relative to high-clue
puzzles, but in a quantifiable way, and for a given clue count every minimal Sudoku arises with
the same probability. This makes an interesting dataset for benchmarking since the puzzles are
representative of Sudoku in general, whereas other data sets often contain puzzles selected for
special properties (clue counts, extremes of difficulty) that may favor one solver or another.
The puzzles in this dataset are generally *very easy* because hard puzzles are rare.

1. [[source](https://web.archive.org/web/20131019184812if_/http://school.maths.uwa.edu.au/~gordon/sudokumin.php)] **17_clue** \
A dataset of *all* 17-clue puzzles (49158 of them). This dataset was initially compiled by Gordon Royle of
the University of Western Australia (up to 49151 puzzles). Between 2017 and 2022 the dataset was completed 
and [proven complete](http://forum.enjoysudoku.com/scan-solution-grids-for-17-clues-as-of-blue-t34012-165.html#p323538) 
via exhaustive search by *champagne*, *blue*, and other members of the Enjoy Sudoku Players Forum.
These puzzles are also *very easy*. About half of them can be solved with naked and hidden singles only. This is largely a test of
how well the solver is optimized for hidden singles.
 
1. [[source](http://magictour.free.fr/sudoku.htm)] **magictour_top1465** \
An old list of 1465 hard puzzles that's been used as a common benchmark (*moderately hard*, but not among 
the hardest by modern standards).

1. [[source](http://forum.enjoysudoku.com/the-hardest-sudokus-new-thread-t6539-600.html#p277835)] **forum_hardest_1905** \
A list of over 2 million *hard* puzzles maintained by members of the Enjoy Sudoku Players Forum.
See ph_1905.zip from the Google Drive link. This was the hardest puzzle list as of May 2019, and
it generally contains puzzles with Sudoku Explainer difficulty ratings above 10.0. Because this
dataset is huge and its puzzles are hard this is a good dataset for testing solving speed. 

1. [[source](http://forum.enjoysudoku.com/the-hardest-sudokus-new-thread-t6539-600.html#p277835)] **forum_hardest_1905_11+** \
A *harder* subset of the forum_hardest_1905 list sampled from puzzles having a Sudoku Explainer difficulty rating 
above 11.0 (around 49,000 puzzles; about the same size as the 17-clue list).

1. [[source](http://forum.enjoysudoku.com/the-hardest-sudokus-new-thread-t6539.html#p65791)] **forum_hardest_1106** \
The list of 376 puzzles that originally kicked off the linked player's forum hardest puzzle thread, 
and which seems to contain on average the *hardest* puzzles of all, at least in terms of backtracking
difficulty (i.e., these puzzles take the longest to solve and require the most backtracking across
a wide range of solvers).

1. [[source](http://sites.google.com/site/sergsudoku/benchmark.zip)] **serg_benchmark** \
A benchmark dataset maintained by user Serg of the Enjoy Sudoku Player's Forum. This dataset 
contains puzzles with two or more solutions. The idea of hardness may not apply to these since
they are not proper Sudoku. However, backtracking solvers are commonly used in searches of various
kinds where it is not known whether a puzzle has 0, 1, or 2+ solutions, so this is an important
use case.

1. [[source](http://www.enjoysudoku.com/gen_puzzles.zip)] **gen_puzzles** \
The raw output of a puzzle generator containing mostly invalid puzzles with 0 solutions. As
with the previous dataset the idea of hardness may not apply to these since they are not proper
Sudoku. However, backtracking solvers are commonly used in searches of various kinds where it is 
not known whether a puzzle has 0, 1, or 2+ solutions, so this is an important use case.

# Benchmarked Platforms

Benchmarks were run for the following CPU platforms (all on Ubuntu 18.04 or 20.04):
1. **Oracle-Optimized3** (Ice Lake Server)\
A compute optimized server platform supporting AVX512BIGALG, AVXVPOPCNTDQ, and AVX512VL instructions.
1. **i7-1065G7** (Ice Lake Client)\
A modern laptop supporting AVX512BIGALG, AVXVPOPCNTDQ, and AVX512VL instructions.
1. **GCE-c2-standard-4** (Cascade Lake)\
A compute optimized server platform supporting AVX512VL instructions.
1. **i5-8600k** (Coffee Lake)\
A fast desktop supporting AVX2 instructions.
1. **i7-4930k** (Ivy Bridge)\
An older desktop with support for SSE4.2 but not AVX2
1. **TR-2990WX** (Threadripper)\
An AMD platorm with AVX2 (but faster for Tdoku without it)

# Benchmarked Compilers
Benchmarks were generally run for Clang-{8,9,10,11} and GCC-{8,9,10} both with and without profile guided optimization, 
and in the case of Threadripper both with and without AVX2. For each (platform X dataset X solver) the charts below show 
the most favorable result across all of the compiler configurations tested (so different compilers may have been used 
for adjacent bars for two different solvers on the same dataset, or for the same solver on different datasets).

# Benchmark Summary

Below is a series of charts comparing the fastest 5 tested solvers across a range of platforms and datasets, where each bar
shows results for the most favorable compiler configuration for the given (platform,dataset,solver) across a range
of compiler configurations tested.

Some generalizations:
* For over-constrained non-minimal puzzles FSSS2 tends to be fastest by a decent margin. This is likely due to FSSS2's
  ability to perform parallel inference, something that is particularly effective for over-constrained puzzles but
  somewhat less effective for minimal puzzles since these are more likely to require long chains of serial inference.
* For easy minimal puzzles (e.g., unbiased and 17-clue) Rust Sudoku tends to be either fastest or on par with the fastest 
  depending on the age of the platform. This is likely because such puzzles require only a few of the simplest and most
  common techniques and members of the JCZSolve family are exceptionally well optimized for just these techniques.
* For harder puzzles Tdoku tends to be fastest by a signficant margin. It incorporates a wider set of inference
  techniques into a unified framework which reduces the amount of search required for hard puzzles. This generality
  comes at the cost of making the simplest inferences more expensive as messages must be passed box->band->box, and this
  is disadvantageous when only simple inferences are required. However, Tdoku mostly offsets this disadvantage on easy
  puzzles with its own form of parallelism.
* For invalid puzzles Tdoku tends to be fastest by a significant margin. This is likely due to Tdoku's method of
  initialization by band, which catches certain inconsistencies very quickly.

![](https://docs.google.com/spreadsheets/d/e/2PACX-1vRrWT05pUsB0LRS8ZR-j7WNvoUIpX6TDHBGeWhJnd7bRedgNn-a60TLVIRYO9A51yUZuXo-ugWx-ibK/pubchart?oid=584139883&format=image)

![](https://docs.google.com/spreadsheets/d/e/2PACX-1vRrWT05pUsB0LRS8ZR-j7WNvoUIpX6TDHBGeWhJnd7bRedgNn-a60TLVIRYO9A51yUZuXo-ugWx-ibK/pubchart?oid=1741583019&format=image)

![](https://docs.google.com/spreadsheets/d/e/2PACX-1vRrWT05pUsB0LRS8ZR-j7WNvoUIpX6TDHBGeWhJnd7bRedgNn-a60TLVIRYO9A51yUZuXo-ugWx-ibK/pubchart?oid=1180131374&format=image)

![](https://docs.google.com/spreadsheets/d/e/2PACX-1vRrWT05pUsB0LRS8ZR-j7WNvoUIpX6TDHBGeWhJnd7bRedgNn-a60TLVIRYO9A51yUZuXo-ugWx-ibK/pubchart?oid=2129238453&format=image)

![](https://docs.google.com/spreadsheets/d/e/2PACX-1vRrWT05pUsB0LRS8ZR-j7WNvoUIpX6TDHBGeWhJnd7bRedgNn-a60TLVIRYO9A51yUZuXo-ugWx-ibK/pubchart?oid=754000374&format=image)

![](https://docs.google.com/spreadsheets/d/e/2PACX-1vRrWT05pUsB0LRS8ZR-j7WNvoUIpX6TDHBGeWhJnd7bRedgNn-a60TLVIRYO9A51yUZuXo-ugWx-ibK/pubchart?oid=2018421902&format=image)
