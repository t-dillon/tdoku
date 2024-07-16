#include "../include/tdoku.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef __cplusplus
extern "C" {
#endif


char* solve(char *puzzle) {
    char *result;
    char solution[81];
    size_t size, guesses;
    size_t count = SolveSudoku(puzzle, 2, 0, solution, &guesses);
    if (count == 1) {
      SolveSudoku(puzzle, 1, 0, solution, &guesses);
      sprintf(result, "%.81s", solution);
    }
    return result;
}

#ifdef __cplusplus
}
#endif
