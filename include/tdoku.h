#ifndef TDOKU_H
#define TDOKU_H

#ifndef __cplusplus
#include <stddef.h>
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C"
#else
extern
#endif
size_t TdokuSolverDpllTriadSimd(const char *input, size_t limit, uint32_t flags,
                                char *solution, size_t *num_guesses);

/**
 * Solves a Sudoku puzzle.
 * Parameters:
 *   input: a row-major 81 character string with digits for clues and periods for blank cells.
 *   limit: the maximum number of solutions to find before returning.
 *   flags: solver-specific flags. See individual solvers for usage.
 *   solution: pointer to an 81 character array to receive the first solution found.
 *   num_guesses: out parameter to receive the number of guessers performed during search.
 * Returns: the number of solutions found.
 */
size_t SolveSudoku(const char *input, size_t limit, uint32_t flags,
                   char *solution, size_t *num_guesses) {
    *num_guesses = 0;
    return TdokuSolverDpllTriadSimd(input, limit, flags, solution, num_guesses);
};

#endif //TDOKU_H
