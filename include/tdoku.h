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
                                uint32_t configuration,
                                char *solution,
                                size_t *num_guesses);

size_t TdokuEnumerate(const char *puzzle,
                      size_t limit,
                      void (*callback)(const char *));

bool TdokuConstrain(bool pencilmark, char *puzzle);

bool TdokuMinimize(bool pencilmark, bool monotonic, char *puzzle);
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
static inline size_t SolveSudoku(const char *input, size_t limit, uint32_t configuration,
                          char *solution, size_t *num_guesses) {
    return TdokuSolverDpllTriadSimd(input, limit, configuration, solution, num_guesses);
}

static inline size_t Enumerate(const char *puzzle, size_t limit,
                               void (*callback)(const char *)) {
    return TdokuEnumerate(puzzle, limit, callback);
}

/**
 * Given a partially constrained puzzle adds random clues until the solution is unique. This
 * procedure is fast, but biased in the sense that different puzzles may arise with widely
 * varying probabilities and it makes no effort to adjust for these differences or to estimate
 * these probabilities. It also does not guarantee that the resulting puzzle is minimal.
 * @param pencilmark
 *       A boolean indicating if the second argument is a pencilmark sudoku (vs. a vanilla one)
 * @param puzzle
 *       An in/out buffer of 81 or 729 characters containing the input partial puzzle and
 *       output re-constrained puzzle.
 * @return
 *       A boolean indicating success or failure.
 */
static inline bool Constrain(bool pencilmark, char *puzzle) {
    return TdokuConstrain(pencilmark, puzzle);
}

/**
 * Minimizes a vanilla or pencilmark puzzle by testing removal of all clues in random order,
 * restoring any clue that's required to keep the solution unique.
 * @param pencilmark
 *       A boolean indicating whether to minimize a pencilmark sudoku (vs. a vanilla one)
 * @param monotonic
 *       A boolean indicating the minimizer should return true only if we have a minimal
 *       puzzle after the first restored clue.
 * @param puzzle
 *       An 81 or 729 character puzzle to minimize (for vanilla vs. pencilmark)
 */
static inline bool Minimize(bool pencilmark, bool monotonic, char *puzzle) {
    return TdokuMinimize(pencilmark, monotonic, puzzle);
}

#endif //TDOKU_H
