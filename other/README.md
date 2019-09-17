
## Other Solvers

To facilitate benchmarking, several solvers of current or historical interest can be
configured here.

1. **norvig-sudoku** (P.Norvig 2007)\
 -- To the location of your choice clone https://github.com/t-dillon/norvig-sudoku-bench.git (a fork
    of pauek/norvig-sudoku, which is a C++ port of the original Python solver).\
 -- For the original see: https://norvig.com/sudoku.html \
 -- Link required files using:\
    `$ other/norvig/link.sh path/to/norvig-sudoku-bench`\
 -- build with -DNORVIG=on

1. **fast_solv_9r2** (I.Zelaya 2008)\
 -- Patched sources are included (from [here](https://github.com/attractivechaos/plb/blob/master/sudoku/incoming/fast_solv_9r2.c)).\
 -- An early exact cover/DLX-based solver.\
 -- build with -DFAST_SOLV_9R2=on

1. **bb_sudoku** (B.Turner 2009)\
 -- Patched sources are included (from [here](https://sites.google.com/site/bbsudokufiles)).\
 -- Announced: [setbb post archive](http://programmers.enjoysudoku.com/www.setbb.com/sudoku/viewtopic4cf4.html)\
 -- Build with -DBB_SUDOKU=on

1. **JSolve** (J.Linhart 2010)\
 -- Patched sources are included (from [here](http://www.enjoysudoku.com/JSolve12.zip)).\
 -- Announced: [setbb post archive](http://programmers.enjoysudoku.com/www.setbb.com/sudoku/viewtopic42ef.html)\
 -- build with -DJSOLVE=on

1. **kudoku** (AttractiveChaos 2011)\
 -- Patched sources are included 
    (from [here](https://raw.githubusercontent.com/attractivechaos/plb/master/sudoku/sudoku_v1.c)).\
 -- An early exact cover solver (but not DLX).\
 -- build with -DKUDOKU=on

1. **FSSS2** (M.Dobrichev 2014-2015)\
 -- To the location of your choice clone https://github.com/t-dillon/fsss2_bench (a fork of
    dobrichev/fsss2 with some patches to work with the benchmark program here).\
 -- Link required files using:\
    `$ other/fsss2/link.sh path/to/fsss2_bench`\
 -- build with -DFSSS2=on

1. **JCZSolve** (Zhouyundong et al. 2012-2016)\
 -- Copyright unspecified. Find the source in JCZSolve10.zip attached to [this post](http://forum.enjoysudoku.com/3-77us-solver-2-8g-cpu-testcase-17sodoku-t30470-210.html#p249309).\
 -- From JCZSolve10.zip place files JCZSolve.c, JCZSolve.h in other/JCZSolve\
 -- `$ patch JCZSolve.c JCZSolve.c.diff`\
 -- build with -DJCZSOLVE=on\

1. **SK_BFORCE2** (GPenet 2018 based on JCZSolve)\
 -- To the location of your choice clone https://github.com/t-dillon/SK_BFORCE2_bench (a fork
    of GPenet/SK_BFORCE2 with some patches to work with the benchmark program here)\
 -- Link required files using:\
    `$ other/SK_BFORCE2/link.sh path/to/SK_BFORCE2_bench`\
 -- build with -DSK_BFORCE2=on

1. **rust_sudoku** (Emerentius 2017-2019 based on JCZSolve)\
 -- To the location of your choice clone https://github.com/t-dillon/rust_sudoku_bench (a fork of
    Emerentius/sudoku modified to build a dynamic library the benchmark program can call).\
 -- Build the rust project using:\
    `$ cd path/to/rust_sudoku_bench`\
    `$ cargo build --release`\
 -- Link shared library where tdoku looks for it:\
    `$ cd path/to/tdoku`\
    `$ other/rust_sudoku/link.sh path/to/rust_sudoku_bench`\
 -- Build with -DRUST_SUDOKU=on

