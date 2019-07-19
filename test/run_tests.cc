#include "../src/all_solvers.h"
#include "../src/bitutil.h"

#include <chrono>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <vector>

using namespace std;

void Run(const string &testdata_filename, const Solver &solver) {
    ifstream file;
    file.open(testdata_filename);
    if (file.fail()) {
        cout << "Error opening " << testdata_filename << endl;
        exit(1);
    }
    string line;
    bool fail = false;
    while (getline(file, line)) {
        stringstream ss(line);
        string puzzle, expect_str, solution;
        getline(ss, puzzle, ':');
        getline(ss, expect_str, ':');
        int expect = stoi(expect_str);
        if (expect > 0 && !solver.ReturnsCount()) expect = 1;
        if (expect > 1 && !solver.ReturnsFullCount()) expect = 2;

        char output[82]{};
        size_t backtracks;
        size_t count = solver.Solve(puzzle.c_str(), 100000, output, &backtracks);
        if (count != expect) {
            cout << "FAIL: " << solver.Id() << "\n"
                 << "      puzzle:   " << puzzle << "\n"
                 << "      expected: " << expect << "\n"
                 << "      observed: " << count << endl;
            fail = true;
        } else if (expect_str == "1" && solver.ReturnsSolution()) {
            getline(ss, solution, ':');
            solver.Solve(puzzle.c_str(), 1, output, &backtracks);
            if (strncmp(solution.c_str(), output, 81) != 0) {
                cout << "FAIL: " << solver.Id() << "\n"
                     << "      puzzle:   " << puzzle << "\n"
                     << "      expected: " << solution << "\n"
                     << "      observed: " << output << endl;
                fail = true;
            }
        }
    }
    file.close();
    if (!fail) cout << "PASS: " << solver.Id() << endl;
}

vector<Solver> GetSolvers() {
    vector<Solver> solvers;
    // @formatter:off
#ifdef BB_SUDOKU
    solvers.emplace_back(Solver(OtherSolverBBSudoku,          0, "bb_sudoku"));
#endif
#ifdef JSOLVE
    solvers.emplace_back(Solver(OtherSolverJSolve,            0, "jsolve"));
#endif
#ifdef KUDOKU
    solvers.emplace_back(Solver(OtherSolverKudoku,            0, "kudoku", 3));
#endif
#ifdef FSSS2
    solvers.emplace_back(Solver(OtherSolverFsss2,             0, "fsss2", 2));
#endif
#ifdef JCZSOLVE
    solvers.emplace_back(Solver(OtherSolverJCZSolve,          0, "jczsolve"));
#endif
#ifdef SK_BFORCE2
    solvers.emplace_back(Solver(OtherSolverSKBFORCE2,         0, "skbforce", 2));
#endif
#ifdef RUST_SUDOKU
    solvers.emplace_back(Solver(OtherSolverRustSudoku,        0, "rust_sudoku", 6));
#endif
#ifdef MINISAT
    solvers.emplace_back(Solver(TdokuSolverMiniSat,           0, "minisat", 1));
#endif
    solvers.emplace_back(Solver(TdokuSolverBasic,             0, "tdoku_basic"));
    solvers.emplace_back(Solver(TdokuSolverDpllTriadScc,      0, "tdoku_dpll_triad_scc"));
    solvers.emplace_back(Solver(TdokuSolverDpllTriadSimd,     0, "tdoku_dpll_triad_simd"));
    // @formatter:on
    return solvers;
}

int main(int argc, char **argv) {
    string testdata_filename = "test/test_puzzles";
    if (argc > 1) testdata_filename = argv[1];

    auto solvers = GetSolvers();
    for (auto &solver : solvers) {
        Run(testdata_filename, solver);
    }
}
