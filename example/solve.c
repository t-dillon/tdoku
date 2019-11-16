#include "../include/tdoku.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char **argv) {
    size_t limit = argc > 1 ? atoi(argv[1]) : 10000;

    char *puzzle = NULL;
    char solution[81];
    size_t size, guesses;

    while (getline(&puzzle, &size, stdin) != -1) {
        solution[0] = '\0';
        int count = SolveSudoku(puzzle, limit, 0, solution, &guesses);
        if (limit > 1 && count == 1) {
            SolveSudoku(puzzle, 1, 0, solution, &guesses);
        }
        printf("%.81s:%d:%.81s\n", puzzle, count, solution);
    }
}
