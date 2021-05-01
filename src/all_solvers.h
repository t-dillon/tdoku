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

    SolverFn OtherSolverGss;
    SolverFn OtherSolverZ3;
    SolverFn OtherSolverGurobi;
    SolverFn OtherSolverMiniSat;
    SolverFn OtherSolverZeroDoku;
    SolverFn OtherSolverFastSolv9r2;
    SolverFn OtherSolverKudoku;
    SolverFn OtherSolverLHLSudoku;
    SolverFn OtherSolverNorvig;
    SolverFn OtherSolverBBSudoku;
    SolverFn OtherSolverFsss;
    SolverFn OtherSolverJSolve;
    SolverFn OtherSolverFsss2;
    SolverFn OtherSolverJCZSolve;
    SolverFn OtherSolverSKBFORCE2;
    SolverFn OtherSolverRustSudoku;
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
    std::string desc_;
    bool returns_solution_;
    bool returns_count_;
    bool returns_full_count_;
    bool returns_guess_count_;

public:
    Solver(SolverFn *solver_fn, uint32_t configuration, std::string name, std::string desc, uint32_t features)
            : solve_(solver_fn), configuration_(configuration), name_(std::move(name)), desc_(std::move(desc)),
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

    inline std::string Desc() const {
        return desc_;
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

#ifdef GSS
    solvers.emplace_back(Solver(OtherSolverGss,               1, "gss1",                      "        ", 15));
    solvers.emplace_back(Solver(OtherSolverGss,               2, "gss2",                      "        ", 15));
    solvers.emplace_back(Solver(OtherSolverGss,               3, "gss3",                      "        ", 15));
    solvers.emplace_back(Solver(OtherSolverGss,               4, "gss4",                      "        ", 15));
    solvers.emplace_back(Solver(OtherSolverGss,               5, "gss5",                      "        ", 15));
    solvers.emplace_back(Solver(OtherSolverGss,               6, "gss6",                      "        ", 15));
    solvers.emplace_back(Solver(OtherSolverGss,               7, "gss7",                      "        ", 15));
    solvers.emplace_back(Solver(OtherSolverGss,               8, "gss8",                      "        ", 15));
    solvers.emplace_back(Solver(OtherSolverGss,               0, "gss",                       "        ", 15));
#endif
#ifdef GUROBI
    solvers.emplace_back(Solver(OtherSolverGurobi,            0, "gurobi",                    "Lsh  xmh", 15));
#endif
#ifdef Z3
    solvers.emplace_back(Solver(OtherSolverZ3,                0, "z3",                        "Lsh  xmh",  1));
#endif
#ifdef MINISAT
    solvers.emplace_back(Solver(OtherSolverMiniSat,           0, "minisat_minimal",           "Ps   xmh", 15));
    solvers.emplace_back(Solver(OtherSolverMiniSat,           1, "minisat_natural",           "Ps   xmh", 15));
    solvers.emplace_back(Solver(OtherSolverMiniSat,           2, "minisat_complete",          "Psh  xmh", 15));
    solvers.emplace_back(Solver(OtherSolverMiniSat,           3, "minisat_augmented",         "Pshrcxmh", 15));
#endif
#ifdef TDEV
    solvers.emplace_back(Solver(TdokuSolverDpllTriadScc,      0, "_tdev_dpll_triad",          "Pshrcxm ", 15));
    solvers.emplace_back(Solver(TdokuSolverDpllTriadScc,      1, "_tdev_dpll_triad_scc_i",    "PshrcXm ", 15));
    solvers.emplace_back(Solver(TdokuSolverDpllTriadScc,      2, "_tdev_dpll_triad_scc_h",    "Pshrcxmh", 15));
    solvers.emplace_back(Solver(TdokuSolverDpllTriadScc,      3, "_tdev_dpll_triad_scc_ih",   "PshrcXmh", 15));
    solvers.emplace_back(Solver(TdokuSolverBasic,             0, "_tdev_basic",               "G       ", 15));
    solvers.emplace_back(Solver(TdokuSolverBasic,             1, "_tdev_basic_heuristic",     "Gs    m ", 15));
#endif
#ifdef ZERODOKU
    solvers.emplace_back(Solver(OtherSolverZeroDoku,          0, "zerodoku",                  "Gsh   m ", 14));
#endif
#ifdef FAST_SOLV_9R2
    solvers.emplace_back(Solver(OtherSolverFastSolv9r2,       0, "fast_solv_9r2",             "Msh   m ", 14));
#endif
#ifdef KUDOKU
    solvers.emplace_back(Solver(OtherSolverKudoku,            0, "kudoku",                    "Msh   m ", 15));
#endif
#ifdef LHL
    solvers.emplace_back(Solver(OtherSolverLHLSudoku,         0, "lhl_sudoku",                "Cs    m ", 14));
#endif
#ifdef NORVIG
    solvers.emplace_back(Solver(OtherSolverNorvig,            0, "norvig",                    "Csh   m ", 15));
#endif
#ifdef BB_SUDOKU
    solvers.emplace_back(Solver(OtherSolverBBSudoku,          0, "bb_sudoku",                 "Cshrc m ", 15));
#endif
#ifdef FSSS
    solvers.emplace_back(Solver(OtherSolverFsss,              0, "fsss",                      "Cshrc m ", 11));
#endif
#ifdef JSOLVE
    solvers.emplace_back(Solver(OtherSolverJSolve,            0, "jsolve",                    "Cshrc m ", 15));
#endif
#ifdef FSSS2
    solvers.emplace_back(Solver(OtherSolverFsss2,             0, "fsss2",                     "Dsh   m ", 10));
    solvers.emplace_back(Solver(OtherSolverFsss2,             1, "fsss2_locked",              "Dshrc m ", 10));
#endif
#ifdef JCZSOLVE
    solvers.emplace_back(Solver(OtherSolverJCZSolve,          0, "jczsolve",                  "Bshr  m ", 15));
#endif
#ifdef SK_BFORCE2
    solvers.emplace_back(Solver(OtherSolverSKBFORCE2,         0, "sk_bforce2",                "Bshr  m ", 10));
#endif
#ifdef RUST_SUDOKU
    solvers.emplace_back(Solver(OtherSolverRustSudoku,        0, "rust_sudoku",               "Bshr  m ", 14));
#endif
    solvers.emplace_back(Solver(TdokuSolverDpllTriadSimd,     0, "tdoku",                     "Xshrcxmh", 15));
    // @formatter:on
    return solvers;
}
#endif // __cplusplus

#endif // TDOKU_SOLVER_H
