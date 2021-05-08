
## Other Solvers

To facilitate benchmarking, several solvers of current or historical interest are included here.

So set them up for benchmarking run the following commands:

```bash
git submodule update --init --recursive
other/setup_jczsolve.sh 
other/setup_rust_sudoku.sh 
```

1. **norvig-sudoku** (P.Norvig 2007)\
A C++ port of the solver from Peter Norvig's well known article [Solving Every Sudoku Puzzle](https://norvig.com/sudoku.html).
    * Sources are included as a git submodule.
    * build with -DNORVIG=on

1. **fast_solv_9r2** (I.Zelaya 2008)\
An early exact cover/DLX-based solver originally announced [here](http://programmers.enjoysudoku.com/www.setbb.com/sudoku/viewtopic17e5.html) on the setbb Sudoku Programmers Forum and subsequently included in attractivechaos's [benchmarks](https://attractivechaos.wordpress.com/2011/06/19/an-incomplete-review-of-sudoku-solver-implementations/) of fast solvers in 2011.
   * Patched sources from [here](https://github.com/attractivechaos/plb/blob/master/sudoku/incoming/fast_solv_9r2.c) are already included.
   * build with -DFAST_SOLV_9R2=on
      
1. **zerodoku** (J.Schirle 2008)\
A simple and fast solver included in early benchmarks by members of the setbb Sudoku Programmers Forum, and later by attractivechaos.
   * Patched sources from [here](https://github.com/attractivechaos/plb/tree/master/sudoku/incoming) are already included.
   * build with -DZERODOKU=on   

1. **bb_sudoku** (B.Turner 2009)\
An influential early solver originally announced [here](http://programmers.enjoysudoku.com/www.setbb.com/sudoku/viewtopic4cf4.html) on the setbb Sudoku Programmer's Forum. Capable of using hidden singles, locked candidates, subsets, and various fishes, but configured here for benchmarking using only hidden singles and locked candidates since this is the fastest mode.
   * Patched sources from [here](https://sites.google.com/site/bbsudokufiles) are already included.
   * Build with -DBB_SUDOKU=on

1. **JSolve** (J.Linhart 2010)\
An early fast solver tracing its lineage to bb_sudoku with additional optimization. Originally announced [here](http://programmers.enjoysudoku.com/www.setbb.com/sudoku/viewtopic42ef.html) on the setbb forum and subsequently included in attractivechaos's [benchmarks](https://attractivechaos.wordpress.com/2011/06/19/an-incomplete-review-of-sudoku-solver-implementations/) of fast solvers in 2011.
   * Patched sources from [here](http://www.enjoysudoku.com/JSolve12.zip) are already included.
   * build with -DJSOLVE=on

1. **FSSS** (M.Dobrichev 2010)\
Another fast solver from the era of JSolve and bb_sudoku.
   * Patched sources from [here](https://sites.google.com/site/dobrichev/fsss/) are already included.
   * build with -DFSSS=on

1. **kudoku** (AttractiveChaos 2011)\
A solver based on exact cover (but not using dancing links). Kudoku can be seen in action [here](https://attractivechaos.github.io/plb/kudoku.html) and also as a part of attractivechaos's own [benchmarks](https://attractivechaos.wordpress.com/2011/06/19/an-incomplete-review-of-sudoku-solver-implementations/) of fast solvers in 2011.
   * Patched sources from [here](https://raw.githubusercontent.com/attractivechaos/plb/master/sudoku/sudoku_v1.c) are already included.
   * build with -DKUDOKU=on

1. **FSSS2** (M.Dobrichev 2014-2015)\
A SIMD bitboard-based redesign by the author of FSSS which set a new standard of performance. 
   * Sources are included as a git submodule.
   * build with -DFSSS2=on

1. **lhl_sudoku** (Lee Hsien Loong, <2015)\
A casual but well-known solver with the distinction of being the only one on this list written by a head-of-state.
   * Sources are included as a git submodule.
   * build with -DLHL=on

1. **JCZSolve** (Zhouyundong et al. 2012-2016)\
A fast and influential solver incorporating an innovative representation by Zhouyundong and resulting from the collaborative effort of several members of the New Sudoku Players Forum.
   * Copyright unspecified and sources not directly incliuded. 
   * The setup_jczsolve.sh script will fetch and patch sources from JCZSolve10.zip attached to [this post](http://forum.enjoysudoku.com/3-77us-solver-2-8g-cpu-testcase-17sodoku-t30470-210.html#p249309).
   * build with -DJCZSOLVE=on

1. **SK_BFORCE2** (GPenet 2018)\
A solver derived from JCZSolve and including further optimizations.
   * Sources are included as a git submodule.
   * build with -DSK_BFORCE2=on

1. **rust_sudoku** (Emerentius 2017-2019)\
The rust Sudoku library. A port of JCZSolve to rust and including further optimizations.
   * Sources are included as a git submodule, but require a separate build step.
   * Install [rustup](https://rustup.rs/)
   * Run other/setup_rust_sudoku.sh, which will build rust_sudoku shared libraries using clang 9,10,11
   * Build with -DRUST_SUDOKU=on

1. **gss** (Bart 2019)\
A solver aimed at using as many inference techniques as possible to reduce the need for backtracking (see [here](https://github.com/bartp5/gss)).
   * Sources are included as a git submodule.
   * build with -DGSS=on
 
 For additional comparisons, the project also includes Sudoku solvers based on a few genera purpose SAT solvers and optimizers:
 
 1. **Minisat**
    * `$ sudo apt install minisat libz-dev`
    * Build with -DMINISAT=on 
 
 1. **Gurobi**
    * Install Gurobi
    * Build with -DGUROBI=on -DGUROBI_DIR=${GUROBI_LOCATION}/gurobi900/linux64
  
 1. **Z3**
    * `$ sudo apt install libz3-dev`
    * Build with -DZ3=on
