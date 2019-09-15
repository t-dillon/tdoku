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

void Run(const string &testdata_filename, const Solver &solver, bool verbose) {
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

        bool this_fail = count != expect;
        if (this_fail || verbose) {
            cout << (this_fail ? "FAIL: " : "") << solver.Id() << "\n"
                 << "      puzzle:   " << puzzle << "\n"
                 << "      expected: " << expect << "\n"
                 << "      observed: " << count << endl;
        }
        if (!this_fail && expect_str == "1" && solver.ReturnsSolution()) {
            getline(ss, solution, ':');
            solver.Solve(puzzle.c_str(), 1, output, &backtracks);
            this_fail = strncmp(solution.c_str(), output, 81) != 0;
            if (this_fail || verbose) {
                cout << (this_fail ? "FAIL: " : "") << solver.Id() << "\n"
                     << "      puzzle:   " << puzzle << "\n"
                     << "      expected: " << solution << "\n"
                     << "      observed: " << output << endl;
            }
        }
        fail |= this_fail;
    }
    file.close();
    if (!fail) cout << "PASS: " << solver.Id() << endl;
}

int main(int argc, char **argv) {
    bool verbose = false;
    string testdata_filename = "test/test_puzzles";

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-v") == 0) {
            verbose = true;
        } else {
            testdata_filename = argv[1];
        }
    }

    auto solvers = GetAllSolvers();
    for (auto &solver : solvers) {
        Run(testdata_filename, solver, verbose);
    }
}
