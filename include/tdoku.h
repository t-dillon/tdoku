#ifndef TDOKU_H
#define TDOKU_H

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#else
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
 *   input:
 *      An 81 character string representing a standard sudoku puzzle, or a 729 character string
 *      representing a pencilmark sudoku puzzle. For standard sudoku for row 0..8 and col 0..8,
 *      the value at input[row * 9 + col] should be a digit '1'..'9' for a given clue or '.' for
 *      an empty cell. For pencilmark sudoku for row 0..8, col 0..8, and digit 1..9, the value
 *      at input[row * 81 + col * 9 + digit - 1] should be '.' if the digit is eliminated or else
 *      (char)('0' + digit). No input validation is done. Do not pass '0' instead of '.' for
 *      unknown cells or eliminated digits. The solver decides if it is dealing with a standard
 *      sudoku or pencilmark sudoku by checking whether input[81] is >= '.' (vs. newline or null).
 *   limit:
 *       The maximum number of solutions to find before returning.
 *   configuration:
 *       Solver-specific configuration. Unused for tdoku.
 *   solution:
 *       Pointer to an 81 character array to receive the solution. Tdoku will only return
 *       a solution if it was given a limit of 1. Otherwise it's assumed we're just interested
 *       in solution counts (e.g., 0, 1, 2+).
 *   num_guesses:
 *       Out parameter to receive the number of guesses performed during search.
 * Returns:
 *   the number of solutions found up to the given limit.
 */
size_t SolveSudoku(const char *input, size_t limit, uint32_t configuration,
                   char *solution, size_t *num_guesses) {
    *num_guesses = 0;
    return TdokuSolverDpllTriadSimd(input, limit, configuration, solution, num_guesses);
}

#endif //TDOKU_H
