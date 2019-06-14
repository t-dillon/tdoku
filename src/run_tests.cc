#include "all_solvers.h"
#include "bitutil.h"

#include <chrono>
#include <memory>
#include <sstream>
#include <vector>

using namespace std;
using chrono::system_clock;
using chrono::milliseconds;
using chrono::duration_cast;

void test_satisfiable(const Solver &solver) {
    auto puzzle =
            "..62...9.........51...6.3........7.8.937...5...4..8.....9.7.......5....73...9.84.";
    auto expected =
            "736285194942137685185469372521946738893721456674358219219874563468513927357692841";

    char output[82]{};
    size_t backtracks;

    string input = puzzle;
    int result = solver.Solve(input.c_str(), 1, output, &backtracks);
    string observed(output);

    bool pass = result == 1 && observed == expected;
    cout << (pass ? " pass : " : "[FAIL]: ") << "sat\t";
    cout.width(10); cout << backtracks << "\t  " << solver.Id() << endl;
    if (!pass) {
        cout << "expected: " << expected << endl;
        cout << "observed: " << observed << endl;
    }
}

void test_count(Solver &solver) {
    auto puzzle =
            "12.3.....4.5...6...7.....2.6..1..3....45..........8..9...45.1.........8......2..7";

    char output[82]{};
    size_t backtracks;

    string input = puzzle;
    int result = solver.Solve(input.c_str(), 50000000, output, &backtracks);

    bool pass = result == 190;
    cout << (pass ? " pass : " : "[FAIL]: ") << "satcount\t";
    cout.width(10); cout << result << "\t" << backtracks << "\t  " << solver.Id() << endl;
}

void test_unsatisfiable(Solver &solver) {
    auto puzzle =
            "6.2.5.........3.4..........43...8....1....2........7..5..27...........81...6....8";

    char output[82]{};
    size_t backtracks;

    string input = puzzle;
    int result = solver.Solve(input.c_str(), 1, output, &backtracks);

    bool pass = result == 0;
    cout << (pass ? " pass : " : "[FAIL]: ") << "unsat\t";
    cout.width(10); cout << backtracks << "\t  " << solver.Id() << endl;
}

vector<Solver> get_solvers() {
    vector<Solver> solvers;
    // @formatter:off
    solvers.emplace_back(Solver(TdokuSolverBasic,             0, "tdoku_basic"));
    solvers.emplace_back(Solver(TdokuSolverDpllTriadScc,      0, "tdoku_dpll_triad_scc"));
    solvers.emplace_back(Solver(TdokuSolverDpllTriadSimd,     0, "tdoku_dpll_triad_simd"));
#ifdef FSSS2
    solvers.emplace_back(Solver(OtherSolverFsss2,             0, "fsss2"));
#endif
#ifdef JCZSOLVE
    solvers.emplace_back(Solver(OtherSolverJCZSolve,          0, "jczsolve"));
#endif
#ifdef JSOLVE
    solvers.emplace_back(Solver(OtherSolverJSolve,            0, "jsolve"));
#endif
#ifdef MINISAT
    solvers.emplace_back(Solver(TdokuSolverMiniSat,           0, "minisat"));
#endif
    // @formatter:on
    return solvers;
};

int main() {
    auto solvers = get_solvers();

    for (auto &solver : solvers) {
        test_satisfiable(solver);
    }
    for (auto &solver : solvers) {
        test_count(solver);
    }
    for (auto &solver : solvers) {
        test_unsatisfiable(solver);
    }
}
