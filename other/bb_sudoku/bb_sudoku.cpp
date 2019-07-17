/****************************************************************\
**  BB_Sudoku  Bit Based Sudoku Solver                          **
\****************************************************************/

/****************************************************************\
**  (c) Copyright Brian Turner, 2008-2009.  This file may be    **
**      freely used, modified, and copied for personal,         **
**      educational, or non-commercial purposes provided this   **
**      notice remains attached.                                **
\****************************************************************/

/****************************************************************\
** The solver source code has now been broken into 3 files:     **
**   - bb_sudoku.cpp         : Driver code which handles things **
**                               like decoding arguments,       **
**                               loading puzzles, timings,      **
**                               and outputs.                   **
**   - bb_sudoku_solver.cpp  : The actual solver code           **
**   - bb_sudoku_tables.h    : Various tables used for solving  **
**   - random.h              : Various Random Number Generators **
**                                                              **
** I do use a nonstandard method for including the solver code, **
**   but it makes compiling from the command line easier and    **
**   does not need a makefile or anything.                      **
**                                                              **
**                                                              **
** Compiling:                                                   **
**                                                              **
**   This code has been compiled with both Visual C (Windows),  **
**   and gcc (Linux).  The following shows options used:        **
**                                                              **
**   Visual C (tested under Windows XP)                         **
**     Build under a Win32 Console Application, turn off the    **
**     precomiled headers, and use the Release configuration    **
**     for full optimizations.                                  **
**     NOTE: Under XP, there is a Windows bug when using the    **
**           timer with AMDs that can cause negative times.     **
**                                                              **
**   GCC (tested under Ubuntu 8.10)                             **
**     Use the following command line to compile the code:      **
**                                                              **
**       gcc -O3 -lrt -xc -obb_sudoku.out bb_sudoku.cpp         **
**                                                              **
**         -O3  = preform full optimizations                    **
**         -lrt = needed for linking of timing routines         **
**         -xc  = compile as a C program                        **
**         -oBB_Sudoku.out = name of the output file            **
**         BB_Sudoku.cpp   = name of file to compile            **
**                                                              **
\****************************************************************/

#ifdef WIN32
  #include <windows.h>           // Allow high preformance timing routines
#else
  #include <time.h>              // Allow high preformance timing routines
  #include <stdlib.h>            // For exit()
#endif
#include <stdio.h>               // For IO (fgets, stdin, printf)
#include <cstring>

#include "random.h"              // Include various random number generators
#include "bb_sudoku_tables.h"    // Include the various tables used when solving
#include "bb_sudoku_solver.cpp"  // Include the solver


// Control how the solutions are displayed
char display_mode = 1;  // 0 = no output (for timing)
                        // 1 = output 1 line solution

// Control timing modes
char timing_mode = 0;   // 0 = No timings (external timing)
                        // 1 = full program loading only
                        // 2 = puzzle timing

// Search for a single, or multiple solutions
char num_search = 1;    // 0 = search for all the solutions
                        // 1 = stop searching after 1 solution
                        // 2 = stop if more than 1 found

// Use Methods defines which advanced solving methods are used
//   We use a bit mask to define which methods are to be used
//   Default to using Guesses and Locked Candidates
unsigned int use_methods = USE_GUESSES | USE_LOCK_CAND;

// Use ret_puzzle to time how long it takes to get the solution
//   when used with -D1 -T2, prints the puzzle and time
char ret_puzzle = 1;    // 0 = return the original puzzle
                        // 1 = return the solution
                        // 2 = show fixed numbers

// Show only puzzles with unique solutions, mainly used when
//   generating puzzles to throw away invalid puzzles
char solved_only = 1;   // 0 = only puzzles with unique solutions
                        // 1 = display all puzzles

// Independent check of the final solution
//   (not compatible with Sol_Search = 0)
char double_check = 0;  // 0 = no independent check
                        // 1 = verify the solution

// Calculate category (all solving methods activated)
char calc_category = 0; // 0 = do not calculate category
                        // 1 = calculate category

// Run in generator mode
char gen_mode = 0;      // 0 = solver mode
                        // 1 = generator mode

// Minimize the puzzles
char min_mode = 0;      // 0 = do not minimize
                        // 1 = return minimized puzzles

// Normalize the puzzles
char norm_mode = 0;     // 0 = do not normalize
                        // 1 = return normalized puzzles

// Randomize the puzzles
char random_mode = 0;   // 0 = do not randomize
                        // 1 = return randomized puzzles

// undocumented flag to diplay additional stats (-X)
char show_stats = 0;    // 0 = do not display extra stats
                        // 1 = display extra stats

// undocumented flag to supress extra displays (-Q)
char quiet_mode = 0;    // 0 = display data normally
                        // 1 = supress additional displays

// filename used when an input file is declared
char fname[250] = {0};  // start fname as a 0 length string
FILE *infile = stdin;   // input file handle, default to stdin
FILE *outfile = stdout; // output file handle, default to stdout

// buffer to read in the text version of puzzles (example puzzle included)
char buffer[250] = "....3..824.1..................4.65...8.....3....7........5..7..6.....4...3..2....";


// Forward function definitions
void InitTables (void);
void HandleArgs (int argc, char* argv[]);
void SetupTiming (void);
void DoubleCheck (void);
void DisplaySolution (void);
void DisplayTiming (void);
void ShowValues (void);


// For timing, use the high preformance windows timers
#ifdef WIN32
  LARGE_INTEGER Frequency, StartTime, EndTime, SolveTime;
  #define GETTIME(x) QueryPerformanceCounter (&x)
#else
  struct timespec StartTime, EndTime, DiffTime;
  #define GETTIME(x) (void) clock_gettime(CLOCK_REALTIME, &x)
#endif




/**********\
**  main  *******************************************************\
**                                                              **
**    main routine sets up the timers and handles loading the   **
**    puzzles and calling the solver.                           **
**                                                              **
\****************************************************************/
int main(int argc, char* argv[])
{
  InitTables();                          // Initialize a couple bitbased tables
  HandleArgs (argc, argv);               // Process command line arguments
  SetupTiming();                         // Prepare for timing the solver
  GETTIME (StartTime);                   // Get the current timer for program timing

  // Read the puzzles from the stardard input, looping through all the puzzles
  if (gen_mode)
  { printf ("The generator is not incuded in this version.\n"); exit (0); }
  else
  {
    while (fgets(buffer, 240, infile))
    {
      if (buffer[0] == '#') continue;      // ignore comments in the inputs
      PuzzleCnt++;                         // increment the puzzle count
      PuzSolCnt = 0;                       // reset the solutions for this puzzle
      if (timing_mode == 2)
        GETTIME (StartTime);               // Get the current timer for puzzle timing

      if (min_mode)                        // if minimize mode,
        //Minimize(buffer);                 //   calc the minimized puzzle
        { printf ("The minimizer is not incuded in this version.\n"); exit (0); }
      else if (norm_mode)                  // else if normaize mode,
        //Normalize(buffer);                 //   return the normalized puzzle
        { printf ("The normalizer is not incuded in this version.\n"); exit (0); }
      else if (random_mode)                // else if randomize mode
        //Randomize(buffer);                 //   return a logically equivalent puzzle
        { printf ("The randomizer is not incuded in this version.\n"); exit (0); }
      else                                 // otherwise solve the puzzle
        PuzSolCnt = Solver(num_search, use_methods, ret_puzzle, 0, buffer);

      if (timing_mode == 2)
        GETTIME (EndTime);                 // Get ending time if timing puzzles
      if (double_check) DoubleCheck();     // Verify the solution
      SolvedCnt += PuzSolCnt;              // Update solved count
      if (display_mode) DisplaySolution(); // Display the solution and timing
    }
  }

  GETTIME (EndTime);                     // Get ending timer
  DisplayTiming();                       // Display the final timing and counts
  return 0;
}



/****************\
**  InitTables  *************************************************\
**                                                              **
**    InitTables populates the B2V and NSet tables.  They can   **
**    be initialized when defined, but if the grid sizes are    **
**    increased, the size of these tables will also increase    **
**    exponentially (9x9=512 items, 25x25=33,554,432 items).    **
**                                                              **
\****************************************************************/
void InitTables (void)
{ int i, v;

  RND_Init();
  for (i = 0; i < 512; i++)
  { B2V[i] = NSet[i] = 0;
    for (v = i; v; NSet[i]++)
    { NSetX[i][NSet[i]] = v & -v;
      v = v ^ NSetX[i][NSet[i]];
    }
  }
  for (i = 1; i <= 9; i++)
    B2V[1 << (i-1)] = i;
}




/****************\
**  HandleArgs  *************************************************\
**                                                              **
**    HandleArgs decodes the command line arguments and sets    **
**    the appropriate flags.                                    **
**                                                              **
\****************************************************************/
void HandleArgs (int argc, char* argv[])
{ int    i;

  // Check the arguments passed into the program
  for (i = 1; i < argc; i++)
  {
    if ((argv[i][0] != '-') || (argv[i][1] == '?') ||
        (argv[i][1] == 'h') || (argv[i][1] == 'H'))
    {
      printf ("Usage BB_Sudoku [options] < [puzzlefile]\n\n");
      printf ("  options are : -T0  : No internal timings\n");
      printf ("                -T1  : Time entire program run (Default)\n");
      printf ("                -T2  : Time each puzzle individually\n\n");
      printf ("                -D0  : No solution displayed (Timing mode)\n");
      printf ("                -D1  : Output solutions on a single line\n\n");
      printf ("                -S1  : Solve for 1 solution, Stop after finding 1 solution\n");
      printf ("                -S2  : Stop if 2 solutions found\n");
      printf ("                -S+  : Solve for all solutions\n\n");
      printf ("                -Mx+ : Use method x (see below for available methods)\n");
      printf ("                -Mx- : Do NOT Use method x\n");
      printf ("                        L = Locked Candidates\n");
      printf ("                        S = Disjoint Subsets (Hidden/Naked Pair/Triples/Quads\n");
      printf ("                        F = Fishy series (X-Wing, Swordfish, Jellyfish)\n");
      printf ("                        1 = 1 Step commonality\n");
      printf ("                        2 = 2 Step commonality\n");
      printf ("                        G = Guessing and Backtracking\n\n");
      printf ("                -P   : Returns the puzzle, not the solution\n\n");
      printf ("                -C   : Calculate Category (uses all solving methods)\n\n");
      //printf ("                -G   : Generator mode (displays a series of related puzzles)\n\n");
      //printf ("                -Z   : Minimize  (reduce the number of clues)\n");
      //printf ("                -R   : Randomize (return a logically equivalent puzzle)\n");
      //printf ("                -N   : Normalize (return the puzzles in normalized form)\n\n");
      printf ("                       Default is -T0 -D1 -S1 -MG+ -ML+ -MS- -MF- -M1- -M2-\n\n\n");
      printf ("  puzzlefile is a either text file with a single puzzle on each line,\n");
      printf ("    81 characters per puzzle, or a puzzle string that can contain\n");
	  printf ("    sudoku variations (see the readme.txt for format information).\n\n");
      #ifdef WIN32
        printf ("  When timings are used, the program pauses for 1 second to allow\n");
        printf ("    other threads to end, providing a more accurate timing.\n\n");
      #endif
      exit(0);
    }
    if ((argv[i][1] == 't') || (argv[i][1] == 'T'))
      if ((argv[i][2] >= '0') && (argv[i][2] <= '2'))
        timing_mode = argv[i][2] - '0';
    if ((argv[i][1] == 'd') || (argv[i][1] == 'D'))
      if ((argv[i][2] >= '0') && (argv[i][2] <= '1'))
        display_mode = argv[i][2] - '0';
    if ((argv[i][1] == 's') || (argv[i][1] == 'S'))
    { if (argv[i][2] == '1') num_search = 1;
      if (argv[i][2] == '2') num_search = 2;
      if (argv[i][2] == '+') num_search = 99;
    }
    if ((argv[i][1] == 'm') || (argv[i][1] == 'M'))
    { if ((argv[i][2] == 'g') || (argv[i][2] == 'G'))
      { if (argv[i][3] == '-') use_methods = use_methods & ~USE_GUESSES;
        if (argv[i][3] == '+') use_methods = use_methods |  USE_GUESSES;
      }
      if ((argv[i][2] == 'l') || (argv[i][2] == 'L'))
      { if (argv[i][3] == '-') use_methods = use_methods & ~USE_LOCK_CAND;
        if (argv[i][3] == '+') use_methods = use_methods |  USE_LOCK_CAND;
      }
      if ((argv[i][2] == 's') || (argv[i][2] == 'S'))
      { if (argv[i][3] == '-') use_methods = use_methods & ~USE_SUBSETS;
        if (argv[i][3] == '+') use_methods = use_methods |  USE_SUBSETS;
      }
      if ((argv[i][2] == 'f') || (argv[i][2] == 'F'))
      { if (argv[i][3] == '-') use_methods = use_methods & ~USE_FISHIES;
        if (argv[i][3] == '+') use_methods = use_methods |  USE_FISHIES;
      }
      if (argv[i][2] == '1')
      { if (argv[i][3] == '-') use_methods = use_methods & ~USE_1_STEP;
        if (argv[i][3] == '+') use_methods = use_methods |  USE_1_STEP;
      }
      if (argv[i][2] == '2')
      { if (argv[i][3] == '-') use_methods = use_methods & ~USE_2_STEP;
        if (argv[i][3] == '+') use_methods = use_methods |  USE_2_STEP;
      }
    }
    if ((argv[i][1] == 'p') || (argv[i][1] == 'P')) 
	  { ret_puzzle = 0; if (argv[i][2] == '+')      solved_only = 0; }
    if ((argv[i][1] == 'x') || (argv[i][1] == 'X')) show_stats = 1;
    if ((argv[i][1] == 'v') || (argv[i][1] == 'V')) double_check = 1;
    if ((argv[i][1] == 'q') || (argv[i][1] == 'Q')) quiet_mode = 1;
    if ((argv[i][1] == 'c') || (argv[i][1] == 'C')) { calc_category = display_mode = 1; ret_puzzle = 0; }
    if ((argv[i][1] == 'g') || (argv[i][1] == 'G')) gen_mode = 1;
    if ((argv[i][1] == 'n') || (argv[i][1] == 'N')) norm_mode   = display_mode = 1;
    if ((argv[i][1] == 'r') || (argv[i][1] == 'R')) random_mode = display_mode = 1;
    if ((argv[i][1] == 'z') || (argv[i][1] == 'Z')) min_mode    = display_mode = 1;
    if ((argv[i][1] == 'i') || (argv[i][1] == 'I'))
      if((infile = fopen(&argv[i][2], "r")) == NULL)
      { printf ("Error opening file '%s'", &argv[i][2]); exit(1); }
    if ((argv[i][1] == 'o') || (argv[i][1] == 'O'))
      if((outfile = fopen(&argv[i][2], "w")) == NULL)
      { printf ("Error opening file '%s'", &argv[i][2]); exit(1); }
    if ((argv[i][1] == 'a') || (argv[i][1] == 'A'))
      if((outfile = fopen(&argv[i][2], "a")) == NULL)
      { printf ("Error opening file '%s'", &argv[i][2]); exit(1); }
  }
  if (calc_category) use_methods = USE_LOCK_CAND | USE_SUBSETS | USE_FISHIES |
                                                   USE_1_STEP  | USE_2_STEP;

  // Print Title
  if (!quiet_mode) fprintf (outfile, "Bit Based Sudoku Solver v1.0, by Brian Turner\n\n");
}




/****************\
**  SetupTiming  *************************************************\
**                                                              **
**    SetupTiming increases the waits for 1 second for other    **
**    tasks to finish, and increases the priority of the        **
**    solver.  Since we are using a high preformace timers,     **
**    these steps help make the timing more accurate and        **
**    repeatable (there is still some variation from run to     **
**    run, but much less than if we don't do this).             **
**                                                              **
**    Note: under Windows, doing this really does make the      **
**          timings more consistant. Under Linux, there does    **
**          not appear to be anything I can do to improve       **
**          the consistancy of the timings.  The timings        **
**          appear to change by 10% or more between runs.       **
**                                                              **
\****************************************************************/
void SetupTiming (void)
{
  // Only do this for Windows, it does not seem to help under Linux
  #ifdef WIN32
    // Get the frequency of the timer (for my system, it was a 0.279 usec a tick)
    QueryPerformanceFrequency (&Frequency);

    // If puzzle timing, wait 1 second before running the puzzle to allow
    //   other background threads to finish up.  This provides better timings
    if (timing_mode)
    {
      GETTIME (StartTime);
      do { GETTIME (EndTime); }
      while ((EndTime.QuadPart - StartTime.QuadPart) < Frequency.QuadPart);

      // To make the timing more accurate, raise the priority of the program.
      //   This helps prevent other things from inturrupting the solving.
      SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
      SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
    }
  #endif
}




/*****************\
**  DoubleCheck  *************************************************\
**                                                              **
**    DoubleCheck provides a double check to verify that the    **
**    solver is calculating valid solutions.                    **
**                                                              **
\****************************************************************/
void DoubleCheck (void)
{ int i, j, t;

  for (i = 0; i < 81; i++)
    if ((buffer[i] < '1') || (buffer[i] > '9')) { PuzSolCnt = 0; return; }
  for (i = 0; i < 27; i++)
  { for (j = t = 0; j < 9; j++)
      t |= V2B[buffer[Group[i][j]] - '0'];
    if (t != 0x1FF) PuzSolCnt = 0;
  }
}




/*********************\
**  DisplaySolution  ********************************************\
**                                                              **
**    DisplaySolution handles the final display task for each   **
**    puzzle, and displays the timings.  This is controled by   **
**    by the state of the display_mode and timing_mode.         **
**                                                              **
\****************************************************************/
char CatStr[6][10] = {"0-Invalid", "1-Easy   ", "2-Medium ",
                      "3-Hard   ", "4-Expert ", "5-Insane "};
void DisplaySolution (void)
{ int i;
  float  fulltime = 0.0;

  // Display answer if displays are used
  if (display_mode && (solved_only || (PuzSolCnt == 1)))
  { // Display the solution
    for (i = 0; i < 81; i++)
      fprintf (outfile, "%c", buffer[i]);

    if (!norm_mode)
    { if (!ret_puzzle)        fprintf (outfile, " (%2d)", Givens);
      if (PuzSolCnt == 0)     fprintf (outfile, " - No Solution ");
      else if (PuzSolCnt > 1) fprintf (outfile, " - %3d Found   ", PuzSolCnt);
      else                    fprintf (outfile, " - Solved      ");

      if (calc_category)      fprintf (outfile, "#%s ", CatStr[CatPuzzle(buffer)]);

      // If timing per puzzle, display time
      if (timing_mode == 2)
      {
        #ifdef WIN32
          if (EndTime.QuadPart > StartTime.QuadPart)  // show 0 if neg, (bug in timing on XP)
            fulltime = (float)(EndTime.QuadPart - StartTime.QuadPart) / (float)Frequency.QuadPart;
        #else
          if (StartTime.tv_nsec < EndTime.tv_nsec)
          { DiffTime.tv_sec  = EndTime.tv_sec - StartTime.tv_sec - 1;
            DiffTime.tv_nsec = 1000000000 + EndTime.tv_nsec - StartTime.tv_nsec;
          }
          else
          { DiffTime.tv_sec  = EndTime.tv_sec  - StartTime.tv_sec;
            DiffTime.tv_nsec = EndTime.tv_nsec - StartTime.tv_nsec;
          }
          fulltime = (float)DiffTime.tv_sec + ((float)DiffTime.tv_nsec / 1000000000.0);
        #endif

        fprintf (outfile, "  Time = %9.2f usec", fulltime * 1000000);
      }
    }
    fprintf (outfile, "%s", &buffer[81]);
    fflush (outfile);
  }
}



/*******************\
**  DisplayTiming  **********************************************\
**                                                              **
**    DisplayTiming displays timing of the entire program, and  **
**    puzzle counts (puzzles and solutions).                    **
**                                                              **
\****************************************************************/
void DisplayTiming (void)
{ float  fulltime;

  fprintf (outfile, "  %5ld Puzzles,  %5ld Solved ", PuzzleCnt, SolvedCnt);
  if (timing_mode == 1)
  {
    #ifdef WIN32
      fulltime = (float)(EndTime.QuadPart - StartTime.QuadPart) / (float)Frequency.QuadPart;
    #else
      if (StartTime.tv_nsec < EndTime.tv_nsec)
      { DiffTime.tv_sec  = EndTime.tv_sec - StartTime.tv_sec - 1;
        DiffTime.tv_nsec = 1000000000 + EndTime.tv_nsec - StartTime.tv_nsec;
      }
      else
      { DiffTime.tv_sec  = EndTime.tv_sec  - StartTime.tv_sec;
        DiffTime.tv_nsec = EndTime.tv_nsec - StartTime.tv_nsec;
      }
      fulltime = (float)DiffTime.tv_sec + ((float)DiffTime.tv_nsec / 1000000000.0);
    #endif

    if (fulltime > 0.95)
      fprintf (outfile, "  Time = %3.3f sec   ", fulltime);
    else if (fulltime > 0.00095)
      fprintf (outfile, "  Time = %3.3f msec  ", fulltime * 1000);
    else
      fprintf (outfile, "  Time = %3.3f usec  ", fulltime * 1000000);
    fprintf (outfile, "(%3.3f usec / puzzle)\n\n", fulltime * 1000000 / PuzzleCnt);
  }
  if (show_stats)
    fprintf (outfile, "\n  %9ld Singles Processed\n  %9ld Hidden Singles\n  %9ld Locked Candidates"
            "\n  %9ld SubSets Searched\n  %9ld Fishies\n  %9ld 1-Steps\n  %9ld 2-Steps"
            "\n  %9ld Guesses\n  %9d Max Depth\n",
            SCnt, HCnt, LCCnt, SSCnt, FCnt, OneCnt, TwoCnt, GCnt, MaxDepth);
  fprintf (outfile, "\n");
}

