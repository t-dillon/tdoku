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

typedef size_t SolverFn(const char *input, size_t limit, uint32_t flags,
                        char *solution, size_t *num_guesses);

#ifdef __cplusplus
extern "C" {
#endif
    SolverFn TdokuSolverBasic;
    SolverFn TdokuSolverDpllTriadScc;
    SolverFn TdokuSolverDpllTriadSimd;
    SolverFn TdokuSolverMiniSat;

    SolverFn OtherSolverBBSudoku;
    SolverFn OtherSolverFastSolv9r2;
    SolverFn OtherSolverNorvig;
    SolverFn OtherSolverJSolve;
    SolverFn OtherSolverKudoku;
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

    solvers.emplace_back(Solver(TdokuSolverBasic,             0, "_tdev_basic",              15));
    solvers.emplace_back(Solver(TdokuSolverBasic,             1, "_tdev_basic_heuristic",    15));
#ifdef MINISAT
    solvers.emplace_back(Solver(TdokuSolverMiniSat,           0, "minisat_minimal_01",        9));
    solvers.emplace_back(Solver(TdokuSolverMiniSat,           1, "minisat_natural_01",        9));
    solvers.emplace_back(Solver(TdokuSolverMiniSat,           2, "minisat_complete_01",       9));
    solvers.emplace_back(Solver(TdokuSolverMiniSat,           3, "minisat_augmented_01",      9));
#endif
    solvers.emplace_back(Solver(TdokuSolverDpllTriadScc,      0, "_tdev_dpll_triad",         15));
    solvers.emplace_back(Solver(TdokuSolverDpllTriadScc,      1, "_tdev_dpll_triad_scc_i",   15));
    solvers.emplace_back(Solver(TdokuSolverDpllTriadScc,      2, "_tdev_dpll_triad_scc_h",   15));
    solvers.emplace_back(Solver(TdokuSolverDpllTriadScc,      3, "_tdev_dpll_triad_scc_ih",  15));

#ifdef NORVIG
    solvers.emplace_back(Solver(OtherSolverNorvig,            0, "norvig",                   15));
#endif
#ifdef FAST_SOLV_9R2
    solvers.emplace_back(Solver(OtherSolverFastSolv9r2,       0, "fast_solv_9r2",            14));
#endif
#ifdef KUDOKU
    solvers.emplace_back(Solver(OtherSolverKudoku,            0, "kudoku",                    7));
#endif
#ifdef BB_SUDOKU
    solvers.emplace_back(Solver(OtherSolverBBSudoku,          0, "bb_sudoku",                15));
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
