#include <minisat/core/Solver.h>

namespace {

using Minisat::Lit;
using Minisat::mkLit;
using namespace std;

struct SolverMiniSat {
    Minisat::Solver solver;

    SolverMiniSat() {
        InitializeVariables();
        InitializeTriadDefinitions();
        InitializeTriadOnnes();
        InitializeCellOnnes();
    }

    // normal cell literals, of which we have 9*9*9
    Lit Literal(int row, int column, int value) {
        return mkLit(value + 9 * (column + 9 * row), true);
    }

    // horizontal triad literals, of which we have 9*3*9, starting after the cell literals
    Lit HTriadLiteral(int row, int column, int value) {
        int base = 81 * 9;
        return mkLit(base + value + 9 * (column + 3 * row));
    }

    // vertical triad litearls, of which we have 3*9*9, starting after the h_triad literals
    Lit VTriadLiteral(int row, int column, int value) {
        int base = (81 + 27) * 9;
        return mkLit(base + value + 9 * (row + 3 * column));
    }

    void InitializeVariables() {
        for (int i = 0; i < 15 * 9 * 9; i++) {
            solver.newVar();
        }
    }

    // create an exactly-one constraint over a set of literals
    void CreateOnne(const Minisat::vec<Minisat::Lit> &literals) {
        solver.addClause(literals);
        for (int i = 0; i < literals.size() - 1; i++) {
            for (int j = i + 1; j < literals.size(); j++) {
                solver.addClause(~literals[i], ~literals[j]);
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
                    CreateOnne(h_triad_def);

                    Minisat::vec<Minisat::Lit> v_triad_def;
                    v_triad_def.push(Literal(j0, i, value));
                    v_triad_def.push(Literal(j1, i, value));
                    v_triad_def.push(Literal(j2, i, value));
                    v_triad_def.push(~v_triad);
                    CreateOnne(v_triad_def);
                }
            }
        }
    }

    void InitializeTriadOnnes() {
        for (int i = 0; i < 9; i++) {
            for (int value = 0; value < 9; value++) {
                Minisat::vec<Minisat::Lit> row;
                row.push(HTriadLiteral(i, 0, value));
                row.push(HTriadLiteral(i, 1, value));
                row.push(HTriadLiteral(i, 2, value));
                CreateOnne(row);

                Minisat::vec<Minisat::Lit> column;
                column.push(VTriadLiteral(0, i, value));
                column.push(VTriadLiteral(1, i, value));
                column.push(VTriadLiteral(2, i, value));
                CreateOnne(column);

                Minisat::vec<Minisat::Lit> hbox;
                hbox.push(HTriadLiteral(3 * (i / 3) + 0, i % 3, value));
                hbox.push(HTriadLiteral(3 * (i / 3) + 1, i % 3, value));
                hbox.push(HTriadLiteral(3 * (i / 3) + 2, i % 3, value));
                CreateOnne(hbox);

                Minisat::vec<Minisat::Lit> vbox;
                vbox.push(VTriadLiteral(i % 3, 3 * (i / 3) + 0, value));
                vbox.push(VTriadLiteral(i % 3, 3 * (i / 3) + 1, value));
                vbox.push(VTriadLiteral(i % 3, 3 * (i / 3) + 2, value));
                CreateOnne(vbox);
            }
        }
    }

    void InitializeCellOnnes() {
        for (int row = 0; row < 9; row++) {
            for (int column = 0; column < 9; column++) {
                Minisat::vec<Minisat::Lit> literals;
                for (int value = 0; value < 9; value++) {
                    literals.push(Literal(row, column, value));
                }
                CreateOnne(literals);
            }
        }
    }

    bool SolveSudoku(const char *input, char *solution, size_t *num_guesses) {
        Minisat::vec<Minisat::Lit> assumptions;
        for (int row = 0; row < 9; row++) {
            for (int column = 0; column < 9; column++) {
                char digit = input[row * 9 + column];
                if (digit != '.') {
                    assumptions.push(Literal(row, column, digit - '1'));
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
        *num_guesses = solver.decisions - 1;
        return satisfied;
    }
};

} //end anonymous namespace


extern "C"
size_t TdokuSolverMiniSat(const char *input, size_t /*unused_limit*/, uint32_t /*unused_flags*/,
                          char *solution, size_t *num_guesses) {
    static SolverMiniSat solver;
    return solver.SolveSudoku(input, solution, num_guesses);
}
