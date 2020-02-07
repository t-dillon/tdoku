#ifndef TDOKU_H
#define TDOKU_H

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>
#else
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
size_t TdokuSolverDpllTriadSimd(const char *input,
                                size_t limit,
                                uint32_t flags,
                                char *solution,
                                size_t *num_guesses);

bool TdokuGeneratorDpllTriadSimd(bool pencilmark,
                                 const int *permutation,
                                 char *puzzle);
#ifdef __cplusplus
}
#endif

/**
 * Solves a Sudoku or Pencilmark Sudoku puzzle.
 * @param input
 *      An 81 character string representing a standard sudoku puzzle, or a 729 character string
 *      representing a pencilmark sudoku puzzle. For standard sudoku for row 0..8 and col 0..8,
 *      the value at input[row * 9 + col] should be a digit '1'..'9' for a given clue or '.' for
 *      an empty cell. For pencilmark sudoku for row 0..8, col 0..8, and digit 1..9, the value
 *      at input[row * 81 + col * 9 + digit - 1] should be '.' if the digit is eliminated or else
 *      (char)('0' + digit). No input validation is done. Do not pass '0' instead of '.' for
 *      unknown cells or eliminated digits. The solver decides if it is dealing with a standard
 *      sudoku or pencilmark sudoku by checking whether input[81] is >= '.' (vs. newline or null).
 * @param limit
 *       The maximum number of solutions to find before returning.
 * @param configuration
 *       Solver-specific configuration. Unused for tdoku.
 * @param solution
 *       Pointer to an 81 character array to receive the solution. Tdoku will only return
 *       a solution if it was given a limit of 1. Otherwise it's assumed we're just interested
 *       in solution counts (e.g., 0, 1, 2+).
 * @param num_guesses
 *       Out parameter to receive the number of guesses performed during search.
 * @return
 *       The number of solutions found up to the given limit.
 */
size_t SolveSudoku(const char *input, size_t limit, uint32_t configuration,
                   char *solution, size_t *num_guesses) {
    return TdokuSolverDpllTriadSimd(input, limit, configuration, solution, num_guesses);
}

/**
 * Generates a Sudoku or Pencilmark Sudoku puzzle
 * @param pencilmark
 *       A boolean indicating whether to generate a pencilmark sudoku (vs. a vanilla one)
 * @param permutation
 *       A permutation of range(729) for use in generating the puzzle
 * @param puzzle
 *       An output buffer of 81 or 729 characters depending on requested puzzle type.
 * @return
 *       A boolean indicating if generation was successful.
 */
bool GenerateSudoku(bool pencilmark, const int *permutation, char *puzzle) {
    return TdokuGeneratorDpllTriadSimd(pencilmark, permutation, puzzle);
}

#endif //TDOKU_H
