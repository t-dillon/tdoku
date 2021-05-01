#include <minisat/core/Solver.h>
#include <memory>
#include <iostream>

namespace {

using Minisat::Lit;
using Minisat::mkLit;

const Minisat::lbool True = Minisat::lbool((uint8_t) 1);
const Minisat::lbool False = Minisat::lbool((uint8_t) 0);

struct SolverMiniSat {
    int config_;
    std::unique_ptr<Minisat::Solver> solver_;
    int num_normal_vars_ = 0;

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
    SolverMiniSat(int config) : config_(config), solver_(std::make_unique<Minisat::Solver>()) {
        Reinitialize();
    }

    void Reinitialize() {
        InitializeVariables(config_ == 3);
        InitializeCellConstraints(config_ > 0);
        if (config_ < 3) {
            InitializeGroupConstraints(config_ > 1);
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
            solver_->newVar();
        }
        num_normal_vars_ = solver_->nVars();
    }

    // create one or both sides of an exactly-one constraint over a set of literals
    void Constrain(const Minisat::vec<Minisat::Lit> &literals, bool one, bool only_one) {
        if (one) {
            solver_->addClause(literals);
        }
        if (only_one) {
            for (int i = 0; i < literals.size() - 1; i++) {
                for (int j = i + 1; j < literals.size(); j++) {
                    solver_->addClause(~literals[i], ~literals[j]);
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

    // count the number of satisfying assignments up to the given limit.
    size_t SatCount(Minisat::vec<Minisat::Lit> &assumptions, int limit) {
        // minisat does not have a native way to count satisfying assignments. to achieve
        // this we have to repeatedly solve(), each time adding a blocking clause that
        // excludes the last solution found. since minisat also doesn't have a general way
        // to remove clauses, and it would be too expensive to reinitialize a solver
        // from scratch for each puzzle, we add a variable for each call to SatCount that
        // can control the activity of the blocking clauses and allow minisat to simplify
        // them away later.

        // still, we will want to periodically reinitialize the solver. while this is
        // expensive to do, it is also expensive to accumulate the blocking clause activator
        // variables and associated unit clauses that we make below. more frequent
        // reinitialization is better for hard puzzles, less frequent is better for easy ones.
        // here we'll reinitialize every 20 SatCounts, which seems a good compromise.
        if (solver_->nVars() - num_normal_vars_ > 20) {
            solver_.reset(new Minisat::Solver());
            Reinitialize();
        }

        // create activator variable and temporarily add as an assumption
        auto blocking_clause_activator = solver_->newVar();
        assumptions.push(Minisat::mkLit(blocking_clause_activator, true));

        Minisat::vec<Minisat::Lit> blocking_clause;
        size_t num_solutions = 0;
        while (num_solutions < limit && solver_->solve(assumptions)) {
            num_solutions++;

            blocking_clause.clear();
            // add a clause which, when the activator is true, ...
            blocking_clause.push(Minisat::mkLit(blocking_clause_activator, false));
            // asserts that some component of the current solution as reversed polarity
            for (int i = 0; i < num_normal_vars_; i++) {
                blocking_clause.push(Minisat::mkLit(i, solver_->modelValue(i) == False));
            }
            solver_->addClause(blocking_clause);
        }

        // we now want to make the blocking clauses for this puzzle go away. we remove
        // the activator assumption and add a clause asserting deactivation. minisat will
        // simplify the blocking clauses away on the next call to solve().
        assumptions.pop();
        solver_->addClause(Minisat::mkLit(blocking_clause_activator, false));

        return num_solutions;
    }

    size_t SolveSudoku(const char *input, bool pencilmark, size_t limit,
                       char *solution, size_t *num_guesses) {
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
        size_t count = 0;
        solver_->decisions = 0;
        if (limit == 1) {
            if (solver_->solve(assumptions)) {
                count = 1;
                for (int row = 0; row < 9; row++) {
                    for (int column = 0; column < 9; column++) {
                        for (int value = 0; value < 9; value++) {
                            if (solver_->model[value + 9 * (column + 9 * row)] == True) {
                                solution[row * 9 + column] = value + '1';
                            }
                        }
                    }
                }
            }
        } else {
           count = SatCount(assumptions, limit);
        }
        // minisat doesn't report decisions correctly. For each call to solve it accrues
        // 1 + the actual decision count, so we have to subtract the number of number of
        // models found.
        *num_guesses = solver_->decisions - count;
        return count;
    }
};

SolverMiniSat s0{0};
SolverMiniSat s1{1};
SolverMiniSat s2{2};
SolverMiniSat s3{3};

SolverMiniSat *solvers[4]{&s0, &s1, &s2, &s3};

} //end anonymous namespace


extern "C"
size_t OtherSolverMiniSat(const char *input, size_t limit, uint32_t config,
                          char *solution, size_t *num_guesses) {
    bool pencilmark = input[81] >= '.';
    return solvers[config % 4]->SolveSudoku(input, pencilmark, limit, solution, num_guesses);
}
