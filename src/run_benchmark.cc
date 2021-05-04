#include "all_solvers.h"
#include "build_info.h"
#include "klib/ketopt.h"
#include "util.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <vector>

using namespace std;
using chrono::steady_clock;
using chrono::microseconds;
using chrono::duration_cast;

namespace {

struct Options {
    // whether to expect 729-character pencilmark input instead of 81-character sudoku.
    bool pencilmark = false;
    // whether to rate puzzles by time (instead of by backtracks)
    bool rate_by_backtracks = false;
    // size of the dataset to create from the input (via randomization, replication, sampling).
    size_t test_dataset_size = 100000;
    // target warmup time for caches, branch prediction before actual benchmarking
    int min_seconds_warmup = 4;
    // target duration of benchmarking run
    int min_seconds_test = 10;
    // whether to randomly sample and permute loaded puzzles in constructing the test dataset.
    // this is desirable to avoid bias and test data dependence when comparing between solvers or
    // when comparing different heuristics for the same solver. you can disable randomization to
    // make sure you're testing puzzles exactly as given, but if your goal is just to reduce
    // benchmark variance it's better to fix the random seed instead (not that much variance is
    // introduced by randomization if your generated test dataset is of reasonable size).
    bool randomize = true;
    // if randomizing, the random seed to use. If the given (or default) random seed is zero
    // then rd() will be used.
    uint64_t random_seed = 0;
    // whether to stop at the first solution vs. validating uniqueness.
    bool first_solution = false;
    // whether to validate puzzle solutions during warmup. we don't validate results during
    // actual benchmarking.
    bool validate = true;
    // whether to output results in csv format instead of markdown table format
    bool csv_output = false;
    // the set of solvers to benchmark
    vector<Solver> solvers{GetAllSolvers()};
};

struct Benchmark {
    const Options options_;
    const size_t puzzle_size_;
    const size_t puzzle_buf_size_; // puzzle_size_ rounded up for alignment
    vector<char> dataset_{};
    // when validating puzzles during warmup it is an error if the solver can not find a
    // solution, UNLESS the dataset indicates that it contains puzzles with no solutions
    // via a comment at the top of the file containing the string 'ALLOWZERO'.
    bool allow_zero_ = false;

    Util util;

    explicit Benchmark(const Options &options) :
            options_(options),
            puzzle_size_(options.pencilmark ? 729 : 81),
            puzzle_buf_size_(options.pencilmark ? 736 : 96) {}

    void PrintSudoku(const char *board, bool one_line, ostream &stream) {
        for (int row = 0; row < 9; row++) {
            for (int col = 0; col < 9; col++) {
                if (options_.pencilmark) {
                    for (int digit = 0; digit < 9; digit++) {
                        stream << board[row * 81 + col * 9 + digit];
                    }
                    stream << "|";
                } else {
                    stream << board[row * 9 + col];
                }
            }
            if (!one_line) stream << endl;
        }
    }

    void PrintSudoku(const char *board, bool one_line) {
        PrintSudoku(board, one_line, cout);
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
        dataset_.resize(options_.test_dataset_size * puzzle_buf_size_);
        fill(dataset_.begin(), dataset_.end(), 0);

        // load input file; if there are more puzzles in the input file than we want in our
        // test dataset then sample such that each input puzzle has the same probability of
        // being in the test dataset.
        string line;
        int num_loaded = 0, num_processed = 0;
        while (getline(file, line)) {
            if (line.length() > 0 && line[0] != '#') {
                if (line[line.size() - 1] == '\r') {
                    line.erase(line.size() - 1);
                }
                if (line.length() >= puzzle_size_) {
                    num_processed++;
                    if (num_loaded < options_.test_dataset_size) {
                        char *dest = &dataset_[puzzle_buf_size_ * num_loaded];
                        strncpy(dest, line.c_str(), puzzle_size_);
                        if (options_.randomize) util.PermuteSudoku(dest, options_.pencilmark);
                        num_loaded++;
                    } else if (util.RandomDouble() <
                               (double) options_.test_dataset_size / num_processed) {
                        auto replace = util.RandomUInt() % options_.test_dataset_size;
                        char *dest = &dataset_[puzzle_buf_size_ * replace];
                        strncpy(dest, line.c_str(), puzzle_size_);
                        if (options_.randomize) util.PermuteSudoku(dest, options_.pencilmark);
                    }
                }
            } else if (line.find("ALLOWZERO") != string::npos) {
                allow_zero_ = true;
            }
        }
        file.close();

        // if we've requested a test dataset at least as large as the input file, then fit as
        // many full copies of the input file as we can.
        if (num_loaded == num_processed) {
            while (num_loaded + num_processed < options_.test_dataset_size) {
                memcpy(&dataset_[puzzle_buf_size_ * num_loaded], &dataset_[0],
                       puzzle_buf_size_ * num_processed);
                if (options_.randomize) {
                    for (int j = 0; j < num_processed; j++) {
                        util.PermuteSudoku(&dataset_[puzzle_buf_size_ * (num_loaded + j)],
                                options_.pencilmark);
                    }
                }
                num_loaded += num_processed;
            }
        }
        // then complete the dataset by sampling from loaded puzzles with uniform probability.
        for (int i = num_loaded; i < options_.test_dataset_size; i++) {
            auto which = util.RandomUInt() % num_loaded;
            strncpy(&dataset_[puzzle_buf_size_ * i],
                    &dataset_[puzzle_buf_size_ * which],
                    puzzle_size_);
            if (options_.randomize) {
                util.PermuteSudoku(&dataset_[puzzle_buf_size_ * i], options_.pencilmark);
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
        return all_of(covered.begin(), covered.end(), [](uint32_t x) { return x == 0x1ff; });
    }

    void ExitError(const char *puzzle, const char *context) {
        cout << "Error during " << context << endl;
        PrintSudoku(puzzle, false);
        exit(1);
    }

    // warm caches, branch predictor, etc. and estimate solving speed on this data.
    double WarmupAndEstimateRate(const Solver &solver) {
        char output[81]{0};
        size_t num_guesses;
        int warmup_count = 0;
        microseconds start = duration_cast<microseconds>(steady_clock::now().time_since_epoch());
        microseconds end = start;
        while ((end - start).count() < options_.min_seconds_warmup * 1000000) {
            // during warmup/rate estimation we'll solve puzzles in random order just in case
            // the data set order is biased (e.g., easy or hard puzzles up front).
            const char *puzzle = &dataset_[puzzle_buf_size_ *
                                           (util.RandomUInt() % options_.test_dataset_size)];
            output[0] = '.'; // make sure we won't validate a previous solution
            size_t count = solver.Solve(puzzle, 1, output, &num_guesses);
            if (!allow_zero_ &&
                (!count || (options_.validate &&
                            solver.ReturnsSolution() && !ValidateSolution(output)))) {
                ExitError(puzzle, "warmup");
            }
            warmup_count++;
            end = duration_cast<microseconds>(steady_clock::now().time_since_epoch());
        }
        double puzzles_per_second = 1000000.0 * warmup_count / (end - start).count();
        return puzzles_per_second;
    }

    void OutputHeader(const string &filename) {
        if (!options_.csv_output) {
            cout << endl << "|" << left << setw(37) << filename << " ";
            cout << "|  puzzles/sec|  usec/puzzle|   %no_guess|  guesses/puzzle|" << endl;
            cout << "|--------------------------------------"
                    "|------------:|------------:|-----------:|---------------:|" << endl;
        }
    }

#ifdef _WIN32
#define COMMAS ""
#else
#define COMMAS "'"
#endif

    void OutputResult(const Solver &solver, const string &dataset_filename,
                      size_t num_solved, double usec_total,
                      size_t total_guesses, size_t total_no_guess) {
        setlocale(LC_NUMERIC, "");
        const char *f1 = "%.0s%.0s%.0s%.0s|%-27s%-11s|"
                         "%" COMMAS "12.1f |%" COMMAS "12.1f |%10.1f%% |%" COMMAS "15.2f |";
        const char *f2 = "%.0s%.0s%.0s%.0s|%-27s%-11s|"
                         "%" COMMAS "12.1f |%" COMMAS "12.1f |        N/A |            N/A |";
        const char *f3 = "%s,%s,%s,%s,%s%.0s,%f,%f,%f,%f";
        const char *f4 = "%s,%s,%s,%s,%s%.0s,%f,%f,N/A,N/A";

        const char *fmt = options_.csv_output ?
                          (solver.ReturnsGuessCount() ? f3 : f4) :
                          (solver.ReturnsGuessCount() ? f1 : f2);

        double puzzles_per_second = 1000000 * num_solved / usec_total;
        double usec_per_puzzle = usec_total / (double) num_solved;
        double percent_no_guess = 100 * total_no_guess / (double) num_solved;
        double guesses_per_puzzle = total_guesses / (double) num_solved;

        char str[1024];
        sprintf(str, fmt,
                CXX_COMPILER_ID, CXX_COMPILER_VERSION, CXX_FLAGS,
                dataset_filename.c_str(), solver.Id().c_str(), solver.Desc().c_str(),
                puzzles_per_second, usec_per_puzzle, percent_no_guess, guesses_per_puzzle);
        cout << str << endl;
    }

    // we'll preload and permute the puzzles in each dataset before running each solver against
    // it. for each solver we'll run for a warmup period before measurement both to warm caches,
    // branch prediction, etc., and to estimate runtime. there's a lot of variance in runtime.
    // for the fast solvers we want to do passes over the full data set so the results are most
    // comparable. but for the slow solvers we'll run over just a fraction of the dataset if
    // necessary to finish in a reasonable time.
    void Test(const string &filename) {
        if (options_.random_seed > 0) {
            util.RandomSeed(options_.random_seed);
        }
        Load(filename);
        OutputHeader(filename);

        // for the slow solvers we'll solve puzzles in this order to avoid any difficulty biases.
        auto perm = util.Permutation(options_.test_dataset_size);

        for (const Solver &solver : options_.solvers) {
            double puzzles_per_second = WarmupAndEstimateRate(solver);
            // we'll use the procedure for fast solvers if we expect to complete a full pass through
            // the data set in less than twice the min test time (or really less than up to 4x the min
            // test time since warmup is run with a limit of 1).
            bool fast = puzzles_per_second * options_.min_seconds_test * 2 > options_.test_dataset_size;

            char puzzle_output[81]{0};
            size_t puzzle_guesses;
            size_t total_guesses = 0;
            size_t total_no_guess = 0;
            size_t total_solved = 0;

            microseconds start = duration_cast<microseconds>(steady_clock::now().time_since_epoch());
            microseconds end = start;

            if (fast) {
                while ((end - start).count() < options_.min_seconds_test * 1000000) {
                    for (int i = 0; i < options_.test_dataset_size; i++) {
                        const char *puzzle = &dataset_[puzzle_buf_size_ * i];
                        size_t solutions = solver.Solve(puzzle, options_.first_solution ? 1 : 2,
                                                        puzzle_output, &puzzle_guesses);
                        if (!allow_zero_ && !solutions) {
                            ExitError(puzzle, "benchmark");
                        }
                        total_guesses += puzzle_guesses;
                        total_no_guess += (puzzle_guesses == 0);
                    }
                    total_solved += options_.test_dataset_size;
                    end = duration_cast<microseconds>(steady_clock::now().time_since_epoch());
                }
            } else {
                while ((end - start).count() < options_.min_seconds_test * 2000000) {
                    const char *puzzle = &dataset_[puzzle_buf_size_ * perm[total_solved % options_.test_dataset_size]];
                    size_t solutions = solver.Solve(puzzle, options_.first_solution ? 1 : 2,
                                                    puzzle_output, &puzzle_guesses);
                    if (!allow_zero_ && !solutions) {
                        ExitError(puzzle, "benchmark");
                    }
                    total_guesses += puzzle_guesses;
                    total_no_guess += (puzzle_guesses == 0);
                    total_solved += 1;
                    end = duration_cast<microseconds>(steady_clock::now().time_since_epoch());
                }
            }

            auto total_usec = (end - start).count();
            OutputResult(solver, filename, total_solved, total_usec, total_guesses, total_no_guess);
        }
    }

    void Rate(const string &dataset_filename) {
        ifstream file;
        file.open(dataset_filename);
        if (file.fail()) {
            cout << "Error opening " << dataset_filename << endl;
            exit(1);
        }
        dataset_.resize(options_.test_dataset_size * puzzle_buf_size_);
        fill(dataset_.begin(), dataset_.end(), 0);

        char solution[81];
        size_t guesses = 0;
        string line;
        while (getline(file, line)) {
            if (line.length() > 0 && line[0] != '#') {
                if (line[line.size() - 1] == '\r') {
                    line.erase(line.size() - 1);
                }
                if (line.length() >= puzzle_size_) {
                    for (int i = 0; i < options_.test_dataset_size; i++) {
                        char *dest = &dataset_[i * puzzle_buf_size_];
                        strncpy(dest, line.c_str(), puzzle_size_);
                        if (options_.randomize) {
                            util.PermuteSudoku(dest, options_.pencilmark);
                        }
                    }
                    for (const Solver &solver : options_.solvers) {
                        microseconds start =
                                duration_cast<microseconds>(steady_clock::now().time_since_epoch());
                        double total_guesses = 0.0;
                        for (int i = 0; i < options_.test_dataset_size; i++) {
                            const char *puzzle = &dataset_[i * puzzle_buf_size_];
                            solver.Solve(puzzle, 1, solution, &guesses);
                            total_guesses += guesses;
                        }
                        microseconds end =
                                duration_cast<microseconds>(steady_clock::now().time_since_epoch());
                        double cost = (options_.rate_by_backtracks ?
                                total_guesses : (double)(end - start).count());
                        printf("%12.1f\t", cost / options_.test_dataset_size);
                    }
                    cout << endl;
                }
            }
        }
    }
};

} // namespace


int main(int argc, char **argv) {
    Options options{};

    bool do_rating = false;
    ketopt_t opt = KETOPT_INIT;
    char c;
    while ((c = (char)ketopt(&opt, argc, argv, 1, "abc::e:fhn:pr::s:t:v::w:z::", nullptr)) != -1) {
        switch (c) {
            case 'a': {
                do_rating = true;
                break;
            }
            case 'b': {
                options.rate_by_backtracks = true;
                break;
            }
            case 'c': {
                options.csv_output = opt.arg == nullptr ? true : stoi(opt.arg) > 0;
                break;
            }
            case 'e': {
                options.random_seed = stoull(opt.arg);
                break;
            }
            case 'f': {
                options.first_solution = true;
                break;
            }
            case 'n': {
                options.test_dataset_size = (size_t) stoi(opt.arg);
                break;
            }
            case 'p': {
                options.pencilmark = true;
                break;
            }
            case 'r': {
                options.randomize = opt.arg == nullptr ? true : stoi(opt.arg) > 0;
                break;
            }
            case 's': {
                options.solvers.clear();
                stringstream ss(opt.arg);
                string solver_id;
                while (getline(ss, solver_id, ',')) {
                    auto all_solvers = GetAllSolvers();
                    for (const Solver &s : all_solvers) {
                        if (s.Id() == solver_id) {
                            options.solvers.push_back(s);
                        }
                    }
                }
                break;
            }
            case 't': {
                options.min_seconds_test = stoi(opt.arg);
                break;
            }
            case 'v': {
                options.validate = opt.arg == nullptr ? true : stoi(opt.arg) > 0;
                break;
            }
            case 'w': {
                options.min_seconds_warmup = stoi(opt.arg);
                break;
            }
            case 'h':
            default: {
                cout << "usage: run_benchmark <options> puzzle_file_1 [...] " << endl;
                cout << "options:" << endl;
                cout << "  -a                  // do rating" << endl;
                cout << "  -b                  // rate by backtracks" << endl;
                cout << "  -c [0|1]            // output csv instead of table [default 0]" << endl;
                cout << "  -e <seed>           // random seed [default random_device{}()]" << endl;
                cout << "  -h                  // display this help message" << endl;
                cout << "  -n <size>           // test set size [default 2500000]" << endl;
                cout << "  -p                  // expect 729 character pencilmark sudoku" << endl;
                cout << "  -r [0|1]            // randomly permute puzzles [default 1]" << endl;
                cout << "  -s solver_1,...     // which solvers to run [default all]" << endl;
                cout << "  -t <secs>           // target test time [default 20]" << endl;
                cout << "  -v [0|1]            // validate during warmup [default 1]" << endl;
                cout << "  -w <secs>           // target warmup time [default 10]" << endl;
                cout << "solvers: " << endl;
                for (auto &solver : GetAllSolvers()) {
                    cout << " " << solver.Id();
                }
                cout << "\nbuild info: "
                     << CXX_COMPILER_ID << " " << CXX_COMPILER_VERSION << CXX_FLAGS << endl;
                exit(0);
            }
        }
    }

    Benchmark benchmark(options);

    if (opt.ind == argc) {
        benchmark.Test("data/puzzles1_unbiased");
    } else {
        for (int i = opt.ind; i < argc; i++) {
            if (do_rating) {
                benchmark.Rate(argv[i]);
            } else {
                benchmark.Test(argv[i]);
            }
        }
    }
}
