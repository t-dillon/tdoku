#include "bitutil.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <memory>
#include <vector>

using namespace std;

namespace {

typedef uint32_t Bits;
  
constexpr Bits kAll = 0x1ff;
typedef tuple<int, int, int> RowColBox;

struct SolverBasic {
    array<Bits, 9> rows_{}, cols_{}, boxes_{};
    vector<RowColBox> cells_todo_;
    size_t limit_ = 1;
    bool min_heuristic_ = false;
    size_t num_todo_ = 0, num_guesses_ = 0, num_solutions_ = 0;

    int NumCandidates(const RowColBox &row_col_box) {
        //int [row, col, box] = cells_todo_[todo_index];
        int row = get<0>(row_col_box);
        int col = get<1>(row_col_box);
        int box = get<2>(row_col_box);
        auto candidates = rows_[row] & cols_[col] & boxes_[box];
        return NumBitsSet(candidates);
    }

    // move a cell with the fewest candidates to the head of the sublist [todo_index:end]
    void MoveBestTodoToFront(size_t todo_index) {
        auto first = cells_todo_.begin() + todo_index;
        auto best = first;
        int best_count = NumCandidates(*best);
        for (auto next = first + 1; best_count > 1 && next < cells_todo_.end(); ++next) {
            int next_count = NumCandidates(*next);
            if (next_count < best_count) {
                best_count = next_count;
                best = next;
            }
        }
        swap(*first, *best);
    }

    // Returns true if a solution is found, updates *solution to reflect assignments
    // made on solution path. Also updates num_guesses_ to reflect the number of
    // guesses made during search.
    void SatisfyGivenPartialAssignment(size_t todo_index, char *solution) {
        if (min_heuristic_) MoveBestTodoToFront(todo_index);

        //int [row, col, box] = cells_todo_[todo_index];
        int row = get<0>(cells_todo_[todo_index]);
        int col = get<1>(cells_todo_[todo_index]);
        int box = get<2>(cells_todo_[todo_index]);

        auto candidates = rows_[row] & cols_[col] & boxes_[box];
        while (candidates) {
            uint32_t candidate = GetLowBit(candidates);

            // only count assignment as a guess if there's more than one candidate.
            if (candidates ^ candidate) num_guesses_++;

            // clear the candidate from available candidate sets for row, col, box
            rows_[row] ^= candidate;
            cols_[col] ^= candidate;
            boxes_[box] ^= candidate;

            solution[row * 9 + col] = (char) ('1' + LowOrderBitIndex(candidate));

            // recursively solve remaining cells and back out with the last solution.
            if (todo_index < num_todo_) {
                SatisfyGivenPartialAssignment(todo_index + 1, solution);
            } else {
                ++num_solutions_;
            }

            if (num_solutions_ == limit_) return;

            // restore the candidate to available candidate sets for row, col, box
            rows_[row] ^= candidate;
            cols_[col] ^= candidate;
            boxes_[box] ^= candidate;

            candidates = ClearLowBit(candidates);
        }
    }

    const int boxen[81] = {0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2, 0, 0, 0, 1, 1, 1, 2, 2, 2,
                           3, 3, 3, 4, 4, 4, 5, 5, 5, 3, 3, 3, 4, 4, 4, 5, 5, 5, 3, 3, 3, 4, 4, 4, 5, 5, 5,
                           6, 6, 6, 7, 7, 7, 8, 8, 8, 6, 6, 6, 7, 7, 7, 8, 8, 8, 6, 6, 6, 7, 7, 7, 8, 8, 8};
  
    bool Initialize(const char *input, size_t limit, uint32_t configuration, char *solution) {
        rows_.fill(kAll);
        cols_.fill(kAll);
        boxes_.fill(kAll);
        limit_ = limit;
        min_heuristic_ = configuration > 0;
        num_guesses_ = 0;
        num_solutions_ = 0;

        // copy initial clues to solution since our todo list won't include these cells
        memcpy(solution, input, 81);
        cells_todo_.clear();

        for (int row = 0; row < 9; ++row) {
            for (int col = 0; col < 9; ++col) {
                int cell = row * 9 + col;
                int box = boxen[cell];
                if (input[row * 9 + col] == '.') {
                    // blank cell: add to the todo list
                    cells_todo_.emplace_back(make_tuple(row, col, box));
                } else {
                    // a given clue: clear availability bits for row, col, and box
                    uint32_t value = 1u << (uint32_t) (input[cell] - '1');
                    if (rows_[row] & value && cols_[col] & value && boxes_[box] & value) {
                        rows_[row] ^= value;
                        cols_[col] ^= value;
                        boxes_[box] ^= value;
                    } else {
                        return false;
                    }
                }
            }
        }
        num_todo_ = cells_todo_.size() - 1;
        return true;
    }
};

}  // namespace


extern "C"
size_t TdokuSolverBasic(const char *input, size_t limit, uint32_t configuration,
                        char *solution, size_t *num_guesses) {
    static SolverBasic solver;
    if (solver.Initialize(input, limit, configuration, solution)) {
        solver.SatisfyGivenPartialAssignment(0, solution);
        *num_guesses = solver.num_guesses_;
        return solver.num_solutions_;
    } else {
        *num_guesses = 0;
        return 0;
    }
}
