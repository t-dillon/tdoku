
# Benchmark Data Sets

1. [[source](http://www.kaggle.com/bryanpark/sudoku)] **kaggle** \
A dataset of 100000 puzzles sampled from a larger dataset created for ML experiments and 
hosted on Kaggle. The Kaggle page has a link to the code used for puzzle generation. These
puzzles are *extremely easy*. Trivial even. Each has a large number of clues with lots of
redundancy. 100% of the puzzles are solved with naked singles only. This is  more a test
of a parsing and initialization than actual solving speed.

1. [[source](http://staffhome.ecm.uwa.edu.au/~00013890/sudokumin.php)] **17_clue** \
A complete or nearly-complete dataset of all 17-clue puzzles (49151 of them) maintained by 
Gordon Royle of the University of Western Australia. These puzzles are also *very
easy*. About half of them can be solved with naked and hidden singles only. This is largely a test of
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
A *harder* subset of the list forum_hardest_1905 list having a Sudoku Explainer difficulty rating 
above 11.0 (coincidentally also about 49,000 puzzles, same as the 17-clue list).

1. [[source](http://forum.enjoysudoku.com/the-hardest-sudokus-new-thread-t6539.html#p65791)] **forum_hardest_1106** \
The list of 376 puzzles that originally kicked off the linked player's forum hardest puzzle thread, 
and which seems to contain on average the *hardest* puzzles of all (i.e., the puzzles take the longest
to solve and require the most backtracking for all solvers).

1. [[source](http://sites.google.com/site/sergsudoku/benchmark.zip)] **serg_benchmark** \
A benchmark dataset maintained by user Serg of the Enjoy Sudoku Player's Forum. This dataset 
contains puzzles with two or more solutions. The idea of hardness may not apply to these since
they are not proper Sudoku. However, brute force solvers are commonly used in searches of various
kinds where it is not known whether a puzzle has 0, 1, or 2+ solutions, so this is an important
use case.

1. [[source](http://www.enjoysudoku.com/gen_puzzles.zip)] **gen_puzzles** \
The raw output of a puzzle generator containing mostly invalid puzzles with 0 solutions. As
with the previous dataset the idea of hardness may not apply to these since they are not proper
Sudoku. However, brute force solvers are commonly used in searches of various kinds where it is 
not known whether a puzzle has 0, 1, or 2+ solutions, so this is an important use case.

# Benchmark Machines

1. **GCE-c2-standard-4** (Cascade Lake)\
The most modern platform tested so far. These can take advantage of AVX512VL instructions.
1. **i5-8600k** (Coffee Lake)\
A fast desktop supporting AVX2 instructions.
1. **i7-4930k** (Ivy Bridge)\
An older desktop with support for SSE4.2 but not AVX2
1. **TR-2990WX** (Threadripper)\
An AMD platorm with AVX2 (but faster for Tdoku without it)

# Benchmark Summary

![](https://docs.google.com/spreadsheets/d/e/2PACX-1vQGsldjnZhAU0XB-lAcmwQD8PpWMNIkAT0tumcPeS61Fft3sMKs0e9X22LRT26ij07c5U5GwqslnlSX/pubchart?oid=1180131374&format=image)

![](https://docs.google.com/spreadsheets/d/e/2PACX-1vQGsldjnZhAU0XB-lAcmwQD8PpWMNIkAT0tumcPeS61Fft3sMKs0e9X22LRT26ij07c5U5GwqslnlSX/pubchart?oid=1929162374&format=image)

![](https://docs.google.com/spreadsheets/d/e/2PACX-1vQGsldjnZhAU0XB-lAcmwQD8PpWMNIkAT0tumcPeS61Fft3sMKs0e9X22LRT26ij07c5U5GwqslnlSX/pubchart?oid=1085609822&format=image)

![](https://docs.google.com/spreadsheets/d/e/2PACX-1vQGsldjnZhAU0XB-lAcmwQD8PpWMNIkAT0tumcPeS61Fft3sMKs0e9X22LRT26ij07c5U5GwqslnlSX/pubchart?oid=1298913250&format=image)
