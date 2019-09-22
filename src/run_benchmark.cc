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
    size_t test_dataset_size_ = 1000000;
    int min_seconds_test_ = 20;
    int min_seconds_warmup_ = 10;
    bool csv_output_ = false;
    // when randomize_ is true we'll randomly sample loaded puzzles in constructing the
    // test dataset AND we'll randomly permute those puzzles. this is desirable to avoid
    // bias and test data dependence when comparing between solvers or when comparing
    // different heuristics for the same solver. if you are running benchmarks to optimize
    // a given solver and you are not changing heuristics or ordering, then it's OK to
    // turn off randomization to get lower benchmark variance, but it may be better to get
    // lower benchmark variance by setting a fixed seed and keeping randomization on.
    bool randomize_ = true;
    // whether to validate puzzle solutions during warmup. we don't validate results during
    // actual benchmarking.
    bool validate_ = true;
    // when validating puzzles during warmup it is an error if the solver can not find a
    // solution, UNLESS the dataset indicates that it contains puzzles with no solutions
    // via a comment at the top of the file containing the string 'ALLOWZERO'.
    bool allow_zero_ = false;

    random_device rd;
    mt19937_64 rng{rd()};
    uniform_int_distribution<uint32_t> random_uint;
    uniform_real_distribution<> random_double{0.0, 1.0};
    uint64_t rng_seed = 0;

    vector<Solver> solvers_;
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

    void BlockShuffle(vector<int> *vec) {
        vector<int> blocks{0, 1, 2};
        shuffle(blocks.begin(), blocks.end(), rng);
        for (int i = 0; i < 3; i++) {
            vector<int> block{0, 1, 2};
            shuffle(block.begin(), block.end(), rng);
            for (int j = 0; j < 3; j++) {
                (*vec)[i * 3 + j] = blocks[i] * 3 + block[j];
            }
        }
    }
    // permute rows, columns, bands, and digits to produce a randomly transformed but
    // equivalent puzzle.
    string PermuteSudoku(const string &input) {
        string digit_permutation = "123456789";
        vector<int> row_permutation{0, 1, 2, 3, 4, 5, 6, 7, 8};
        vector<int> col_permutation{0, 1, 2, 3, 4, 5, 6, 7, 8};
        shuffle(digit_permutation.begin(), digit_permutation.end(), rng);
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
        return string(output, 81);
    }

    // generate a dataset of the requested size from the input file in a way that maximizes
    // representativeness and minimizes benchmark variance.
    void Load(const string &dataset_filename) {
        ifstream file;
        file.open(dataset_filename);
        if (file.fail()) {
            cout << "Error opening " << dataset_filename << endl;
            exit(1);
        }

        allow_zero_ = false;
        testing_data_.clear();
        testing_data_.reserve(test_dataset_size_);

        // load input file; if there are more puzzles in the input file than we want in our
        // test dataset then sample such that each input puzzle has the same probability of
        // being in the test dataset.
        string line;
        int input_count = 0;
        while (getline(file, line)) {
            if (line.length() > 0 && line[0] != '#') {
                if (line[line.size() - 1] == '\r') {
                    line.erase(line.size() - 1);
                }
                if (line.length() == 81) {
                    input_count++;
                    if (input_count <= test_dataset_size_) {
                        testing_data_.push_back(randomize_ ? PermuteSudoku(line) : line);
                    } else if (random_double(rng) < (double)test_dataset_size_ / input_count) {
                        auto replace = random_uint(rng) % test_dataset_size_;
                        testing_data_[replace] = randomize_ ? PermuteSudoku(line) : line;
                    }
                }
            } else if (line.find("ALLOWZERO") != string::npos) {
                allow_zero_ = true;
            }
        }
        file.close();

        // if we've requested a test dataset larger than the input file, then fit as many full
        // copies of the input file as we can.
        int num_full_copies = test_dataset_size_ / input_count;
        for (int i = 1; i < num_full_copies; i++) {
            for (int j = 0; j < input_count; j++) {
                string puzzle = testing_data_[j];
                testing_data_.push_back(randomize_ ? PermuteSudoku(puzzle) : puzzle);
            }
        }
        // then complete the dataset by sampling from loaded puzzles with uniform probability.
        for (int i = testing_data_.size(); i < test_dataset_size_; i++) {
            string puzzle = testing_data_[random_uint(rng) % input_count];
            testing_data_.push_back(randomize_ ? PermuteSudoku(puzzle) : puzzle);
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
        if (rng_seed > 0) {
            rng.seed(rng_seed);
        }
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

        for (Solver &solver : solvers_) {
            // warm caches, branch predictor, etc. and estimate solving speed on this data
            int warmup_count = 0;
            microseconds start =
                    duration_cast<microseconds>(system_clock::now().time_since_epoch());
            microseconds end = start;
            while ((end - start).count() < min_seconds_warmup_ * 1000000) {
                string &puzzle = testing_data_[warmup_count % test_dataset_size_];
                output[0] = '.'; // make sure we won't validate a previous solution
                size_t count = solver.Solve(puzzle.c_str(), 1, output, &num_guesses);
                if ((!allow_zero_ && !count) ||
                    (!allow_zero_ && validate_ &&
                     solver.ReturnsSolution() && !ValidateSolution(output))) {
                    cout << "Error during warmup" << endl;
                    PrintSudoku(puzzle.c_str(), false);
                    exit(1);
                }
                warmup_count++;
                end = duration_cast<microseconds>(system_clock::now().time_since_epoch());
            }

            double puzzles_per_second = 1000000.0 * warmup_count / (end - start).count();
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
    vector<Solver> all_solvers = GetAllSolvers();
    string solver_list;

    Benchmark benchmark;

    ketopt_t opt = KETOPT_INIT;
    char c;
    while ((c = ketopt(&opt, argc, argv, 1, "c::e:hn:r::s:t:v::w:z::", nullptr)) != -1) {
        switch (c) {
            case 'c': {
                benchmark.csv_output_ = opt.arg == nullptr ? true : stoi(opt.arg) > 0;
                break;
            }
            case 'e': {
                benchmark.rng_seed = stoi(opt.arg);
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
                solver_list = opt.arg;
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
                cout << "  -e <seed>           // random seed [default random_device{}()]" << endl;
                cout << "  -n <size>           // test set size [default 2500000]" << endl;
                cout << "  -r [0|1]            // randomly permute puzzles [default 1]" << endl;
                cout << "  -s solver_1,...     // which solvers to run [default all]" << endl;
                cout << "  -t <secs>           // target test time [default 20]" << endl;
                cout << "  -v [0|1]            // validate during warmup [default 1]" << endl;
                cout << "  -w <secs>           // target warmup time [default 10]" << endl;
                cout << "solvers: " << endl;
                for (auto &solver : all_solvers) {
                    cout << " " << solver.Id();
                }
                cout << "\nbuild info: " << CXX_COMPILER_ID << " " << CXX_COMPILER_VERSION
                     << CXX_FLAGS << endl;
                exit(0);
            }
        }
    }

    if (solver_list.empty()) {
        benchmark.solvers_ = all_solvers;
    } else {
        stringstream ss(solver_list);
        string solver_id;
        while (getline(ss, solver_id, ',')) {
            for (const Solver &s : all_solvers) {
                if (s.Id() == solver_id) {
                    benchmark.solvers_.push_back(s);
                }
            }
        }
    }

    if (opt.ind == argc) {
        benchmark.Test("data/puzzles4_forum_hardest_1905_11+");
    } else {
        for (int i = opt.ind; i < argc; i++) {
            benchmark.Test(argv[i]);
        }
    }
}
