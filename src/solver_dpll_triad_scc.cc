#include <algorithm>
#include <array>
#include <bitset>
#include <climits>
#include <cstring>
#include <iostream>
#include <map>
#include <random>
#include <set>
#include <vector>
#include <assert.h>

using namespace std;

namespace {

constexpr int kNumBoxes = 9;
constexpr int kNumPosClausesPerBox = 16; // 9 cells, 6 triads, 1 slack
constexpr int kNumValues = 9;

constexpr uint16_t kNumLiterals = kNumBoxes * kNumPosClausesPerBox * kNumValues * 2;
constexpr uint16_t kAllAsserted = kNumBoxes * (kNumPosClausesPerBox - 1) * kNumValues;

typedef uint32_t ClauseId;
typedef uint32_t LiteralId;

template<int literals>
class FastBitset {
    uint64_t bits[literals / 64 + 1]{};

public:
    void set(uint32_t index) {
        bits[index >> 6u] |= (1ul << (index & 63u));
    }

    bool operator[](uint32_t index) const {
        return bits[index >> 6u] & (1ul << (index & 63u));
    }

    bool pos_or_neg(uint32_t index) const {
        auto positive = index & ~1u;
        return bits[positive >> 6u] & (3ul << (positive & 63u));
    }
};

struct State {
    // 1s for asserted literals, 0s for literals negated or unknown
    FastBitset<kNumLiterals> asserted;
    // the number of literals that can be eliminated before the clause produces binary implications
    vector<uint16_t> clause_free_literals;
    // the number of implications for a given literal. we will not copy the implication lists
    // themselves as part of the state. instead the global state has a vector for each literal
    // that we use as a stack, and these counts are the stack pointers.
    array<uint16_t, kNumLiterals> implication_counts;
    // number of literals asserted. we are done when this equals kAllAsserted.
    uint32_t num_asserted = 0;

    State() : asserted{}, clause_free_literals{}, implication_counts{} {}

    State(const State &prior_state) = default;
};

struct SolverDpllTriadScc {
    // this mapping from ClauseId to LiteralId will not change after setup.
    vector<vector<LiteralId>> clauses_to_literals_{};
    // this mapping from LiteralId to ClauseId will not change after setup.
    array<vector<ClauseId>, kNumLiterals> literals_to_clauses_{};
    // initial setup will populate these vectors with implications that are part of Sudoku
    // rules. during BCP and DPLL search we'll discover new implications and push and pop them
    // from these vectors. we don't copy these vectors as part of the state. instead we just
    // copy implication counts that determine the logical size of these lists.
    array<vector<LiteralId>, kNumLiterals> literals_to_implications_{};
    // a list of clauses expressing that each cell must have a value. if we're not using SCCs
    // for choosing literals to branch then it suffices to pick among these clauses and then
    // pick a literal from the chosen clause.
    vector<ClauseId> positive_cell_clauses_{};
    // initial state with the correct implication counts. we'll clone this when we begin
    // solving each new puzzle, but this will not change after setup.
    State initial_state_{};
    // whether to use strongly connected component size as a heuristic for variable selection.
    bool scc_heuristic_ = true;
    // whether to apply inferences reached during strongly connected component evaluation.
    bool scc_inference_ = true;
    // stop after finding this many solutions.
    size_t limit_ = 1;

    size_t num_guesses_ = 0;
    size_t num_solutions_ = 0;
    State result_{};

    SolverDpllTriadScc() {
        SetupConstraints();
    }

    static void Display(State *state) {
        string div1 = " +=====+=====+=====+=====+=====+=====+=====+=====+=====+=====+=====+=====+";
        string div2 = " +-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+-----+";
        for (int i = 0; i < 12; i++) {
            cout << (((i % 4) == 0) ? div1 : div2) << endl;
            for (int vi = 0; vi < 3; vi++) {
                for (int j = 0; j < 12; j++) {
                    cout << " | ";
                    for (int vj = 0; vj < 3; vj++) {
                        int box = i / 4 * 3 + j / 4;
                        int elm = (i % 4) * 4 + (j % 4);
                        if (state->asserted[Not(Literal(box, elm, vi * 3 + vj))]) {
                            cout << " ";
                        } else {
                            cout << vi * 3 + vj + 1;
                        }
                    }
                }
                cout << " |" << endl;
            }
        }
        cout << div1 << endl << endl;
    }

    ///////////////////////////////////////////////
    // constraint setup
    ///////////////////////////////////////////////

    // literals are numbered so positive and negative literals for the same variable are adjacent.
    // a positive literal has id % 2 == 0.
    static inline LiteralId Not(LiteralId literal) {
        return literal ^ 1u;
    }

    // returns a *positive* literal id reflecting the proposition that the given element of the
    // given box has the given value. boxes and values are numbered 0-8. elements are numbered
    // based on a 4x4 grid, with the upper-left 3x3 subgrid being the actual 9 cells of the box
    // and the 3x1 and 1x3 extra column and row being horizontal and vertical triads. The last
    // element of the 4x4 grid is unused, but remains for indexing convenience.
    static uint32_t Literal(int box, int elem, int value) {
        // this order strikes the best balance of locality and avoiding division in ValidLiteral
        return 2 * (elem + 16 * (value + 9 * box));
    }

    // return true if the literal is in use (vs. in the filler space at the end of each box).
    static bool ValidLiteral(LiteralId literal) {
        return ((literal % 32u) & 0x1eu) != 0x1eu;
    }

    inline void AddImplication(LiteralId from, LiteralId to, State *state) {
        auto &implications = literals_to_implications_[from];
        auto &current_size = state->implication_counts[from];
        if (implications.size() == current_size) {
            implications.push_back(to);
        } else {
            implications[current_size] = to;
        }
        current_size++;
    }

    inline void AddClauseWithMinimum(const vector<LiteralId> &literals, int min) {
        ClauseId new_clause_id = clauses_to_literals_.size();
        for (LiteralId literal : literals) {
            literals_to_clauses_[literal].push_back(new_clause_id);
        }
        clauses_to_literals_.push_back(literals);
        initial_state_.clause_free_literals.push_back(literals.size() - 1 - min);
        if (min == 1 && literals.size() == 9) {
            positive_cell_clauses_.push_back(new_clause_id);
        }
    }

    void AddExactlyNConstraint(const vector<LiteralId> &literals, int n) {
        AddClauseWithMinimum(literals, n);
        if (n == 1) {
            for (size_t i = 0; i < literals.size() - 1; i++) {
                for (size_t j = i + 1; j < literals.size(); j++) {
                    AddImplication(literals[i], Not(literals[j]), &initial_state_);
                    AddImplication(literals[j], Not(literals[i]), &initial_state_);
                }
            }
        } else {
            vector<LiteralId> negations;
            negations.reserve(literals.size());
            for (auto literal : literals) negations.push_back(Not(literal));
            AddClauseWithMinimum(negations, negations.size() - n);
        }
    }

    void SetupConstraints() {
        for (int box = 0; box < 9; box++) {
            // ExactlyN constraints over values for a given cell or triad [1/9] and [3/9]
            for (int elem = 0; elem < 15; elem++) {
                vector<LiteralId> literals;
                for (int val = 0; val < 9; val++) {
                    literals.push_back(Literal(box, elem, val));
                }
                // exactly one for normal cells, exactly three for triads
                if (elem / 4 < 3 && elem % 4 < 3) {
                    AddExactlyNConstraint(literals, 1);
                } else {
                    AddExactlyNConstraint(literals, 3);
                }
            }
            // ExactlyN constraints to define each triad [1/4]
            for (int val = 0; val < 9; val++) {
                for (int i = 0; i < 3; i++) {
                    vector<LiteralId> h_triad, v_triad;
                    for (int j = 0; j < 3; j++) {
                        h_triad.push_back(Literal(box, i * 4 + j, val));
                        v_triad.push_back(Literal(box, i + j * 4, val));
                    }
                    h_triad.push_back(Not(Literal(box, i * 4 + 3, val)));
                    v_triad.push_back(Not(Literal(box, i + 12, val)));
                    AddExactlyNConstraint(h_triad, 1);
                    AddExactlyNConstraint(v_triad, 1);
                }
            }
        }
        // ExactlyN constraints over band triads within and across boxes [1/3]
        for (int val = 0; val < 9; val++) {
            for (int band = 0; band < 3; band++) {
                for (int i = 0; i < 3; i++) {
                    vector<LiteralId> h_within, h_across, v_within, v_across;
                    for (int j = 0; j < 3; j++) {
                        h_within.push_back(Literal(band * 3 + i, j * 4 + 3, val));
                        h_across.push_back(Literal(band * 3 + j, i * 4 + 3, val));
                        v_within.push_back(Literal(i * 3 + band, j + 12, val));
                        v_across.push_back(Literal(j * 3 + band, i + 12, val));
                    }
                    AddExactlyNConstraint(h_within, 1);
                    AddExactlyNConstraint(h_across, 1);
                    AddExactlyNConstraint(v_within, 1);
                    AddExactlyNConstraint(v_across, 1);
                }
            }
        }
    }

    ///////////////////////////////////////////////
    // boolean constraint propagation
    ///////////////////////////////////////////////

    vector<LiteralId> noneliminated;

    // we have a clause with a minimum of N that's now down to N+1 literals. if any of its
    // remaining literals are eliminated then the rest are implied.
    void AddBinaryImplicationsAmongNonEliminated(ClauseId clause_id, State *state) {
        const auto &literals = clauses_to_literals_[clause_id];
        int expect = literals.size() - initial_state_.clause_free_literals[clause_id];
        // optimize for the common case where the clause has a minimum of 1
        if (expect == 2) {
            LiteralId first = kNumLiterals;
            for (LiteralId literal : literals) {
                if (!state->asserted[Not(literal)]) {
                    if (first == kNumLiterals) {
                        first = literal;
                    } else {
                        AddImplication(Not(first), literal, state);
                        AddImplication(Not(literal), first, state);
                        return;
                    }
                }
            }
            assert(false);
        } else {
            noneliminated.clear();
            for (LiteralId literal : literals) {
                if (!state->asserted[Not(literal)]) {
                    noneliminated.push_back(literal);
                }
            }
            for (size_t i = 0; i < noneliminated.size() - 1; i++) {
                for (size_t j = i + 1; j < noneliminated.size(); j++) {
                    AddImplication(Not(noneliminated[i]), noneliminated[j], state);
                    AddImplication(Not(noneliminated[j]), noneliminated[i], state);
                }
            }
        }
    }

    bool Assert(LiteralId literal, State *state) {
        if (state->asserted[literal]) {
            return true;
        }
        if (state->asserted[Not(literal)]) {
            return false;
        }
        state->asserted.set(literal);
        state->num_asserted++;

        // decrement free literal counts for clauses containing the negation to reflect that these
        // literals are eliminated; update implication lists if this produces new binary clauses.
        for (auto clause_id : literals_to_clauses_[Not(literal)]) {
            if (--state->clause_free_literals[clause_id] == 0) {
                AddBinaryImplicationsAmongNonEliminated(clause_id, state);
            }
        }

        // now perform unit propagation by asserting implications of this literal. while it might
        // appear that new implications can be added during this loop, in practice this does not
        // occur, so it's best to read num_implications up front.
        const vector<LiteralId> &implications = literals_to_implications_[literal];
        uint16_t num_implications = state->implication_counts[literal];
        for (int i = 0; i < num_implications; i++) {
            if (!Assert(implications[i], state)) return false;
        }
        return true;
    }

    ///////////////////////////////////////////////
    // path-based strongly connected components with adaptations
    ///////////////////////////////////////////////

    int preorder_counter = 0;
    array<int, kNumLiterals> preorder_index{};
    vector<uint16_t> stack_p;
    vector<uint16_t> stack_s;
    array<int, kNumLiterals> literal_to_component_id{};
    int next_component_id = 0;
    int best_component_literal = -1;
    int best_component_size = -1;

    // returns false if visitation finds us to be in an inconsistent state.
    bool SccVisit(LiteralId literal, State *state) {
        if (scc_inference_) {
            int common_ancestor = -1;
            for (auto ancestor : stack_p) {
                if (preorder_index[ancestor] <= preorder_index[Not(literal)]) {
                    common_ancestor = ancestor;
                } else {
                    break;
                }
            }
            if (common_ancestor >= 0) {
                // we found a proximal ancestor implying both the literal and its negation.
                // (this ancestor might actually be the negation). we can therefore eliminate
                // the ancestor (and as a consequence the chain of literals from the ancestor
                // up to the root of stack_p). this might lead to discovery of a conflict.
                if (!Assert(Not(common_ancestor), state)) return false;
                // or it might lead to discovery of an assertion that lets us skip this branch.
                if (state->asserted[literal]) return true;
            }
        }
        preorder_index[literal] = preorder_counter++;
        stack_p.push_back(literal);
        stack_s.push_back(literal);

        auto &implications = literals_to_implications_[literal];
        auto &num_implications = state->implication_counts[literal];

        for (size_t i = 0; i < num_implications; i++) {
            uint16_t implication = implications[i];
            if (state->asserted[implication]) {
                // we can skip any already-asserted implications. these correspond to subsumed
                // binary clauses that have no effect on inference.
                continue;
            } else if (preorder_index[implication] == -1) {
                if (!SccVisit(implication, state)) {
                    return false; // back out. we are in an inconsistent state.
                }
                if (scc_inference_ && state->asserted.pos_or_neg(literal)) {
                    // visiting an implication and its consequences may have resulted in the
                    // current literal's assertion or negation. either way we can stop.
                    break;
                }
            } else if (literal_to_component_id[implication] == -1) {
                while (preorder_index[stack_p.back()] > preorder_index[implication]) {
                    stack_p.pop_back();
                }
            }
        }
        if (literal == stack_p.back()) {
            stack_p.pop_back();
            int component_size = (find(stack_s.rbegin(), stack_s.rend(), literal) -
                                  stack_s.rbegin() + 1);
            if (!state->asserted.pos_or_neg(literal)) {
                bool negation_has_component = literal_to_component_id[Not(literal)] >= 0;
                for (auto it = stack_s.end() - component_size; it != stack_s.end(); it++) {
                    literal_to_component_id[*it] = next_component_id;
                }
                // if the negation has a prior component it will be of the same size, and we
                // should prefer it since topologically there may exist a path of implication
                // from this component to the one containing the negation. in this case skip.
                if (!negation_has_component) {
                    // otherwise, we want to prioritize the largest component.
                    if (component_size > best_component_size) {
                        best_component_size = component_size;
                        best_component_literal = literal;
                    }
                }
                next_component_id++;
            }
            stack_s.resize(stack_s.size() - component_size);
        }
        return true;
    }

    bool FindStronglyConnectedComponents(State *state) {
        preorder_counter = 0;
        preorder_index.fill(-1);
        stack_p.clear();
        stack_s.clear();
        literal_to_component_id.fill(-1);
        next_component_id = 0;
        best_component_literal = -1;
        best_component_size = -1;
        // it suffices to explore positive literals as roots since every non-excluded negative
        // literal will be visited and will form the necessary component.
        for (uint16_t literal = 0; literal < kNumLiterals; literal += 2) {
            // we want SCCs of the graph of binary clauses, excluding subsumed clauses
            // and clauses that are actually unit due to an asserted negation.
            if (preorder_index[literal] == -1 && ValidLiteral(literal) &&
                !state->asserted.pos_or_neg(literal)) {
                if (!SccVisit(literal, state)) {
                    return false;
                }
            }
        }
        return true;
    }

    ///////////////////////////////////////////////
    // heuristic search
    ///////////////////////////////////////////////

    // find a literal in a large component and with favorable topological ordering.
    LiteralId ChooseLiteralToBranchByComponent(State *state) {
        return best_component_literal;
    }

    // find a positive clause with as few undetermined literals as possible and return one
    // such literal. assumes that the puzzle is *not* already solved.
    LiteralId ChooseLiteralToBranchByClause(State *state) {
        int min_free = INT8_MAX, which_clause = 0;
        for (int clause_id : positive_cell_clauses_) {
            int num_free = state->clause_free_literals[clause_id];
            if (num_free < min_free) {
                min_free = num_free;
                which_clause = clause_id;
            }
        }
        for (LiteralId literal : clauses_to_literals_[which_clause]) {
            if (!state->asserted[Not(literal)]) {
                return literal;
            }
        }
        exit(1); // shouldn't be possible if puzzle is unsolved.
    }

    void BranchOnLiteral(LiteralId literal, State *state) {
        num_guesses_++;
        State state_copy = *state;
        if (Assert(literal, &state_copy)) {
            CountSolutionsConsistentWithPartialAssignment(&state_copy);
            if (num_solutions_ == limit_) {
                return;
            }
        }
        if (Assert(Not(literal), state)) {
            CountSolutionsConsistentWithPartialAssignment(state);
        }
    }

    void CountSolutionsConsistentWithPartialAssignment(State *state) {
        if (scc_heuristic_ || scc_inference_) {
            while (state->num_asserted < kAllAsserted) {
                auto prev_asserted = state->num_asserted;
                if (!FindStronglyConnectedComponents(state)) return;
                if (prev_asserted == state->num_asserted) break;
            }
        }
        if (state->num_asserted == kAllAsserted) {
            if (++num_solutions_ == 1) {
                result_ = *state;
            }
            return;
        } else {
            LiteralId branch_literal = scc_heuristic_ ?
                                       ChooseLiteralToBranchByComponent(state) :
                                       ChooseLiteralToBranchByClause(state);
            BranchOnLiteral(branch_literal, state);
        }
    }

    ///////////////////////////////////////////////
    // pencilmark sudoku generation
    ///////////////////////////////////////////////

    random_device rd{};
    mt19937_64 rng{rd()};

    bool Generate(bool pencilmark, const vector<int> &permutation, int level,
                  char *clues, State *state) {
        if (state->num_asserted == kAllAsserted) {
            return true; // current assertions fully identify solution
        }
        if (level == permutation.size()) {
            return false; // not solved, but remaining assertions all tried and failed
        }
        int var_idx = permutation[level];
        int cell = var_idx / 9;
        int box = cell / 27 * 3 + (cell % 9) / 3;
        int elm = ((cell / 9) % 3) * 4 + (cell % 3);
        int val = var_idx % 9;
        LiteralId lit = Literal(box, elm, val);

        if (state->asserted[lit] || state->asserted[Not(lit)]) {
            return Generate(pencilmark, permutation, level + 1, clues, state);
        } else {
            State state2 = *state;
            if (pencilmark) {
                clues[var_idx] = '.';
                if (Assert(Not(lit), &state2) &&
                    Generate(pencilmark, permutation, level + 1, clues, &state2)) {
                    return true;
                }
                clues[var_idx] = (char) ('1' + val);
                return Assert(lit, state) &&
                       Generate(pencilmark, permutation, level + 1, clues, state);
            } else {
                clues[cell] = (char)('1' + val);
                if (Assert(lit, &state2) &&
                    Generate(pencilmark, permutation, level + 1, clues, &state2)) {
                    return true;
                }
                clues[cell] = '.';
                return Assert(Not(lit), state) &&
                       Generate(pencilmark, permutation, level + 1, clues, state);
            }
        }
    }

    void Minimize(bool pencilmark, const vector<int> &permutation, char *clues) {
        limit_ = 2;
        int max = pencilmark ? 729 : 81;
        for (int i = 0; i < max; i++) {
            int j = pencilmark ? permutation[729-i-1] : i;
            char tmp = clues[j];
            clues[j] = pencilmark ? (char)('1' + (j % 9)) : '.';
            if (tmp == clues[j]) continue;
            State state = initial_state_;
            InitializePuzzle(clues, pencilmark, &state);
            num_solutions_ = 0;
            CountSolutionsConsistentWithPartialAssignment(&state);
            if (num_solutions_ > 1) {
                clues[j] = tmp;
            }
        }
    }

    bool InitializePuzzle(const char *input, bool pencilmark, State *state) {
        for (int i = 0; i < 81; i++) {
            int box = i / 27 * 3 + (i % 9) / 3;
            int elm = ((i / 9) % 3) * 4 + (i % 3);
            if (pencilmark) {
                for (int j = 0; j < 9; j++) {
                    if (input[i * 9 + j] == '.') {
                        if (!Assert(Not(Literal(box, elm, j)), state)) return false;
                    }
                }
            } else {
                char digit = input[i];
                if (digit != '.') {
                    int val = digit - '1';
                    if (!Assert(Literal(box, elm, val), state)) return false;
                }
            }
        }
        return true;
    }

    ///////////////////////////////////////////////
    // entry
    ///////////////////////////////////////////////

    size_t SolveSudoku(const char *input, size_t limit, uint32_t configuration,
                       char *solution, size_t *num_guesses) {
        limit_ = limit;
        scc_inference_ = (configuration & 1u) > 0;
        scc_heuristic_ = (configuration & 2u) > 0;
        bool pencilmark = input[81] >= '.';
        num_solutions_ = 0;
        *num_guesses = num_guesses_ = 0;

        result_ = initial_state_;
        State state = initial_state_;

        if (!InitializePuzzle(input, pencilmark, &state)) {
            return 0;
        }
        CountSolutionsConsistentWithPartialAssignment(&state);

        for (int i = 0; i < 81; i++) {
            int box = i / 27 * 3 + (i % 9) / 3;
            int elm = ((i / 9) % 3) * 4 + (i % 3);
            for (int val = 0; val < 9; val++) {
                if (result_.asserted[Literal(box, elm, val)]) {
                    solution[i] = char('1' + val);
                }
            }
        }
        *num_guesses = num_guesses_;
        return num_solutions_;
    }

    void Generate(bool pencilmark, char *clues) {
        vector<int> permutation;
        permutation.reserve(729);
        for (int i = 0; i < 729; i++) permutation.push_back(i);
        while (true) {
            State state = initial_state_;
            shuffle(permutation.begin(), permutation.end(), rng);
            for (int i = 0; i < 729; i++) {
                clues[i] = pencilmark ? (char) ('1' + (i % 9)) : '.';
            }
            if (Generate(pencilmark, permutation, 0, clues, &state)) {
                Minimize(pencilmark, permutation, clues);
                break;
            }
        }
    }
};

}  // namespace


extern "C"
size_t TdokuSolverDpllTriadScc(const char *input, size_t limit, uint32_t configuration,
                               char *solution, size_t *num_guesses) {
    static SolverDpllTriadScc solver;
    return solver.SolveSudoku(input, limit, configuration, solution, num_guesses);
}

extern "C"
void GeneratePuzzle(bool pencilmark, char *clues) {
    static SolverDpllTriadScc solver;
    solver.Generate(pencilmark, clues);
}
