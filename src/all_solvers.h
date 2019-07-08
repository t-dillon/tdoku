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

    SolverFn OtherSolverJCZSolve;
    SolverFn OtherSolverJSolve;
    SolverFn OtherSolverFsss2;
    SolverFn OtherSolverSKBFORCE;
#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
class Solver {
private:
    SolverFn *solve_;
    uint32_t flags_;
    std::string name_;
    bool returns_solution_;
    
public:
    Solver(SolverFn *solver_fn, uint32_t flags, std::string name, bool returns_solution=true)
            : solve_(solver_fn), flags_(flags), name_(std::move(name)),
              returns_solution_(returns_solution) {}

    inline int Solve(const char *input, int limit,
                     char *solution, size_t *num_guesses) const {
        return solve_(input, limit, flags_, solution, num_guesses);
    }

    inline std::string Id() const {
        std::ostringstream os;
        os << name_;
        if (flags_) {
            os << "{0x" << std::hex << flags_ << "}";
        }
        return os.str();
    }

    inline bool ReturnsSolution() {
        return returns_solution_;
    }
};
#endif

#endif //TDOKU_SOLVER_H
