#include <cstddef>
#include <cstdint>
#include <cstring>

#ifdef SK_BFORCE2
#include "SK_BFORCE2/sk_t.h"
#include "SK_BFORCE2/Zhn.h"
#include "SK_BFORCE2/Zhn_cpp.h"

size_t SKBFORCE_guesses;

extern "C"
size_t OtherSolverSKBFORCE2(const char *input, size_t limit, uint32_t /*unused_configuration*/,
                           char *solution, size_t *num_guesses) {
    SKBFORCE_guesses = 0;
    int result = zhou[0].CheckValidityQuick((char*)input);
    *num_guesses = SKBFORCE_guesses;
    return result;
}
#endif

#ifdef JCZSOLVE
#include "JCZSolve/JCZSolve.h"
// apply patch JCZSolve.c.diff so JCZSolve uses this extern.
size_t JCZSolve_guesses;

extern "C"
size_t OtherSolverJCZSolve(const char *input, size_t limit, uint32_t /*unused_configuration*/,
                           char *solution, size_t *num_guesses) {
    JCZSolve_guesses = 0;
    size_t count = JCZSolver(input, solution, limit);
    *num_guesses = JCZSolve_guesses;
    return count;
}
#endif

#ifdef JSOLVE
#include "JSolve/JSolve.h"
// apply patch JSolve.c.diff so JSolve uses this extern
size_t JSolve_guesses;

extern "C"
size_t OtherSolverJSolve(const char *input, size_t limit, uint32_t /*unused_configuration*/,
                         char *solution, size_t *num_guesses) {
    JSolve_guesses = 0;
    int count = JSolve(input, solution, limit);
    *num_guesses = JSolve_guesses;
    return count;
}
#endif

#ifdef FSSS2
#include "fsss2/fsss2.h"
int nTrials;
bool do_locked_candidates;

extern "C"
size_t OtherSolverFsss2(const char *input, size_t limit, uint32_t configuration,
                        char *solution, size_t *num_guesses) {
    nTrials = 0;
    do_locked_candidates = configuration > 0;
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
    }
    *num_guesses = nTrials;
    return count;
}
#endif

#ifdef BB_SUDOKU
extern int Solver(char num_search, unsigned int use_methods, char ret_puzzle,
                  int initp, char* buffer);
extern bool InitTables();
extern int GCnt;

extern "C"
size_t OtherSolverBBSudoku(const char *input, size_t limit, uint32_t /*unused_configuration*/,
                           char *solution, size_t *num_guesses) {
    static bool initialized = InitTables();
    GCnt = 0;
    unsigned methods = 0x03; /* guesses & locked candidates */
    memcpy(solution, input, 81); /* BB_SUDOKU uses a single in/out buffer */
    int count = Solver(limit, methods, 1 /* return solution */, 0, solution);
    *num_guesses = GCnt - 1;
    return count;
}
#endif
