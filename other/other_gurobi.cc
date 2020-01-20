#include "gurobi_c++.h"

namespace {

struct SolverGurobi {

    GRBEnv env{};
    GRBModel model{env};
    GRBVar vars[9][9][9];

    SolverGurobi() {
        model.set(GRB_IntParam_Threads, 1);
        model.set(GRB_IntParam_OutputFlag, 0);

        for (int i = 0; i < 9; i++) {
            for (int j = 0; j < 9; j++) {
                for (int k = 0; k < 9; k++) {
                    vars[i][j][k] = model.addVar(0.0, 1.0, 0.0, GRB_BINARY);
                }
            }
        }
        for (int i = 0; i < 9; i++) {
            for (int j = 0; j < 9; j++) {
                GRBLinExpr cell = 0, row = 0, col = 0, box = 0;
                for (int k = 0; k < 9; k++) {
                    cell += vars[i][j][k];
                    row += vars[i][k][j];
                    col += vars[k][i][j];
                    box += vars[(i / 3) * 3 + (k / 3)][(i % 3) * 3 + (k % 3)][j];
                }
                model.addConstr(cell, GRB_EQUAL, 1.0);
                model.addConstr(row, GRB_EQUAL, 1.0);
                model.addConstr(col, GRB_EQUAL, 1.0);
                model.addConstr(box, GRB_EQUAL, 1.0);
            }
        }
    }

    size_t SolveSudoku(const char *input, int limit, bool pencilmark,
                       char *solution, size_t *num_guesses) {
        for (int row = 0; row < 9; row++) {
            for (int column = 0; column < 9; column++) {
                int cell = row * 9 + column;
                for (int value = 0; value < 9; value++) {
                    if (pencilmark) {
                        vars[row][column][value].set(
                                GRB_DoubleAttr_UB, input[cell * 9 + value] == '.' ? 0.0 : 1.0);
                    } else {
                        bool allowed = input[cell] == '.' || input[cell] == (char) ('1' + value);
                        vars[row][column][value].set(
                                GRB_DoubleAttr_UB, allowed ? 1.0 : 0.0);
                    }
                }
            }
        }

        if (limit == 1) {
            model.set(GRB_IntParam_PoolSearchMode, 0);
            model.set(GRB_IntParam_PoolSolutions, 1);
        } else {
            model.set(GRB_IntParam_PoolSearchMode, 2);
            model.set(GRB_IntParam_PoolSolutions, limit);
        }

        model.optimize();

        int status = model.get(GRB_IntAttr_Status);
        bool satisfied = status != GRB_INFEASIBLE;

        if (satisfied) {
            for (int row = 0; row < 9; row++) {
                for (int column = 0; column < 9; column++) {
                    for (int value = 0; value < 9; value++) {
                        if (vars[row][column][value].get(GRB_DoubleAttr_X) > 0.5) {
                            solution[row * 9 + column] = value + '1';
                        }
                    }
                }
            }
        }
        *num_guesses = model.get(GRB_DoubleAttr_IterCount);
        return model.get(GRB_IntAttr_SolCount);
    }
};

} //end anonymous namespace


extern "C"
size_t OtherSolverGurobi(const char *input, size_t limit, uint32_t config,
                         char *solution, size_t *num_guesses) {
    bool pencilmark = input[81] >= '.';
    static SolverGurobi solver{};
    return solver.SolveSudoku(input, limit, pencilmark, solution, num_guesses);
}
