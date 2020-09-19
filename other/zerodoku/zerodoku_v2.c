/*
   zerodoku_v2.c
   Copyright 2008 Jim Schirle
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   For a copy of the GNU General Public License, see
   <http://www.gnu.org/licenses/>.
*/
#include <memory.h>     // memset
#include <stdio.h>      // printf, FILE, fopen, etc
#include <unistd.h>     // getopt
#include <stdint.h>

// modify BOX to be 2, 3, 4, or 5 for different size puzzles
#define BOX 3
#define UNIT (BOX * BOX)
#define GRID (UNIT * UNIT)
#define MASK ((1<<(UNIT+1))-2)
#define GET_POSSIBLE(x) ((~(p.r[s.r[x]]|p.c[s.c[x]]|p.b[s.b[x]])) & MASK)

#define BITCOUNT(x) (__builtin_popcount(x))

// Stats to keep while solving
unsigned int puzlCnt = 0;
unsigned int solnCnt = 0;
unsigned int errCnt = 0;

// Options
unsigned int maxSolutions = 1;

size_t *numGuesses;

// The actual puzzle itself with conflicts for row/col/box
struct Puzzle {
   unsigned int board[GRID];
   unsigned int r[UNIT]; // conflicts
   unsigned int c[UNIT]; // conflicts
   unsigned int b[UNIT]; // conflicts
   int count;
} p;

// quick reference to groups for rows, cols and boxes
unsigned int g[UNIT*3][UNIT];

// possibilities for each square - derived from conflicts
unsigned int possib[GRID];

// used as a quick reference to access the proper conflicts in row/col/box
struct Offsets {
   unsigned int r[GRID];
   unsigned int c[GRID];
   unsigned int b[GRID];
} s;

// try a move in the bitboard - also sets bits in the conflict arrays
void set(int pos, unsigned int val) {
   p.board[pos] = val;
   p.r[s.r[pos]] |= val;
   p.c[s.c[pos]] |= val;
   p.b[s.b[pos]] |= val;
   p.count++;
}

// undo a move - this also clears bits in the conflict arrays
void clear(int pos) {
   int val = ~p.board[pos];
   p.board[pos] = 0;
   p.r[s.r[pos]] &= val;
   p.c[s.c[pos]] &= val;
   p.b[s.b[pos]] &= val;
   p.count--;
}

// Search the board to find naked and hidden singles (or zeros)
int findSingles(int* pos, int* val) {
   int savePos = -1;
   int saveVal = -1;
   int saveCount = UNIT+1;
   int count;
   int i, j, x;
   for (i = 0; i < GRID; i++) {
      if (p.board[i] != 0) {
         possib[i] = 0;
         continue;
      }
      possib[i] = GET_POSSIBLE(i);
      count = BITCOUNT(possib[i]);
      if (count == 0) { // found naked zero - backtrack
         return 0;
      } else if (count == 1) { // found naked single
         *pos = i;
         *val = possib[i];
         return 1;
      } else if (count <= saveCount) { // find lowest number of pencil marks
         savePos = i;
         saveVal = possib[i];
         saveCount = count;
      }
   }
   // search for hidden zeros and singles
   for (i = 0; i < UNIT*3; i++) {
      int once = 0, twice = 0, all = 0;
      for (j = 0; j < UNIT; j++) {
         x = g[i][j];
         all |= (possib[x] | p.board[x]);
         twice |= (once & possib[x]);
         once |= possib[x];
      }
      if (all != MASK) // found hidden 0 - backtrack
         return 0;
      once &= ~twice;
      if (!once)
         continue;
      once &= -once;
      // found a hidden single
      for (j = 0; j < UNIT; j++) { // go back through row and get the single
         if (possib[*pos = g[i][j]] & once) {
            *val = once;
            return 1;
         }
      }
   }
   // give up and return the lowest number of pencil marks in a cell
   *pos = savePos;
   *val = saveVal;
   return (saveCount > UNIT) ? 0 : saveCount;
}

// iterate over the puzzle and find solution(s)
int backTrackSolve() {
   if (p.count == GRID) { // found a solution
      return 1;
   }
   int i;
   int solnsFound = 0;
   int x = -1;
   int val = -1;
   int count = findSingles(&x, &val);
   if (count == 0) // backtrack
      return 0;
   else if (count == 1) { // found a single, so set it and recurse
      set(x, val);
      solnsFound = backTrackSolve();
      clear(x);
      return solnsFound;
   }
   // iterate through the possible moves in this square
   for (i = (val & -val); val; i = (val & -val)) {
      val &= ~i;
      if (val) (*numGuesses)++;
      set(x, i);                        // try this move
      solnsFound += backTrackSolve();   // recurse
      if (solnsFound >= maxSolutions)
         return solnsFound;
      clear(x);                         // undo the move
   }
   return solnsFound;
}

static int uninitialized = 1;

void initializeTables() {
    for (int i = 0; i < GRID; i++) {
        int x = i / UNIT, y = i % UNIT;
        s.r[i] = x;
        s.c[i] = y;
        s.b[i] = x / BOX * BOX + y / BOX;
        g[x][y] = i;
        g[y+UNIT][x] = i;
        g[x+UNIT*2][y] = ((x/BOX*BOX)+(y/BOX))*UNIT+(x%BOX*BOX)+(y%BOX);
    }
    uninitialized = 0;
}

size_t OtherSolverZeroDoku(const char *input, size_t limit, uint32_t unused_configuration,
                           char *solution, size_t *num_guesses) {
    if (uninitialized) {
        initializeTables();
    }
    maxSolutions = limit;
    numGuesses = num_guesses;
    *numGuesses = 0;
    memset(&p, 0, sizeof(p)); // clear for new puzzle
    for (int i = 0; i < 81; i++) {
        if (input[i] != '.') {
            set(i, 1 << (input[i] - '0'));
        }
    }
    return backTrackSolve();
}
