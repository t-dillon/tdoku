#ifndef TDOKU_SOLVER_H
#define TDOKU_SOLVER_H

#ifdef __cplusplus
#include <iostream>
#include <sstream>
#include <string>
#else
#include <stddef.h>
#include <stdint.h>
#endif

//////////////////////////////////////////////////
// Solvers
//////////////////////////////////////////////////

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
 *       Solver-specific configuration.
 *   solution:
 *       Pointer to an 81 character array to receive the solution, if return solutions is
 *       supported by the solver.
 *   num_guesses:
 *       Out parameter to receive the number of guesses performed during search, if guess
 *       counting is supported by the solver.
 * Returns:
 *   the number of solutions found up to the given limit.
 */
typedef size_t SolverFn(const char *input, size_t limit, uint32_t flags,
                        char *solution, size_t *num_guesses);

#ifdef __cplusplus
extern "C" {
#endif
    SolverFn TdokuSolverBasic;
    SolverFn TdokuSolverDpllTriadScc;
    SolverFn TdokuSolverDpllTriadSimd;

    SolverFn OtherSolverGurobi;
    SolverFn OtherSolverMiniSat;
    SolverFn OtherSolverFastSolv9r2;
    SolverFn OtherSolverNorvig;
    SolverFn OtherSolverKudoku;
    SolverFn OtherSolverGss;
    SolverFn OtherSolverZeroDoku;
    SolverFn OtherSolverBBSudoku;
    SolverFn OtherSolverFsss;
    SolverFn OtherSolverJSolve;
    SolverFn OtherSolverFsss2;
    SolverFn OtherSolverJCZSolve;
    SolverFn OtherSolverSKBFORCE2;
    SolverFn OtherSolverRustSudoku;
    SolverFn OtherSolverLHLSudoku;
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
#include <vector>

class Solver {
private:
    SolverFn *solve_;
    uint32_t configuration_;
    std::string name_;
    bool returns_solution_;
    bool returns_count_;
    bool returns_full_count_;
    bool returns_guess_count_;

public:
    Solver(SolverFn *solver_fn, uint32_t configuration, std::string name, uint32_t features)
            : solve_(solver_fn), configuration_(configuration), name_(std::move(name)),
              returns_solution_((features & 1u) > 0),
              returns_count_((features & 2u) > 0),
              returns_full_count_((features & 4u) > 0),
              returns_guess_count_((features & 8u) > 0) {}

    inline size_t Solve(const char *input, size_t limit,
                        char *solution, size_t *num_guesses) const {
        return solve_(input, limit, configuration_, solution, num_guesses);
    }

    inline std::string Id() const {
        return name_;
    }

    inline bool ReturnsSolution() const {
        return returns_solution_;
    }

    inline bool ReturnsCount() const {
        return returns_count_;
    }

    inline bool ReturnsFullCount() const {
        return returns_full_count_;
    }

    inline bool ReturnsGuessCount() const {
        return returns_guess_count_;
    }
};

std::vector<Solver> GetAllSolvers() {
    std::vector<Solver> solvers;
    // @formatter:off

#ifdef GUROBI
    solvers.emplace_back(Solver(OtherSolverGurobi,            0, "gurobi",                   15));
#endif
#ifdef MINISAT
    solvers.emplace_back(Solver(OtherSolverMiniSat,           0, "minisat_minimal",          15));
    solvers.emplace_back(Solver(OtherSolverMiniSat,           1, "minisat_natural",          15));
    solvers.emplace_back(Solver(OtherSolverMiniSat,           2, "minisat_complete",         15));
    solvers.emplace_back(Solver(OtherSolverMiniSat,           3, "minisat_augmented",        15));
#endif
#ifdef TDEV
    solvers.emplace_back(Solver(TdokuSolverBasic,             0, "_tdev_basic",              15));
    solvers.emplace_back(Solver(TdokuSolverBasic,             1, "_tdev_basic_heuristic",    15));
    solvers.emplace_back(Solver(TdokuSolverDpllTriadScc,      0, "_tdev_dpll_triad",         15));
    solvers.emplace_back(Solver(TdokuSolverDpllTriadScc,      1, "_tdev_dpll_triad_scc_i",   15));
    solvers.emplace_back(Solver(TdokuSolverDpllTriadScc,      2, "_tdev_dpll_triad_scc_h",   15));
    solvers.emplace_back(Solver(TdokuSolverDpllTriadScc,      3, "_tdev_dpll_triad_scc_ih",  15));
#endif
#ifdef LHL
    solvers.emplace_back(Solver(OtherSolverLHLSudoku,         0, "lhl_sudoku",               14));
#endif
#ifdef NORVIG
    solvers.emplace_back(Solver(OtherSolverNorvig,            0, "norvig",                   15));
#endif
#ifdef FAST_SOLV_9R2
    solvers.emplace_back(Solver(OtherSolverFastSolv9r2,       0, "fast_solv_9r2",            14));
#endif
#ifdef KUDOKU
    solvers.emplace_back(Solver(OtherSolverKudoku,            0, "kudoku",                    7));
#endif
#ifdef GSS
    solvers.emplace_back(Solver(OtherSolverGss,               1, "gss1",                     15));
    solvers.emplace_back(Solver(OtherSolverGss,               2, "gss2",                     15));
    solvers.emplace_back(Solver(OtherSolverGss,               3, "gss3",                     15));
    solvers.emplace_back(Solver(OtherSolverGss,               4, "gss4",                     15));
    solvers.emplace_back(Solver(OtherSolverGss,               5, "gss5",                     15));
    solvers.emplace_back(Solver(OtherSolverGss,               6, "gss6",                     15));
    solvers.emplace_back(Solver(OtherSolverGss,               7, "gss7",                     15));
    solvers.emplace_back(Solver(OtherSolverGss,               8, "gss8",                     15));
    solvers.emplace_back(Solver(OtherSolverGss,               0, "gss",                      15));
#endif
#ifdef ZERODOKU
    solvers.emplace_back(Solver(OtherSolverZeroDoku,          0, "zerodoku",                  6));
#endif
#ifdef BB_SUDOKU
    solvers.emplace_back(Solver(OtherSolverBBSudoku,          0, "bb_sudoku",                15));
#endif
#ifdef FSSS
    solvers.emplace_back(Solver(OtherSolverFsss,              0, "fsss",                     11));
#endif
#ifdef JSOLVE
    solvers.emplace_back(Solver(OtherSolverJSolve,            0, "jsolve",                   15));
#endif
#ifdef FSSS2
    solvers.emplace_back(Solver(OtherSolverFsss2,             0, "fsss2",                    10));
    solvers.emplace_back(Solver(OtherSolverFsss2,             1, "fsss2_locked",             10));
#endif
#ifdef JCZSOLVE
    solvers.emplace_back(Solver(OtherSolverJCZSolve,          0, "jczsolve",                 15));
#endif
#ifdef SK_BFORCE2
    solvers.emplace_back(Solver(OtherSolverSKBFORCE2,         0, "sk_bforce2",               10));
#endif
#ifdef RUST_SUDOKU
    solvers.emplace_back(Solver(OtherSolverRustSudoku,        0, "rust_sudoku",               6));
#endif
    solvers.emplace_back(Solver(TdokuSolverDpllTriadSimd,     0, "tdoku",                    15));
    // @formatter:on
    return solvers;
}
#endif // __cplusplus

#endif // TDOKU_SOLVER_H
