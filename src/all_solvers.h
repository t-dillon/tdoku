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
    SolverFn OtherSolverJSolve;
    SolverFn OtherSolverKudoku;
    SolverFn OtherSolverFsss2;
    SolverFn OtherSolverJCZSolve;
    SolverFn OtherSolverSKBFORCE;
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
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
    Solver(SolverFn *solver_fn, uint32_t configuration, std::string name, uint32_t features=7)
            : solve_(solver_fn), configuration_(configuration), name_(std::move(name)),
              returns_solution_((features & 1u) > 0),
              returns_count_((features & 2u) > 0),
              returns_full_count_((features & 4u) > 0),
              returns_guess_count_((features & 8u) == 0) {}

    inline size_t Solve(const char *input, size_t limit,
                        char *solution, size_t *num_guesses) const {
        return solve_(input, limit, configuration_, solution, num_guesses);
    }

    inline std::string Id() const {
        std::ostringstream os;
        os << name_;
        if (configuration_) {
            os << "{0x" << std::hex << configuration_ << "}";
        }
        return os.str();
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
#endif

#endif //TDOKU_SOLVER_H
