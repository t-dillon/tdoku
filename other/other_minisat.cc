#include <minisat/core/Solver.h>

namespace {

using Minisat::Lit;
using Minisat::mkLit;

struct SolverMiniSat {
    Minisat::Solver solver;

    // Construct a solver using one of four CNF representations of Sudoku logic:
    // 0 = minimal representation: each cell has a value. no group has a value twice.
    // 1 = natural representation: to the minimal representation adds explicit constraints
    //     ensuring a cell has *only* one value instead of relying on pigeonhole inferences.
    // 2 = complete representation: to the natural representation adds explicit constraints
    //     ensuring a group has at least one of each value instead of relying on pigeonhole
    //     inferences. i.e., both cells and groups are now modeled as exactly-one.
    // 3 = augmented representation: replaces exactly-one constraints over groups with
    //     definition of triads and constraints over triads.
    //
    //       logic              cells          groups         triads
    //   representation      min=1 max=1    min=1  max=1   min=1  max=1
    //   --------------      ----- -----    -----  -----   -----  -----
    //   0. minimal            X                     X
    //   1. natural            X     X               X
    //   2. complete           X     X        X      X
    //   3. augmented          X     X                       X      X
    //
    SolverMiniSat(int config) {
        InitializeVariables(config == 3);
        InitializeCellConstraints(config > 0);
        if (config < 3) {
            InitializeGroupConstraints(config > 1);
        } else {
            InitializeTriadDefinitions();
            InitializeTriadConstraints();
        }
    }

    // normal cell literals, of which we have 9*9*9
    static Lit Literal(int row, int column, int value) {
        return mkLit(value + 9 * (column + 9 * row), true);
    }

    // horizontal triad literals, of which we have 9*3*9, starting after the cell literals
    static Lit HTriadLiteral(int row, int column, int value) {
        int base = 81 * 9;
        return mkLit(base + value + 9 * (column + 3 * row));
    }

    // vertical triad literals, of which we have 3*9*9, starting after the h_triad literals
    static Lit VTriadLiteral(int row, int column, int value) {
        int base = (81 + 27) * 9;
        return mkLit(base + value + 9 * (row + 3 * column));
    }

    void InitializeVariables(bool with_triads) {
        for (int i = 0; i < (with_triads ? 15 : 9) * 9 * 9; i++) {
            solver.newVar();
        }
    }

    // create one or both sides of an exactly-one constraint over a set of literals
    void Constrain(const Minisat::vec<Minisat::Lit> &literals, bool one, bool only_one) {
        if (one) {
            solver.addClause(literals);
        }
        if (only_one) {
            for (int i = 0; i < literals.size() - 1; i++) {
                for (int j = i + 1; j < literals.size(); j++) {
                    solver.addClause(~literals[i], ~literals[j]);
                }
            }
        }
    }

    void InitializeCellConstraints(bool only_one) {
        for (int row = 0; row < 9; row++) {
            for (int column = 0; column < 9; column++) {
                Minisat::vec<Minisat::Lit> literals;
                for (int value = 0; value < 9; value++) {
                    literals.push(Literal(row, column, value));
                }
                Constrain(literals, true, only_one);
            }
        }
    }

    void InitializeGroupConstraints(bool one) {
        for (int i = 0; i < 9; i++) {
            for (int value = 0; value < 9; value++) {
                Minisat::vec<Minisat::Lit> row;
                Minisat::vec<Minisat::Lit> col;
                Minisat::vec<Minisat::Lit> box;

                for (int j = 0; j < 9; j++) {
                    row.push(Literal(i, j, value));
                    col.push(Literal(j, i, value));
                    box.push(Literal(i / 3 * 3 + j / 3, (i % 3) * 3 + (j % 3), value));
                }

                Constrain(row, one, true);
                Constrain(col, one, true);
                Constrain(box, one, true);
            }
        }
    }

    void InitializeTriadDefinitions() {
        for (int i = 0; i < 9; i++) {
            for (int j = 0; j < 3; j++) {
                for (int value = 0; value < 9; value++) {
                    Lit h_triad = HTriadLiteral(i, j, value);
                    Lit v_triad = VTriadLiteral(j, i, value);
                    int j0 = j * 3 + 0, j1 = j * 3 + 1, j2 = j * 3 + 2;

                    Minisat::vec<Minisat::Lit> h_triad_def;
                    h_triad_def.push(Literal(i, j0, value));
                    h_triad_def.push(Literal(i, j1, value));
                    h_triad_def.push(Literal(i, j2, value));
                    h_triad_def.push(~h_triad);
                    Constrain(h_triad_def, true, true);

                    Minisat::vec<Minisat::Lit> v_triad_def;
                    v_triad_def.push(Literal(j0, i, value));
                    v_triad_def.push(Literal(j1, i, value));
                    v_triad_def.push(Literal(j2, i, value));
                    v_triad_def.push(~v_triad);
                    Constrain(v_triad_def, true, true);
                }
            }
        }
    }

    void InitializeTriadConstraints() {
        for (int i = 0; i < 9; i++) {
            for (int value = 0; value < 9; value++) {
                Minisat::vec<Minisat::Lit> row;
                row.push(HTriadLiteral(i, 0, value));
                row.push(HTriadLiteral(i, 1, value));
                row.push(HTriadLiteral(i, 2, value));
                Constrain(row, true, true);

                Minisat::vec<Minisat::Lit> column;
                column.push(VTriadLiteral(0, i, value));
                column.push(VTriadLiteral(1, i, value));
                column.push(VTriadLiteral(2, i, value));
                Constrain(column, true, true);

                Minisat::vec<Minisat::Lit> hbox;
                hbox.push(HTriadLiteral(3 * (i / 3) + 0, i % 3, value));
                hbox.push(HTriadLiteral(3 * (i / 3) + 1, i % 3, value));
                hbox.push(HTriadLiteral(3 * (i / 3) + 2, i % 3, value));
                Constrain(hbox, true, true);

                Minisat::vec<Minisat::Lit> vbox;
                vbox.push(VTriadLiteral(i % 3, 3 * (i / 3) + 0, value));
                vbox.push(VTriadLiteral(i % 3, 3 * (i / 3) + 1, value));
                vbox.push(VTriadLiteral(i % 3, 3 * (i / 3) + 2, value));
                Constrain(vbox, true, true);
            }
        }
    }

    size_t SolveSudoku(const char *input, char *solution, bool pencilmark, size_t *num_guesses) {
        Minisat::vec<Minisat::Lit> assumptions;
        for (int row = 0; row < 9; row++) {
            for (int column = 0; column < 9; column++) {
                if (pencilmark) {
                    for (int value = 0; value < 9; value++) {
                        if (input[row * 81 + column * 9 + value] == '.') {
                            assumptions.push(~Literal(row, column, value));
                        }
                    }
                } else {
                    char digit = input[row * 9 + column];
                    if (digit != '.') {
                        assumptions.push(Literal(row, column, digit - '1'));
                    }
                }
            }
        }
        solver.decisions = 0;
        bool satisfied = solver.solve(assumptions);
        if (satisfied) {
            for (int row = 0; row < 9; row++) {
                for (int column = 0; column < 9; column++) {
                    for (int value = 0; value < 9; value++) {
                        if (solver.model[value + 9 * (column + 9 * row)] ==
                            Minisat::lbool((uint8_t) 1)) {
                            solution[row * 9 + column] = value + '1';
                        }
                    }
                }
            }
        }
        // minisat doesn't report decisions consistently. If satisfied it reports 1 + the
        // actual decision count.
        *num_guesses = solver.decisions - (satisfied ? 1 : 0);
        return satisfied ? 1 : 0;
    }
};

} //end anonymous namespace


extern "C"
size_t OtherSolverMiniSat(const char *input, size_t /*unused_limit*/, uint32_t config,
                          char *solution, size_t *num_guesses) {
    bool pencilmark = input[81] >= '.';
    switch(config % 4) {
        case 0:
            static SolverMiniSat s0{0};
            return s0.SolveSudoku(input, solution, pencilmark, num_guesses);
        case 1:
            static SolverMiniSat s1{1};
            return s1.SolveSudoku(input, solution, pencilmark, num_guesses);
        case 2:
            static SolverMiniSat s2{2};
            return s2.SolveSudoku(input, solution, pencilmark, num_guesses);
        default:
            static SolverMiniSat s3{3};
            return s3.SolveSudoku(input, solution, pencilmark, num_guesses);
    }
}
