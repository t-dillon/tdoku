#include "z3++.h"
#include <array>
#include <cstring>

namespace {

struct SolverZ3 {
    z3::context c;
    z3::solver s{c};
    z3::expr_vector cell{c};

    SolverZ3() {
        for (int i = 0; i <= 81; i++) {
            std::stringstream idx;
            idx << "c" << i;
            cell.push_back(c.int_const(idx.str().c_str()));
        }
        for (int i = 0; i < 9; i++) {
            z3::expr_vector row{c}, col{c}, box{c};
            for (int j = 0; j < 9; j++) {
                s.add(cell[i * 9 + j] >= 1 && cell[i * 9 + j] <= 9);
                row.push_back(cell[i * 9 + j]);
                col.push_back(cell[j * 9 + i]);
                box.push_back(cell[((i / 3) * 3 + (j / 3)) * 9 + (i % 3) * 3 + (j % 3)]);
            }
            s.add(distinct(row) && distinct(col) && distinct(box));
        }
    }

    size_t SolveSudoku(const char *input, int limit, bool pencilmark,
                       char *solution, size_t *num_guesses) {
        *num_guesses = 0;

        s.push();

        for (int i = 0; i < 81; i++) {
            if (pencilmark) {
                for (int j = 0; j < 9; j++) {
                    if (input[i * 9 + j] == '.') s.add(cell[i] != (j + 1));
                }
            } else {
                if (input[i] != '.') s.add(cell[i] == (input[i] - '0'));
            }
        }

        if (s.check() == z3::sat) {
            z3::model m = s.get_model();
            std::stringstream result;
            for (int i = 0; i < 81; i++) {
                result << m.eval(cell[i]);
            }
            strncpy(solution, result.str().c_str(), 81);
            s.pop();
            return 1;
        } else {
            s.pop();
            return 0;
        }
    }
};

} //end anonymous namespace


extern "C"
size_t OtherSolverZ3(const char *input, size_t limit, uint32_t config,
                     char *solution, size_t *num_guesses) {
    bool pencilmark = input[81] >= '.';
    static SolverZ3 solver{};
    return solver.SolveSudoku(input, limit, pencilmark, solution, num_guesses);
}
