/****************************************************************\
**  BB_Sudoku  Bit Based Sudoku Solver                          **
\****************************************************************/

/****************************************************************\
**  (c) Copyright Brian Turner, 2008-2009.  This file may be    **
**      freely used, modified, and copied for personal,         **
**      educational, or non-commercial purposes provided this   **
**      notice remains attached.                                **
\****************************************************************/
#include <cstdio>
#include <cstring>

#include "random.h"
#include "bb_sudoku_tables.h"

// moved here from driver program bb_sudoku.cpp
bool InitTables()
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
    return true;
}

// G - Grid data structure
//       This contains everything needed for backtracking.
//       The larger this gets, the slower backtracking will be
//         since the entire structure is copied.
struct Grid_Type
{ int          CellsLeft;  // Cells left to solve
  unsigned int Grid[81];   // Possibilities left for each cell
  unsigned int Grp[27];    // Values found in each group
} G[64];

// Solution Grid, which will contain the answer after the puzzle is solved
unsigned int SolGrid[81];

// Naked Singles FIFO stack - new singles found are stored here before
char  SingleCnt = 0;
char  SinglePos[128];
unsigned int SingleVal[128];
#define PushSingle(p, b) { SinglePos[SingleCnt] = p; SingleVal[SingleCnt] = b; SingleCnt++; Changed = 1; }


// Changed Flags
int  Changed;          // Flag to indicate Grid has changed
char ChangedGroup[27]; // Marks which groups have changed
char ChangedLC[27];
int  SingleGroup[27];
#define MarkChanged(x) { ChangedGroup[C2Grp[x][0]] = ChangedGroup[C2Grp[x][1]] = ChangedGroup[C2Grp[x][2]] = Changed = 1; }


// Guess structure
char GuessPos[128];
int  GuessVal[128];
int  OneStepP[81], OneStepI;


// Key global variables used throughout the solving
long   PuzzleCnt = 0;    // Total puzzles tested
long   SolvedCnt = 0;    // Total puzzles solved
int    PuzSolCnt;        // Solutions for a single puzzle
int    No_Sol;           // Flag to indicate there is no solution possible
int    PIdx;             // Position Index, used for guessing and backtracking


// Debug stats
int  SCnt  = 0,  HCnt  = 0, GCnt  = 0, LCCnt  = 0, SSCnt  = 0, FCnt  = 0, OneCnt  = 0, TwoCnt  = 0;
long TSCnt = 0,  THCnt = 0, TGCnt = 0, TLCCnt = 0, TSSCnt = 0, TFCnt = 0, TOneCnt = 0, TTwoCnt = 0;
int  MaxDepth = 0, Givens = 0;


// forward procedure declarations
       int  DecodePuzzleString (int ret_puz, char* buffer);
inline void InitGrid ();
inline void ProcessInitSingles (void);
inline void ProcessSingles (void);
inline void FindHiddenSingles (void);
inline void FindLockedCandidates (void);
inline void FindSubsets (void);
inline void FindFishies (void);
       void DoStep (int doing_2_step, int use_methods);
inline void MakeGuess (void);


/************\
**  Solver  *****************************************************\
**                                                              **
**    Solver runs the sudoku solver.  Input puzzle is in the    **
**    buffer array, and somewhat controlled by a number of      **
**    globals (see the globals at the top of the main program   **
**    for globals and meanings).                                **
**                                                              **
\****************************************************************/
int Solver (char num_search, unsigned int use_methods, char ret_puzzle, int initp, char* buffer)
{ int i, PuzSolCnt = 0;

  if (!initp) 
    use_methods |= DecodePuzzleString(ret_puzzle, buffer); // Load the Grid from the buffer

  // Loop through the puzzle solving routines until finished
  while (Changed)
  { // If No Solution possible, jump straight to the backtrack routine
    if (!No_Sol)
    { // Check if any Singles to be propogated
      if (SingleCnt)
      { SCnt++;
        if (SingleCnt > 2)               // If multiple singles
          ProcessInitSingles();          //   process them all at once
        if (SingleCnt)                   // otherwise
          ProcessSingles();              //   process them one at a time
        if (!G[PIdx].CellsLeft)
        { if (!No_Sol)
          { if (!PuzSolCnt && ret_puzzle)
              for (i = 0; i < 81; i++) buffer[i] = '0' + B2V[SolGrid[i]];
            if (PuzSolCnt && (ret_puzzle == 2))
              for (i = 0; i < 81; i++)
                if (buffer[i] != ('0' + B2V[SolGrid[i]])) buffer[i] = '.';
            PuzSolCnt++;
            if ((PuzSolCnt > 1) && (num_search == 2)) break;
            if (num_search == 1) break;
          }
          No_Sol = Changed = 1;
          continue;
        }
      }

      // If nothing has changed, apply the next solver
      if (Changed)
      { HCnt++; FindHiddenSingles(); if (SingleCnt) continue;
        if (use_methods & USE_LOCK_CAND)
          { LCCnt++;  FindLockedCandidates(); if (Changed) continue; }
        if (use_methods & USE_SUBSETS)
          { SSCnt++;  FindSubsets();          if (Changed) continue; }
        if (use_methods & USE_FISHIES)
          { FCnt++;   FindFishies();          if (Changed) continue; }
        if (use_methods & USE_1_STEP)
          { OneCnt++; DoStep(0, use_methods); if (Changed) continue; }
        if (use_methods & USE_2_STEP)
          { TwoCnt++; DoStep(1, use_methods); if (Changed) continue; }
      }
    }

    //If nothing new found, just make a guess
    if (use_methods & USE_GUESSES)
    { GCnt++; MakeGuess(); }
    if (No_Sol) break;
    if (!initp && (MaxDepth < PIdx)) MaxDepth = PIdx;
    if (PIdx > 62) { printf ("Max Depth exceeded, recompile for more depth.\n\n"); exit(0); }
  }
  if (No_Sol && !initp && (ret_puzzle == 2) && !(use_methods & USE_GUESSES))
    for (i = 0; i < 81; i++)
      buffer[i] = (G[0].Grid[i]) ? '.' : '0' + B2V[SolGrid[i]];

  TSCnt   += SCnt;    // Update Stats
  THCnt   += HCnt;
  TGCnt   += GCnt;
  TLCCnt  += LCCnt;
  TSSCnt  += SSCnt;
  TFCnt   += FCnt;
  TOneCnt += OneCnt;
  TTwoCnt += TwoCnt;
  return PuzSolCnt;
}




/************************\
**  DecodePuzzleString  *****************************************\
**                                                              **
**    DecodePuzzleString sets up the initial grid (along with   **
**    InitGrid), and determine what variations are being used.  **
**                                                              **
\****************************************************************/
int DecodePuzzleString (int ret_puz, char* buffer)
{ int p=0, i=0, KillIdx = 0, CompIdx = 0, g, ret_methods = 0, cpos = 0;
  char b[500];

  InitGrid();
  p = 0; i = 2; b[0] = 'g'; b[1] = '=';
  while (buffer[p] > 0)                    // remove spaces and commas
  { if (buffer[p] == '#') { cpos = p; break; }
    if ((buffer[p] != ' ') && (buffer[p] != 9))
    { b[i] = buffer[p]; i++;} p++;
  }
  if (ret_puz)
  { if (cpos) memmove(&buffer[81], &buffer[p], strlen(buffer)-p+1);
    else      buffer[81] = 0;                // end the buffer
  }

  b[i] = b[i+1] = b[i+2] = 0; p = 0;
  if (b[3] == '=') p = 2;

  while (b[p] > 0)
  { if (b[p+1] == '=')
      switch (b[p])
      { case 'p' : // Puzzle size (needs to be defined first in used)
        case 'P' :
                   printf ("Size variation are not supported in this version\n");
                   exit (0);
                   break;
        case 'c' : // Comparison puzzle (Greater Than / Less Than)
        case 'C' : //p = Decode_GT_LT(p, b);
                   //if (GT_LT[0][0] != GT_LT[0][1]) ret_methods |= GT_LT_PUZZLE;
                   //break;
        case 'k' : // Killer Sudoku (Additive or Multiplicitive, and disjoint)
        case 'K' :
                   //break;
        case 'j' : // Jigsaw Sudoku (plus additional groupings and disjoint)
        case 'J' :
                   //break;
        case 'e' : // Even / Odd Sudoku
        case 'E' :
                   //break;
        case 's' : // Subset puzzles
        case 'S' :
                   //break;
        case 'a' : // Additional constraints (diagonals, etc)
        case 'A' :
                   printf ("Variations are not supported in this version\n");
                   exit (0);
                   break;
        case 'g' : // Givens (default puzzle)
        case 'G' :
        default  : p += 2; g = i = 0;
                   while (g < 81)
                   { if ((b[i+p] == ':') || (b[i+p] == ';') || (b[i+p] == '#') || (b[i+p] == 0)) break;
                     if ((b[i+p] == '/') || (b[i+p] == '\\'))
                       g = g + 9 - (g % 9) - 1;
                     if ((b[i+p] > '0') && (b[i+p] <= '9'))
                       PushSingle((char)(g), V2B[b[i+p] - '0']);
                     i++; g++;
                   }
                   Givens += SingleCnt;
                   break;
      }
    p += i;
  }
  buffer[81] = 10; buffer[82] = 0;  // end the buffer
  return ret_methods;
}




/**************\
**  InitGrid  ***************************************************\
**                                                              **
**    InitGrid takes the text string stored in the buffer and   **
**    populates the solution grid.  It assumes the text is      **
**    properly formatted.                                       **
**                                                              **
\****************************************************************/
inline void InitGrid (void)
{ char i;

  // Initialize some key variables
  G[0].CellsLeft = 81;
  PIdx = 0;
  SingleCnt = 0;
  Changed = 1;
  No_Sol = 0;
  OneStepI = 0;
  Givens = 0;
  SCnt = HCnt = GCnt = LCCnt = SSCnt = FCnt = OneCnt = TwoCnt = 0;

  // Loop through the buffer and set the singles to be set
  for (i = 0; i < 81; i++)
  { G[0].Grid[i] = 0x1FF;
    SolGrid[i] = OneStepP[i] = 0;
  }

  // Clear the Groups Found values
  for (i = 0; i < 27; i++)
    G[0].Grp[i] = ChangedGroup[i] = ChangedLC[i] = SingleGroup[i] = 0;
}




/************************\
**  ProcessInitSingles  *****************************************\
**                                                              **
**    ProcessInitSingles takes a naked single and marks each    **
**    cell in the 3 associated groups as not allowing that      **
**    number.  It also marks the groups as changed so we know   **
**    to check for hidden singles in that group.                **
**                                                              **
**    This routines marks all the groups first, then marks the  **
**    cells for each changed groups.                            **
**                                                              **
\****************************************************************/
inline void ProcessInitSingles (void)
{ int i, t, g, t2, j;
  unsigned int b;

  while (SingleCnt > 2)
  { for (i = 0; i < SingleCnt; i++)
    { t = SinglePos[i];                     // Get local copy of position
      b = SingleVal[i];                     // Get local copy of the value

      if (G[PIdx].Grid[t] == 0) continue;   // Check if we already processed this position
      if (!(G[PIdx].Grid[t] & b))           // Check for error conditions
      { No_Sol = 1; SingleCnt = Changed = 0; return; }  
      SolGrid[SinglePos[i]] = SingleVal[i]; // Store the single in the solution grid
      G[PIdx].CellsLeft--;                  // mark one less empty space
      G[PIdx].Grid[t] = 0;                  // mark this position processed
      for (g = 0; g < 3; g++)               // loop through all 3 groups 
      { if (G[PIdx].Grp[C2Grp[t][g]] & b)
        { No_Sol = 1; SingleCnt = Changed = 0; return; }  
	    G[PIdx].Grp[C2Grp[t][g]] |= b;      // mark the value as found in the group
        SingleGroup[C2Grp[t][g]]  = 1;
	  }
    }

    SingleCnt = 0;
    for (i = 0; i < 27; i++)
      if (SingleGroup[i])
      { SingleGroup[i] = 0;
        for (j = 0; j < 9; j++)
		{ t2 = Group[i][j];                 // get temp copy of position
		  b = G[PIdx].Grp[i];
          if (G[PIdx].Grid[t2] & b)         // check if removing a possibility
          { G[PIdx].Grid[t2] = G[PIdx].Grid[t2] & ~b;   // remove possibility
            if (G[PIdx].Grid[t2] == 0)                  // check for error (no possibility)
              { No_Sol = 1; SingleCnt = 0; Changed = 0; return; }  
            if (B2V[G[PIdx].Grid[t2]])                  // Check if a naked single is found
              PushSingle(t2, G[PIdx].Grid[t2]);         
            MarkChanged(t2);                            // Mark groups as changed
          }
		}
      }
  }
}




/********************\
**  ProcessSingles  *********************************************\
**                                                              **
**    ProcessSingles takes a naked single and marks each cell   **
**    in the 3 associated groups as not allowing that number.   **
**    It also marks the groups as changed so we know to check   **
**    for hidden singles in that group.                         **
**                                                              **
**    This routines marks cells changed as each single is       **
**    processed.                                                **
**                                                              **
\****************************************************************/
inline void ProcessSingles (void)
{ int i, t, g, t2;
  unsigned int b;

  for (i = 0; i < SingleCnt; i++)
  { t = SinglePos[i];                     // Get local copy of position
    b = SingleVal[i];                     // Get local copy of the value

    if (G[PIdx].Grid[t] == 0) continue;   // Check if we already processed this position
    if (!(G[PIdx].Grid[t] & b))           // Check for error conditions
    { No_Sol = 1; SingleCnt = Changed = 0; return; }
    SolGrid[SinglePos[i]] = SingleVal[i]; // Store the single in the solution grid
    G[PIdx].CellsLeft--;                  // mark one less empty space
    G[PIdx].Grid[t] = 0;                  // mark this position processed

    for (g = 0; g < 3; g++)               // loop through all 3 groups
      G[PIdx].Grp[C2Grp[t][g]] |= b;      // mark the value as found in the group
    for (g = 0; g < 20; g++)              // loop through each cell in the groups
    { t2 = (int)In_Groups[t][g];          // get temp copy of position
      if (G[PIdx].Grid[t2] & b)           // check if removing a possibility
      { G[PIdx].Grid[t2] = G[PIdx].Grid[t2] ^ b;    // remove possibility
        if (G[PIdx].Grid[t2] == 0)                  // check for error (no possibility)
           { No_Sol = 1; SingleCnt = 0; Changed = 0; return; }
        if (B2V[G[PIdx].Grid[t2]])                  // Check if a naked single is found
          PushSingle(t2, G[PIdx].Grid[t2]);
        MarkChanged(t2);                            // Mark groups as changed
      }
    }
  }
  SingleCnt = 0;                          // Clear the single count
}




/***********************\
**  FindHiddenSingles  ******************************************\
**                                                              **
**    FindHiddenSingles checks each grouping that has changed   **
**    to see if they contain any hidden singles.  If one is     **
**    found, the routine adds it to the queue and exits.        **
**                                                              **
\****************************************************************/
inline void FindHiddenSingles (void)
{ unsigned int t1, t2, t3, b, t;
  int i, j;

  for (i = 0; i < 27; i++)
    if (ChangedGroup[i])
    { ChangedLC[i] = 1;
      t1 = t2 = t3 = 0;
      for (j = 0; j < 9; j++)
      { b  = G[PIdx].Grid[Group[i][j]];
        t2 = t2 | (t1 & b);
        t1 = t1 | b;
      }
      if ((t1 | G[PIdx].Grp[i]) != 0x01FF)
        { No_Sol = 1; SingleCnt = 0; Changed = 0; return; }
      t3 = t2 ^ t1;
      if (t3)
      { for (j = 0; j < 9; j++)
          if (t3 & G[PIdx].Grid[Group[i][j]])
          { t = t3 & G[PIdx].Grid[Group[i][j]];
            if (!B2V[t]) { No_Sol = 1; SingleCnt = 0; Changed = 0; return; }
            PushSingle((char)Group[i][j], t);
            if (t3 == t) return;
            t3 = t3 ^ t;
          }
      }
      ChangedGroup[i] = 0;
    }
  Changed = 0;
}




/**************************\
**  FindLockedCandidates  ***************************************\
**                                                              **
**    FindLockedCandidates will scan through the grid to find   **
**    any locked candidates.                                    **
**                                                              **
\****************************************************************/
inline void FindLockedCandidates (void)
{ unsigned int Seg[9], b;
  int i, j, k, x;
  int s, s1, s2, gp;

  for (i = 0; i < 18; i += 3)
    if (ChangedLC[i] || ChangedLC[i+1] || ChangedLC[i+2])
    { ChangedLC[i] = ChangedLC[i+1] = ChangedLC[i+2] = 0;
      Seg[0] = G[PIdx].Grid[Group[i  ][0]] | G[PIdx].Grid[Group[i  ][1]] | G[PIdx].Grid[Group[i  ][2]];
      Seg[1] = G[PIdx].Grid[Group[i  ][3]] | G[PIdx].Grid[Group[i  ][4]] | G[PIdx].Grid[Group[i  ][5]];
      Seg[2] = G[PIdx].Grid[Group[i  ][6]] | G[PIdx].Grid[Group[i  ][7]] | G[PIdx].Grid[Group[i  ][8]];
      Seg[3] = G[PIdx].Grid[Group[i+1][0]] | G[PIdx].Grid[Group[i+1][1]] | G[PIdx].Grid[Group[i+1][2]];
      Seg[4] = G[PIdx].Grid[Group[i+1][3]] | G[PIdx].Grid[Group[i+1][4]] | G[PIdx].Grid[Group[i+1][5]];
      Seg[5] = G[PIdx].Grid[Group[i+1][6]] | G[PIdx].Grid[Group[i+1][7]] | G[PIdx].Grid[Group[i+1][8]];
      Seg[6] = G[PIdx].Grid[Group[i+2][0]] | G[PIdx].Grid[Group[i+2][1]] | G[PIdx].Grid[Group[i+2][2]];
      Seg[7] = G[PIdx].Grid[Group[i+2][3]] | G[PIdx].Grid[Group[i+2][4]] | G[PIdx].Grid[Group[i+2][5]];
      Seg[8] = G[PIdx].Grid[Group[i+2][6]] | G[PIdx].Grid[Group[i+2][7]] | G[PIdx].Grid[Group[i+2][8]];
      for (j = 0; j < 9; j++)
      { b = Seg[j] & ((Seg[LC_Segment[j][0]] | Seg[LC_Segment[j][1]]) ^ (Seg[LC_Segment[j][2]] | Seg[LC_Segment[j][3]]));
        if (b)
        {
          for (k = 0; k < 4; k++)
          { s = LC_Segment[j][k];
            s1 = i+s/3;  s2 = (s%3)*3;
            for (x = 0; x < 3; x++)
            { gp = Group[s1][s2+x];
              if (G[PIdx].Grid[gp] & b)
              { G[PIdx].Grid[gp] = G[PIdx].Grid[gp] & ~b;
                MarkChanged(gp);
                if (!(G[PIdx].Grid[gp]))
                  { No_Sol = 1; SingleCnt = 0; Changed = 0; return; }
                if (B2V[G[PIdx].Grid[gp]])
                  PushSingle(gp, G[PIdx].Grid[gp]);
              }
            }
          }
          return;
        }
      }
    }
}




/*****************\
**  FindSubsets  ************************************************\
**                                                              **
**    FindSubsets will find all disjoint subsets (Naked/Hidden  **
**    Pairs/Triples/Quads).  While this reduces the solving     **
**    time on some puzzles, and reduces the numbers of guesses  **
**    by around 18%, the overhead increases solving time for    **
**    general puzzles by 100% (doubling the time).              **
**                                                              **
\****************************************************************/
inline void FindSubsets (void)
{
  int i, g, p, m, t[8], v[8];

  for (g = 0; g < 27; g++)
  { if (Changed) break;
    m = 7 - NSet[G[PIdx].Grp[g]];
    if (m < 2) continue;
    p = 1; t[1] = -1;

    while(p > 0)
    { t[p]++;
      if (t[p] == 9) { p--; continue; }
      if (p == 1)
      { v[1] = G[PIdx].Grid[Group[g][t[p]]];
        if ((!v[1]) || (NSet[v[1]] > m)) continue;
        p++; t[2] = t[1]; continue;
      }
      if (!G[PIdx].Grid[Group[g][t[p]]]) continue;
      v[p] = v[p-1] | G[PIdx].Grid[Group[g][t[p]]];
      if (NSet[v[p]] > m) continue;
      if (NSet[v[p]] == p)
      { for (i = 0; i < 9; i++)
          if ((G[PIdx].Grid[Group[g][i]] & ~v[p]) && (G[PIdx].Grid[Group[g][i]] & v[p]))
          { G[PIdx].Grid[Group[g][i]] &= ~v[p];
            if (B2V[G[PIdx].Grid[Group[g][i]]])
              PushSingle(Group[g][i], G[PIdx].Grid[Group[g][i]]);
            MarkChanged(Group[g][i]);
          }
        p = 1;
        continue;
      }
      if (p < m) { p++; t[p] = t[p-1]; }
    }
  }
}




/*****************\
**  FindFishies  ************************************************\
**                                                              **
**    FindFishies finds the row / column subsets, such as       **
**    X-Wing, Swordfish, and Jellyfish.                         **
**                                                              **
\****************************************************************/
inline void FindFishies (void)
{ int b, i, g, p, m, t[10], v[10], grid[10], x, y;

  for (b = 1; b <= 9; b++)
  { if (Changed) break;
    for (g = 0; g < 9; g++) grid[g] = 0;
    for (g = 0; g < 9; g ++)
      for (i = 0; i < 9; i++)
        if (G[PIdx].Grid[(g*9)+i] & V2B[b]) grid[g] |= V2B[i+1];

    for (g = m = 0; g < 9; g++)
      if (grid[g]) m++;
    if (m < 2) continue;
    m--;
    p = 1; t[1] = -1; t[0] = 0;
    while(p > 0)
    { t[p]++;
      if (t[p] == 9) { p--; continue; }
      if (p == 1)
      { v[1] = grid[t[p]];
        if ((!v[1]) || (NSet[v[1]] > m)) continue;
        p++; t[2] = t[1]; continue;
      }
      if (!grid[t[p]]) continue;
      v[p] = v[p-1] | grid[t[p]];
      if (NSet[v[p]] > m) continue;
      if (NSet[v[p]] == p)
      { for (i = 0; i < 9; i++)
          if (grid[i] & ~v[p])
          { x = grid[i] & v[p];
            while (x)
            { y = (i*9) + B2V[x & -x] - 1;
              if (G[PIdx].Grid[y] & V2B[b])
              { G[PIdx].Grid[y] ^= V2B[b];
                if (B2V[G[PIdx].Grid[y]])
                  PushSingle(y, G[PIdx].Grid[y]);
                MarkChanged(y);
              }
              x ^= (x & -x);
            }
          }
        p = 1;
        continue;
      }
      if (p < m) { p++; t[p] = t[p-1]; }
    }
  }
}




/************\
**  DoStep  *****************************************************\
**                                                              **
**    FindFishies finds the row / column subsets, such as       **
**    X-Wing, Swordfish, and Jellyfish.                         **
**                                                              **
\****************************************************************/
void DoStep (int doing_2_step, int use_methods)
{ static int LastPos = 0;
  int i, j, x1, x2, x3, mt, testcell, tested[81], testpidx, new_method;

  for (i = 0; i < 81; i++) tested[i] = 0;
  testpidx = PIdx;

  while (!Changed)
  { // Find next spot to check
    testcell = mt = x3 = 99;
    for (i = 0; i < 81; i++)
      if ((NSet[G[PIdx].Grid[i]] <= mt) && (NSet[G[PIdx].Grid[i]] > 1) && !tested[i])
      { x1 = NSet[G[PIdx].Grp[C2Grp[i][0]]] + NSet[G[PIdx].Grp[C2Grp[i][1]]] + NSet[G[PIdx].Grp[C2Grp[i][2]]];
        if ((NSet[G[PIdx].Grid[i]] < mt) ||
            ((NSet[G[PIdx].Grid[i]] == mt) && (x1 <= x2) && (OneStepP[i] <= x3)))
        { x2 = x1;
          testcell = i;
          mt = NSet[G[PIdx].Grid[i]];
          x3 = OneStepP[i];
        }
      }

    if (testcell == 99) return;
    tested[testcell] = 1;
    OneStepI++; OneStepP[testcell] = OneStepI;

    for (j = 0; j < NSet[G[PIdx].Grid[testcell]]; j++)
    {
      G[PIdx+1+j] = G[PIdx];
      PushSingle(testcell, NSetX[G[PIdx].Grid[testcell]][j]);
      PIdx = testpidx + 1 + j;
      if ((use_methods & USE_2_STEP) && doing_2_step)
        new_method = (use_methods | USE_1_STEP) & ~(USE_GUESSES | USE_2_STEP);
      else
        new_method = use_methods & ~(USE_GUESSES | USE_1_STEP | USE_2_STEP);
      Solver(1, new_method, 0, PIdx, NULL);

      PIdx = testpidx;
      if (No_Sol)
        for (i = 0; i < 81; i++) G[PIdx+1+j].Grid[i] = 0;
      else
        for (i = 0; i < 81; i++)
          if (!G[PIdx+1+j].Grid[i]) G[PIdx+1+j].Grid[i] = SolGrid[i];
      No_Sol = 0; SingleCnt = 0;
    }
    for (j = 1; j < NSet[G[PIdx].Grid[testcell]]; j++)
      for (i = 0; i < 81; i++) G[PIdx+1].Grid[i] |= G[PIdx+1+j].Grid[i];
    for (i = 0; i < 27; i++) ChangedGroup[i] = 0; Changed = 0;
    for (i = 0; i < 81; i++)
      if (G[PIdx].Grid[i] && (G[PIdx].Grid[i] != G[PIdx+1].Grid[i]))
      { G[PIdx].Grid[i] = G[PIdx+1].Grid[i];
        if (B2V[G[PIdx].Grid[i]]) PushSingle(i, G[PIdx].Grid[i]);
        MarkChanged(i);
      }
  }
}




/***************\
**  MakeGuess  **************************************************\
**                                                              **
**    MakeGuess handles the guessing and backtracking used      **
**    when solving puzzles.                                     **
**                                                              **
\****************************************************************/
inline void MakeGuess (void)
{
  static int LastPos = 0;
  int i;
  int x1, x2;
  unsigned int v;
  int Found = 0;
  char mt;

  Changed = 1;
  SingleCnt = 0;

  // Check to see where the next guess should be
  if (No_Sol)
    while (No_Sol)
    { if (PIdx == 0) return;
      G[PIdx-1].Grid[GuessPos[PIdx]] ^= GuessVal[PIdx];
      if (G[PIdx-1].Grid[GuessPos[PIdx]])
      { if (B2V[G[PIdx-1].Grid[GuessPos[PIdx]]])
          PushSingle(GuessPos[PIdx], G[PIdx-1].Grid[GuessPos[PIdx]]);
        No_Sol = 0;
        MarkChanged(GuessPos[PIdx]);
        PIdx--;
        return;
      }
    }

  // Populate the grid for the next guess
  G[PIdx+1] = G[PIdx];
  PIdx++;

  // Find next spot to check
  if (!Found)
  { mt = 99;
    i = LastPos;
    do
    { if ((NSet[G[PIdx].Grid[i]] < mt) && (NSet[G[PIdx].Grid[i]] > 1))
      { GuessPos[PIdx] = i;
        mt = NSet[G[PIdx].Grid[i]]; 
        if (mt == 2) break;  // if 2, then its the best it can get
      }
      if (++i >= 81) i = 0;
    } while (i != LastPos);
    LastPos = i;
  }

  // Mark the next guess in the position
  No_Sol = 1;
  v = G[PIdx].Grid[GuessPos[PIdx]];
  if (v)
  { v = v & -v;
    GuessVal[PIdx] = v;
    PushSingle(GuessPos[PIdx], v);
    Changed = 1;
    No_Sol = 0;
  }
}




/****************\
**  RatePuzzle  *************************************************\
**                                                              **
**    RatePuzzle converts the status counts into a rating for a **
**    given puzzle.  This should be called after the solver has **
**    been called.                                              **
**                                                              **
**    ok, I never got a formula I liked.  So, if someone wants  **
**    to play around with this and suggest something, please    **
**    go ahead.                                                 **
**                                                              **
**    One of the problems is that the soling routines are so    **
**    generic.  I treat X-Wings and Jellyfishes the sames way,  **
**    same for naked pairs and hidden quads.  Makes coming up   **
**    with a rating difficult.                                  **
**                                                              **
\****************************************************************/
int RatePuzzle (void)
{
  // Values available to manipulate into a puzzle are
  //   SCnt,  HCnt, GCnt, LCCnt, SSCnt, FCnt, OneCnt, TwoCnt, GCnt

  return 0;
}




/***************\
**  CatPuzzle  **************************************************\
**                                                              **
**    CatPuzzle returns a difficulty category, rather than a    **
**    complete ratings.  This is based entirely on the most     **
**    difficult solving technique used.                         **
**                                                              **
**    Categories are:                                           **
**      - Invalid : No Solution, or Multiple Solutions          **
**      - Easy    : Naked / Hidden Single, Locked Candidate     **
**      - Medium  : Disjoint Subsets / Fishies                  **
**      - Hard    : 1 Step Commonality                          **
**      - Expert  : 2 Step Commonality                          **
**      - Insane  : so far, none exist                          **
**                                                              **
\****************************************************************/
int CatPuzzle (char *buf)
{
  if (Solver(2, 0x01, 0, 0, buf) != 1) return 0;  // Invalid (No Solution, or mulitples)
  if (Solver(1, 0x3E, 0, 0, buf) == 0) return 5;  // Insane (none exist, that I know of)
  if (TwoCnt)                          return 4;  // Expert (2 Step Commonality)
  if (OneCnt)                          return 3;  // Hard   (1 Step Commonality)
  if (FCnt || SCnt)                    return 2;  // Medium (Disjoint Subsets / Fishies)
                                       return 1;  // Easy   (Naked/Hidden Single / Locked Candidate)
}
