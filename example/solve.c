#include "../include/tdoku.h"
#include <stdio.h>

int main(int argc, const char **argv) {
    char *puzzle = NULL;
    char solution[81];
    size_t size, guesses;

    while (getline(&puzzle, &size, stdin) != -1) {
        int count = SolveSudoku(puzzle, 100000, 0, solution, &guesses);
        printf("%.81s:%d", puzzle, count);
        solution[0] = '\0';
        if (count == 1) {
            SolveSudoku(puzzle, 1, 0, solution, &guesses);
            printf(":%.81s", solution);
        }
        printf("\n");
    }
}
