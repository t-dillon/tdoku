#include "all_solvers.h"
#include "build_info.h"
#include "klib/ketopt.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <random>
#include <cstdlib>
#include <vector>

using namespace std;
using chrono::system_clock;
using chrono::microseconds;
using chrono::duration_cast;

namespace {

struct Benchmark {
    size_t max_puzzles_to_load_ = 2500000;
    size_t test_dataset_size_ = 1000000;
    int min_seconds_test_ = 20;
    int min_seconds_warmup_ = 10;
    string solvers_ =
            "tdoku_basic:0,tdoku_basic:1,tdoku_dpll_triad_scc:0,"
            "tdoku_dpll_triad_scc:1,tdoku_dpll_triad_scc:2,tdoku_dpll_triad_scc:3,"
            "tdoku_dpll_triad_simd";
    bool csv_output_ = false;
    // when randomize_ is true we'll randomly sample loaded puzzles in constructing the
    // test dataset AND we'll randomly permute those puzzles. this is desirable to avoid
    // bias and test data dependence when comparing between solvers or when comparing
    // different heuristics for the same solver. if you are running benchmarks to optimize
    // a given solver and you are not changing heuristics or ordering, then it's OK to
    // turn off randomization to get lower benchmark variance.
    bool randomize_ = true;
    // whether to validate puzzle solutions during warmup. we don't validate results during
    // actual benchmarking.
    bool validate_ = true;
    // when validating puzzles during warmup it is an error if the solver can not find a
    // solution, UNLESS the dataset indicates that it contains puzzles with no solutions
    // via a comment at the top of the file containing the string 'ALLOWZERO'.
    bool allow_zero_ = false;

    uint64_t seed = std::chrono::system_clock::now().time_since_epoch().count();
    mt19937 rng{seed};
    uniform_int_distribution<uint32_t> random_uint;

    vector<Solver> solvers;
    vector<string> testing_data_;

    static void PrintSudoku(const char *board, bool one_line, ostream &stream) {
        for (int row = 0; row < 9; row++) {
            for (int col = 0; col < 9; col++) {
                stream << board[row * 9 + col];
            }
            if (!one_line) stream << endl;
        }
    }

    static void PrintSudoku(const char *board, bool one_line) {
        PrintSudoku(board, one_line, cout);
    }

    static void BlockShuffle(vector<int> *vec) {
        static std::random_device rd;
        vector<int> blocks{0, 1, 2};
        shuffle(blocks.begin(), blocks.end(), rd);
        for (int i = 0; i < 3; i++) {
            vector<int> block{0, 1, 2};
            shuffle(block.begin(), block.end(), rd);
            for (int j = 0; j < 3; j++) {
                (*vec)[i * 3 + j] = blocks[i] * 3 + block[j];
            }
        }
    }
    // permute rows, columns, bands, and digits to produce a randomly transformed but
    // equivalent puzzle.
    static string PermuteSudoku(const string &input) {
        static std::random_device rd;
        string digit_permutation = "123456789";
        vector<int> row_permutation{0, 1, 2, 3, 4, 5, 6, 7, 8};
        vector<int> col_permutation{0, 1, 2, 3, 4, 5, 6, 7, 8};
        shuffle(digit_permutation.begin(), digit_permutation.end(), rd);
        BlockShuffle(&col_permutation);
        BlockShuffle(&row_permutation);

        char output[81];
        vector<char> out_row;
        out_row.resize(9);
        for (int r = 0; r < 9; r++) {
            vector<char> in_row(input.begin() + r * 9, input.begin() + (r + 1) * 9);
            for (int i = 0; i < 9; i++) {
                out_row[i] = in_row[col_permutation[i]];
                if (out_row[i] != '.') out_row[i] = digit_permutation[out_row[i] - '1'];
            }
            copy(out_row.begin(), out_row.end(), begin(output) + row_permutation[r] * 9);
        }
        return string(output);
    }

    void Load(const string &dataset_filename) {
        allow_zero_ = false;
        vector<string> puzzles;
        puzzles.reserve(max_puzzles_to_load_);

        ifstream file;
        file.open(dataset_filename);
        if (file.fail()) {
            cout << "Error opening " << dataset_filename << endl;
            exit(1);
        }
        string line;
        while (getline(file, line)) {
            if (line.length() > 0 && line[0] != '#') {
                if (max_puzzles_to_load_-- == 0) {
                    break;
                }
                if (line[line.size() - 1] == '\r') {
                    line.erase(line.size() - 1);
                }
                if (line.length() == 81) {
                    puzzles.push_back(line);
                }
            } else if (line.find("ALLOWZERO") != string::npos) {
                allow_zero_ = true;
            }
        }
        file.close();

        testing_data_.clear();
        testing_data_.reserve(test_dataset_size_);
        for (int i = 0; i < test_dataset_size_; i++) {
            if (randomize_) {
                testing_data_.push_back(PermuteSudoku(puzzles[random_uint(rng) % puzzles.size()]));
            } else {
                testing_data_.push_back(puzzles[i % puzzles.size()]);
            }
        }
    }

    void InitSolvers() {
        stringstream ss(solvers_);
        string solver_with_options;

        while (getline(ss, solver_with_options, ',')) {
            string solver = solver_with_options;
            uint32_t configuration = 0;
            size_t pos = solver_with_options.find(':');
            if (pos != string::npos) {
                solver = solver_with_options.substr(0, pos);
                configuration = (uint32_t) stoi(solver_with_options.substr(pos + 1));
            }
            if (solver == "tdoku_basic") {
                solvers.emplace_back(
                        Solver(TdokuSolverBasic, configuration, "tdoku_basic"));
            } else if (solver == "tdoku_dpll_triad_scc") {
                solvers.emplace_back(
                        Solver(TdokuSolverDpllTriadScc, configuration, "tdoku_dpll_triad_scc"));
            } else if (solver == "tdoku_dpll_triad_simd" || solver == "tdoku") {
                solvers.emplace_back(
                        Solver(TdokuSolverDpllTriadSimd, configuration, "tdoku_dpll_triad_simd"));
#ifdef BB_SUDOKU
            } else if (solver == "bb_sudoku") {
                solvers.emplace_back(
                        Solver(OtherSolverBBSudoku, configuration, "bb_sudoku"));
#endif
#ifdef JSOLVE
            } else if (solver == "jsolve") {
                solvers.emplace_back(
                        Solver(OtherSolverJSolve, configuration, "jsolve"));
#endif
#ifdef KUDOKU
            } else if (solver == "kudoku") {
                solvers.emplace_back(
                        Solver(OtherSolverKudoku, configuration, "kudoku", 11));
#endif
#ifdef FSSS2
            } else if (solver == "fsss2") {
                solvers.emplace_back(
                        Solver(OtherSolverFsss2, configuration, "fsss2", 2));
#endif
#ifdef JCZSOLVE
            } else if (solver == "jczsolve") {
                solvers.emplace_back(
                        Solver(OtherSolverJCZSolve, configuration, "jczsolve"));
#endif
#ifdef RUST_SUDOKU
                } else if (solver == "rust_sudoku") {
                solvers.emplace_back(
                        Solver(OtherSolverRustSudoku, configuration, "rust_sudoku", 14));
#endif
#ifdef SK_BFORCE2
            } else if (solver == "sk_bforce2") {
                solvers.emplace_back(
                        Solver(OtherSolverSKBFORCE2, configuration, "sk_bforce2", 2));
#endif
#ifdef MINISAT
            } else if (solver == "minisat") {
                solvers.emplace_back(
                        Solver(TdokuSolverMiniSat, configuration, "minisat", 1));
#endif
            }
        }
    }

    static bool ValidateSolution(const char *board) {
        array<uint32_t, 9 * 3> covered{};
        for (int i = 0; i < 9; i++) {
            for (int j = 0; j < 9; j++) {
                auto bit = 1u << (uint32_t) (board[i * 9 + j] - '1');
                covered[i] ^= bit;
                covered[9 + j] ^= bit;
                covered[18 + 3 * (i / 3) + (j / 3)] ^= bit;
            }
        }
        for (auto x : covered) {
            if (x != 0x1ff) return false;
        }
        return true;
    }

    // we'll preload and permute the puzzles in each dataset before running each solver against
    // it. for each solver we'll run for a warmup period before measurement both to warm caches,
    // branch prediction, etc., and to estimate runtime. there's a lot of variance in runtime.
    // for the fast solvers we want to do passes over the full data set so the results are most
    // comparable. but for the slow solvers we'll run over just a fraction of the dataset if
    // necessary to finish in a reasonable time.
    int Test(const string &dataset_filename) {
        Load(dataset_filename);

        if (!csv_output_) {
            cout << endl << "|" << left << setw(37) << dataset_filename;
            cout << " |  puzzles/sec|  usec/puzzle|   %no_guess|  guesses/puzzle|" << endl;
            cout << "|--------------------------------------|------------:|"
                    " -----------:| ----------:| --------------:|";
            cout << endl;
        }
        char output[81]{0};
        size_t num_guesses;

        for (Solver &solver : solvers) {
            // warm caches, branch predictor, etc. and estimate solving speed on this data
            int n = 0;
            microseconds start =
                    duration_cast<microseconds>(system_clock::now().time_since_epoch());
            microseconds end = start;
            while ((end - start).count() < min_seconds_warmup_ * 1000000) {
                string &puzzle = testing_data_[n % test_dataset_size_];
                output[0] = '.'; // make sure we won't validate a previous solution
                size_t count = solver.Solve(puzzle.c_str(), 1, output, &num_guesses);
                if ((!allow_zero_ && !count) ||
                    (!allow_zero_ && validate_ &&
                     solver.ReturnsSolution() && !ValidateSolution(output))) {
                    cout << "Error during warmup" << endl;
                    PrintSudoku(puzzle.c_str(), false);
                    exit(1);
                }
                n++;
                end = duration_cast<microseconds>(system_clock::now().time_since_epoch());
            }

            double puzzles_per_second = 1000000.0 * n / (end - start).count();
            double est_seconds_full_pass = test_dataset_size_ / puzzles_per_second;

            size_t puzzles_todo;
            if (est_seconds_full_pass < min_seconds_test_ * 2) {
                // for the fast solvers we want to do the lowest multiple of the entire data set
                // that will exceed the min time budget and not exceed the max budget.
                // results are most comparable
                puzzles_todo = test_dataset_size_ *
                               (size_t) ceil(min_seconds_test_ / est_seconds_full_pass);
            } else {
                // for the slow solvers we won't require a full pass over the data set. we'll
                // just aim to hit the max time budget.
                puzzles_todo = (size_t) (test_dataset_size_ * 2 * min_seconds_test_ /
                                         est_seconds_full_pass);
            }

            long num_no_guess = 0;
            long total_guesses = 0;
            start = duration_cast<microseconds>(system_clock::now().time_since_epoch());

            for (int i = 0; i < puzzles_todo; i++) {
                string &puzzle = testing_data_[i % test_dataset_size_];
                size_t count = solver.Solve(puzzle.c_str(), 2, output, &num_guesses);
                if (!allow_zero_ && !count) {
                    cout << "Error during benchmark" << endl;
                    PrintSudoku(puzzle.c_str(), false);
                    exit(1);
                }
                total_guesses += num_guesses;
                num_no_guess += (num_guesses == 0);
            }

            end = duration_cast<microseconds>(system_clock::now().time_since_epoch());

            double usec_total = (end - start).count();
            double seconds = usec_total / 1000000.0;
            double us_per_puzzle = usec_total / puzzles_todo;
            double bt_per_puzzle = total_guesses / (double) puzzles_todo;

#ifdef _WIN32
#define COMMAS ""
#else
#define COMMAS "'"
            setlocale(LC_NUMERIC, "");
#endif
            if (csv_output_) {
                cout << "tdokubench,"
                     << CXX_COMPILER_ID << "," << CXX_COMPILER_VERSION << "," << CXX_FLAGS << ","
                     << dataset_filename << "," << solver.Id() << ","
                     << test_dataset_size_ << "," << puzzles_todo << "," << seconds << ","
                     << puzzles_todo / seconds << "," << us_per_puzzle << ","
                     << 100.0 * num_no_guess / puzzles_todo << "," << bt_per_puzzle << endl;
            } else {
                cout << "|" << left << setw(38) << solver.Id();
                printf("| %" COMMAS "11.1f |", puzzles_todo / seconds);
                printf("%" COMMAS "12.1f |", us_per_puzzle);
                if (solver.ReturnsGuessCount()) {
                    printf("%10.1f%% |", 100.0 * num_no_guess / puzzles_todo);
                    printf("%" COMMAS "15.2f |", bt_per_puzzle);
                } else {
                    printf("%11s |", "N/A");
                    printf("%15s |", "N/A");
                }
                cout << endl;
            }
        }
        return 0;
    }
};

} // namespace

int main(int argc, char **argv) {
    Benchmark benchmark;

    // order the solver list roughly in chronological order of development
#ifdef MINISAT
    benchmark.solvers_.insert(0, "minisat,");
#endif
#ifdef SK_BFORCE2
    benchmark.solvers_.insert(0, "sk_bforce2,");
#endif
#ifdef RUST_SUDOKU
    benchmark.solvers_.insert(0, "rust_sudoku,");
#endif
#ifdef JCZSOLVE
    benchmark.solvers_.insert(0, "jczsolve,");
#endif
#ifdef FSSS2
    benchmark.solvers_.insert(0, "fsss2,fsss2:1,");
#endif
#ifdef KUDOKU
    benchmark.solvers_.insert(0, "kudoku,");
#endif
#ifdef JSOLVE
    benchmark.solvers_.insert(0, "jsolve,");
#endif
#ifdef BB_SUDOKU
    benchmark.solvers_.insert(0, "bb_sudoku,");
#endif

    ketopt_t opt = KETOPT_INIT;
    char c;
    while ((c = ketopt(&opt, argc, argv, 1, "c::hn:r::s:t:v::w:z::", nullptr)) != -1) {
        switch (c) {
            case 'c': {
                benchmark.csv_output_ = opt.arg == nullptr ? true : stoi(opt.arg) > 0;
                break;
            }
            case 'n': {
                benchmark.test_dataset_size_ = (size_t) stoi(opt.arg);
                break;
            }
            case 'r': {
                benchmark.randomize_ = opt.arg == nullptr ? true : stoi(opt.arg) > 0;
                break;
            }
            case 's': {
                benchmark.solvers_ = opt.arg;
                break;
            }
            case 't': {
                benchmark.min_seconds_test_ = stoi(opt.arg);
                break;
            }
            case 'v': {
                benchmark.validate_ = opt.arg == nullptr ? true : stoi(opt.arg) > 0;
                break;
            }
            case 'w': {
                benchmark.min_seconds_warmup_ = stoi(opt.arg);
                break;
            }
            case 'h':
            default: {
                cout << "usage: run_benchmark <options> puzzle_file_1 [...] " << endl;
                cout << "options:" << endl;
                cout << "  -c [0|1]            // output csv instead of table [default 0]" << endl;
                cout << "  -n <size>           // test set size [default 2500000]" << endl;
                cout << "  -r [0|1]            // randomly permute puzzles [default 1]" << endl;
                cout << "  -s solver_1,...     // which solvers to run [default all]" << endl;
                cout << "  -t <secs>           // target test time [default 20]" << endl;
                cout << "  -v [0|1]            // validate during warmup [default 1]" << endl;
                cout << "  -w <secs>           // target warmup time [default 10]" << endl;
                cout << "solvers: " << endl << benchmark.solvers_ << endl;
                cout << "build info: " << CXX_COMPILER_ID << " " << CXX_COMPILER_VERSION
                     << CXX_FLAGS << endl;
                exit(0);
            }
        }
    }

    benchmark.InitSolvers();

    if (opt.ind == argc) {
        benchmark.Test("data/puzzles4_forum_hardest_1905_11+");
    } else {
        for (int i = opt.ind; i < argc; i++) {
            benchmark.Test(argv[i]);
        }
    }
}
