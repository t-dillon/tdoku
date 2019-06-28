#include <cstddef>
#include <cstdint>

#ifdef JCZSOLVE
#include "../other/JCZSolve.h"
// apply patch JCZSolve.c.diff so JCZSolve uses this extern.
size_t JCZSolve_guesses;

extern "C"
size_t OtherSolverJCZSolve(const char *input, size_t limit, uint32_t /*unused_flags*/,
                           char *solution, size_t *num_guesses) {
    JCZSolve_guesses = 0;
    int count = JCZSolver(input, solution, limit);
    *num_guesses = JCZSolve_guesses;
    return count;
}
#endif

#ifdef JSOLVE
#include "../other/JSolve.h"
// apply patch JSolve.c.diff so JSolve uses this extern
size_t JSolve_guesses;

extern "C"
size_t OtherSolverJSolve(const char *input, size_t limit, uint32_t /*unused_flags*/,
                         char *solution, size_t *num_guesses) {
    JSolve_guesses = 0;
    int count = JSolve(input, solution, limit);
    *num_guesses = JSolve_guesses;
    return count;
}
#endif

#ifdef FSSS2
#include "../other/fsss2.h"
// apply patches fsss2.cc.diff,fsss2.h.diff so fsss2 uses these externs
size_t Fsss2_guesses;
bool do_locked_candidates;

extern "C"
size_t OtherSolverFsss2(const char *input, size_t limit, uint32_t flags,
                        char *solution, size_t *num_guesses) {
    Fsss2_guesses = 0;
    do_locked_candidates = flags > 0;
    char zero_based_input[81];
    char zero_based_output[81];
    for (int i = 0; i < 9; i++) {
        for (int j = 0; j < 9; j++) {
            char c = input[i * 9 + j];
            if (c >= '1' && c <= '9') {
                zero_based_input[i * 9 + j] = c - '0';
            } else {
                zero_based_input[i * 9 + j] = 0;
            }
        }
    }
    int count;
    if (limit == 1) {
        hasAnySolution has{};
        count = has.solve(zero_based_input);
    } else {
        getSingleSolution gss{};
        count = gss.solve(zero_based_input, zero_based_output);
        for (int i = 0; i < 81; i++) solution[i] = '0' + zero_based_output[i];
    }
    *num_guesses = Fsss2_guesses;
    return count;
}
#endif
