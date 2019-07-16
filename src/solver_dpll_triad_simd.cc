#include "bitutil.h"
#include "simd_vectors.h"

#include <array>
#include <cstring>
#include <immintrin.h>

using namespace std;

namespace {

typedef Bitvec08x16 Cells08;
typedef Bitvec16x16 Cells16;

constexpr uint16_t kAll = 0x1ff;

//  The state of each box is stored in a vector of 16 uint16_t,     +---+---+---+---+
//  arranged as a 4x4 matrix of 9-bit candidate sets (the high      | c | c | c | H |
//  7 bits of each value are always zero). The top-left 3x3 sub-    +---+---+---+---+
//  matrix stores candidate sets for the 9 cells("c") of the box,   | c | c | c | H |
//  while the right 3x1 column and bottom 1x3 row store candidate   +---+---+---+---+
//  sets representing negative horizontal("H") and vertical("V")    | c | c | c | H |
//  triads respectively. A negative triad candidate will be         +---+---+---+---+
//  eliminated whenever we know that the same value must exist      | V | V | V |   |
//  in one three regular cells to which the triad corresponds.      +---+---+---+---+
//
//  For each value bit there is an exactly-one constraint over the 4 cells in a row
//  or column of the matrix, which corresponds to the biconditional defining the triad.
//
//  Each cell also has a minimum. So there are three sets of clauses represented here.
//
struct Box {
    Cells16 cells{Cells16::All(kAll)};
    uint32_t box_idx, box_i, box_j;

    Box(uint32_t box_idx) : box_idx(box_idx), box_i(box_idx / 3), box_j(box_idx % 3) {}
};

// For a given value there are only 6 possible configurations for how that value can be
// placed in the triads of a band. Our primary representation for the state of a band will
// be in terms of these configurations rather than the triads themselves. The possible
// configurations are numbered according to the following table:
//
//            config       0       1       2       3       4       5
//             elem      0 1 2   0 1 2   0 1 2   0 1 2   0 1 2   0 1 2
//                     +-------+-------+-------+-------+-------+-------+
//            peer0    | X . . | . X . | . . X | . . X | X . . | . X . |
//            peer1    | . X . | . . X | X . . | . X . | . . X | X . . |
//            peer2    | . . X | X . . | . X . | X . . | . X . | . . X |
//                     +-------+-------+-------+-------+-------+-------+
//
// The primary state of the band is stored as 9-bit masks     elem    0   1   2
// in the first 6 elements of an 8 uint16_t vector.                 +---+---+---+---+
//                                                           peer0  | t | t | t |   |
// When constructing elimination masks to send to the boxes         +---+---+---+---+
// we'll convert the configuration vector into a 3x3 matrix  peer1  | t | t | t |   |
// of positive triad candidates, which are arranged with            +---+---+---+---+
// box peers along the rows of 4x4 matrix in a 16 uint16_t   peer2  | t | t | t |   |
// vector (for both horizontal and vertical bands).                 +---+---+---+---+
//                                                                  |   |   |   |   |
//                                                                  +---+---+---+---+
struct Band {
    Cells08 configurations{kAll, kAll, kAll, kAll, kAll, kAll, 0, 0};
    uint32_t box_peers[3]{};

    Band(int orientation, int idx) {
        for (int i = 0; i < 3; i++) {
            box_peers[i] = orientation ? i * 3 + idx : idx * 3 + i;
        }
    }
};

struct State {
    Box boxen[9]{0, 1, 2, 3, 4, 5, 6, 7, 8};
    Band bands[2][3]{{{0, 0}, {0, 1}, {0, 2}},
                     {{1, 0}, {1, 1}, {1, 2}}};
};

struct BoxIndexing {
    uint8_t box_i;
    uint8_t box_j;
    uint8_t box;
    uint8_t elem_i;
    uint8_t elem_j;
    uint8_t elem;

    BoxIndexing() = default;

    explicit BoxIndexing(int cell) : box_i(cell / 27), box_j((cell % 9) / 3),
                                     box(box_i * 3 + box_j),
                                     elem_i((cell / 9) % 3), elem_j(cell % 3),
                                     elem(elem_i * 4 + elem_j) {}
};

constexpr uint16_t shuf00 = 0x0100, shuf01 = 0x0302, shuf02 = 0x0504, shuf03 = 0x0706;
constexpr uint16_t shuf04 = 0x0908, shuf05 = 0x0b0a, shuf06 = 0x0d0c, shuf07 = 0x0f0e;

struct Tables {
    // used when assigning a single candidate during search
    Cells16 cell_assignment_eliminations[16][16]{};

    // @formatter:off
    const Cells08 shuffle_peer_x_shift_to_config_mask[3][3]{
            {{shuf04, shuf05, shuf06, shuf06, shuf04, shuf05, 0xffff, 0xffff},
             {shuf05, shuf06, shuf04, shuf04, shuf05, shuf06, 0xffff, 0xffff},
             {shuf06, shuf04, shuf05, shuf05, shuf06, shuf04, 0xffff, 0xffff}},
            {{shuf05, shuf06, shuf04, shuf05, shuf06, shuf04, 0xffff, 0xffff},
             {shuf06, shuf04, shuf05, shuf06, shuf04, shuf05, 0xffff, 0xffff},
             {shuf04, shuf05, shuf06, shuf04, shuf05, shuf06, 0xffff, 0xffff}},
            {{shuf06, shuf04, shuf05, shuf04, shuf05, shuf06, 0xffff, 0xffff},
             {shuf04, shuf05, shuf06, shuf05, shuf06, shuf04, 0xffff, 0xffff},
             {shuf05, shuf06, shuf04, shuf06, shuf04, shuf05, 0xffff, 0xffff}},
    };
    const Cells16 shuffle_configs_to_triads[2]{
            {{shuf00, shuf01, shuf02, 0xffff, shuf02, shuf00, shuf01, 0xffff},
             {shuf01, shuf02, shuf00, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff}},
            {{shuf04, shuf05, shuf03, 0xffff, shuf05, shuf03, shuf04, 0xffff},
             {shuf03, shuf04, shuf05, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff}}
    };
    const Cells08 shuffle_triads_temp[2]{
            {shuf00, shuf01, shuf02, shuf01, shuf00, shuf00, 0xffff, 0xffff},
            {shuf00, shuf01, shuf02, shuf02, shuf02, shuf01, 0xffff, 0xffff}
    };
    const Cells16 shuffle_temp_to_box_mask[2]{
            // for horizontal bands/triads
            {{shuf00, shuf00, shuf00, shuf03, shuf01, shuf01, shuf01, shuf04},
             {shuf02, shuf02, shuf02, shuf05, 0xffff, 0xffff, 0xffff, 0xffff}},
            // for vertical bands/triads
            {{shuf00, shuf01, shuf02, 0xffff, shuf00, shuf01, shuf02, 0xffff},
             {shuf00, shuf01, shuf02, 0xffff, shuf03, shuf04, shuf05, 0xffff}}
    };

    const Cells16 cell3x3_mask{ kAll, kAll, kAll,    0,
                                kAll, kAll, kAll,    0,
                                kAll, kAll, kAll,    0,
                                   0,    0,    0,    0
    };
    const Cells08 peer_x_elem_to_config_mask[3][3]{
            {{   0,   kAll,   kAll,   kAll,      0,   kAll,    0,    0},
             {kAll,      0,   kAll,   kAll,   kAll,      0,    0,    0},
             {kAll,   kAll,      0,      0,   kAll,   kAll,    0,    0}},
            {{kAll,   kAll,      0,   kAll,   kAll,      0,    0,    0},
             {   0,   kAll,   kAll,      0,   kAll,   kAll,    0,    0},
             {kAll,      0,   kAll,   kAll,      0,   kAll,    0,    0}},
            {{kAll,      0,   kAll,      0,   kAll,   kAll,    0,    0},
             {kAll,   kAll,      0,   kAll,      0,   kAll,    0,    0},
             {   0,   kAll,   kAll,   kAll,   kAll,      0,    0,    0}}
    };

    const Cells08 one_value_mask[9]{
            Cells08::All(1u << 0u), Cells08::All(1u << 1u), Cells08::All(1u << 2u),
            Cells08::All(1u << 3u), Cells08::All(1u << 4u), Cells08::All(1u << 5u),
            Cells08::All(1u << 6u), Cells08::All(1u << 7u), Cells08::All(1u << 8u),
    };
    // @formatter:on

    BoxIndexing box_indexing[81]{};

    Tables() noexcept {
        for (int i : {0, 1, 2, 4, 5, 6, 8, 9, 10}) {  // only need for cells, not triads
            for (uint32_t value = 0; value < 9; value++) {
                Cells16 &mask = cell_assignment_eliminations[i][value];
                for (int j = 0; j < 15; j++) {
                    if (j == i) { // asserted cell: clear all bits but the one asserted
                        mask.Insert(j, kAll ^ (1u << value));
                    } else if (j / 4 < 3 && j % 4 < 3) { // conflict cell: clear the asserted bit
                        mask.Insert(j, 1u << value);
                    } else if (j / 4 == i / 4 || j % 4 == i % 4) { // clear 2 negative triads
                        mask.Insert(j, 1u << value);
                    }
                }
            }
        }
        for (int i = 0; i < 81; i++) box_indexing[i] = BoxIndexing{i};
    }
};

const Tables tables{};

struct SolverDpllTriadSimd {
    State solution_{};
    size_t limit_ = 1;
    size_t num_solutions_ = 0;
    size_t num_guesses_ = 0;

    static bool BoxEliminate(State &state, int box_idx, const Cells16 &eliminations,
                             int from_vertical = 0) {
        // return immediately if there are no new eliminations
        Box &box = state.boxen[box_idx];
        auto eliminating = box.cells & eliminations;
        if (eliminating.AllZero()) return true;

        Cells08 h_band_eliminations, v_band_eliminations;
        do {
            // apply eliminations and check that no cell clause now violates its minimum
            box.cells ^= eliminating;
            Cells16 counts = box.cells.Popcounts9();
            const Cells16 box_minimums{1, 1, 1, 6, 1, 1, 1, 6, 1, 1, 1, 6, 6, 6, 6, 0};
            if (counts.AnyLessThan(box_minimums)) return false;

            // gather literals asserted by triggered cell clauses
            Cells16 triggered = counts.Equal(box_minimums);
            Cells16 all_assertions = box.cells & triggered;
            // and add literals asserted by triggered triad definition clauses
            GatherTriadClauseAssertions(
                    box.cells, [](const Cells16 &x) { return x.RotateRows(); }, all_assertions);
            GatherTriadClauseAssertions(
                    box.cells, [](const Cells16 &x) { return x.RotateCols(); }, all_assertions);

            // construct elimination messages for this box and for our band peers
            Cells16 self_eliminations;
            AssertionsToEliminations(all_assertions, box.box_i, box.box_j, self_eliminations,
                                     h_band_eliminations, v_band_eliminations);

            // repeat if we're propagating new eliminations in this box
            eliminating = box.cells & self_eliminations;
        } while (!eliminating.AllZero());

        // send elimination messages to peers
        if (from_vertical) {
            return BandEliminate(state, 0, box.box_i, h_band_eliminations, box.box_j) &&
                   BandEliminate(state, 1, box.box_j, v_band_eliminations, box.box_i);
        } else {
            return BandEliminate(state, 1, box.box_j, v_band_eliminations, box.box_i) &&
                   BandEliminate(state, 0, box.box_i, h_band_eliminations, box.box_j);
        }
    }

    // input cells contains zeros where nothing is asserted, a single candidate for regular cells
    // that are being asserted, and either 1 or 6 candidates for negative triad literals that are
    // being asserted.
    static inline void AssertionsToEliminations(const Cells16 &assertions, int box_i, int box_j,
                                                Cells16 &box_eliminations,
                                                Cells08 &h_band_eliminations,
                                                Cells08 &v_band_eliminations) {
        // we could guard some or all of the code below with checks that the assertion vector as
        // a whole, or cell or negative triad components of it, are nonzero. but the branches
        // would most often be taken. it's cheaper to compute no-op updates than it is to pay the
        // branch cost.

        // update the self eliminations for new assertions in the box.
        auto cell_assertions_only = assertions & tables.cell3x3_mask;
        // compute matrix broadcasting all assertions across the rows in which they occur.
        Cells16 across_rows = cell_assertions_only;
        across_rows |= across_rows.RotateRows();
        across_rows |= across_rows.RotateRows2();
        // compute matrix broadcasting all assertions across the cols in which they occur.
        Cells16 across_cols = cell_assertions_only;
        across_cols |= across_cols.RotateCols();
        across_cols |= across_cols.RotateCols2();
        // compute matrix broadcasting all assertions everywhere.
        Cells16 across_box = across_rows;
        across_box |= across_box.RotateCols();
        across_box |= across_box.RotateCols2();
        // restrict to 3x3 cells; each now has elimination bits for anything asserted in any cell.
        across_box &= tables.cell3x3_mask;
        // merge back rows and columns to populate elimination bits for negative triad literals.
        across_box |= across_rows | across_cols;
        // for any cell that had an assertion populate all elimination bits (ok to include >kAll).
        across_box |= cell_assertions_only.NonZero();
        // then clear elimination bits for the candidates that were actually asserted.
        across_box ^= cell_assertions_only;
        box_eliminations |= across_box;

        // update band eliminations to reflect positive triad assertions. for positive triad
        // assertions we use shifts 1 and 2 in the mask table because we want to eliminate positive
        // triad candidates in the _other_ two peers.
        Cells08 h_peer = HorizontalPeer(across_rows);
        h_band_eliminations |=
                h_peer.Shuffle(tables.shuffle_peer_x_shift_to_config_mask[box_j][1]) |
                h_peer.Shuffle(tables.shuffle_peer_x_shift_to_config_mask[box_j][2]);
        Cells08 v_peer = VerticalPeer(across_cols);
        v_band_eliminations |=
                v_peer.Shuffle(tables.shuffle_peer_x_shift_to_config_mask[box_i][1]) |
                v_peer.Shuffle(tables.shuffle_peer_x_shift_to_config_mask[box_i][2]);

        // update band eliminations to reflect negative triad assertions. for negative triad
        // assertions we use shift 0 in the mask table because we are eliminating positive triad
        // candidates in the same peer.
        h_band_eliminations |= HorizontalPeer(assertions).Shuffle(
                tables.shuffle_peer_x_shift_to_config_mask[box_j][0]);
        v_band_eliminations |= VerticalPeer(assertions).Shuffle(
                tables.shuffle_peer_x_shift_to_config_mask[box_i][0]);
    }

    // extracts a Cells08 containing vertical triad candidates in positions 4, 5, and 6 for use
    // as an input to shuffling. the contents of other cells are ignored by the shuffle.
    static inline Cells08 VerticalPeer(const Cells16 &box_cells) {
        return box_cells.GetHi();
    }

    // extracts a Cells08 containing horizontal triad candidates in positions 4, 5, and 6 for
    // use as an input to shuffling. the contents of other cells are ignored by the shuffle.
    static inline Cells08 HorizontalPeer(const Cells16 &box_cells) {
        return (box_cells.GetLo().Shuffle({0xffff, 0xffff, 0xffff, 0xffff,
                                           shuf03, shuf07, 0xffff, 0xffff}) |
                box_cells.GetHi().Shuffle({0xffff, 0xffff, 0xffff, 0xffff,
                                           0xffff, 0xffff, shuf03, 0xffff}));
    }

    template<typename RotateFn>
    static inline void GatherTriadClauseAssertions(const Cells16 &cells,
                                                   RotateFn rotate, Cells16 &assertions) {
        // find 'one_or_more' and 'two_or_more', each a set of 4 row/col vectors depending on the
        // given rotation function, where each cell in a row/col contains the bits that occur 1+ or
        // 2+ times across the cells of the corresponding source row/col.
        auto one_or_more = cells;
        auto rotated = rotate(cells);
        auto two_or_more = one_or_more & rotated;
        one_or_more |= rotated;
        rotated = rotate(rotated);
        two_or_more |= one_or_more & rotated;
        one_or_more |= rotated;
        rotated = rotate(rotated);
        two_or_more |= one_or_more & rotated;
        one_or_more |= rotated;
        // we might check here that one_or_more == kAll, but the check is a net loss.

        // assert (in the cells where they remain) candidates that now occur only once an a row/col
        assertions |= one_or_more.and_not(two_or_more) & cells;
    }

    static bool BandEliminate(State &state, int vertical, int band_idx,
                              const Cells08 &eliminations, int from_peer = 0) {
        Band &band = state.bands[vertical][band_idx];
        const Cells08 eliminating = band.configurations & eliminations;
        if (eliminating.AllZero()) return true;
        // after eliminating we might check that every value is still consistent with some
        // configuration, but the check is a net loss.
        band.configurations ^= eliminating;

        Cells16 triads = ConfigurationsToPositiveTriads(band.configurations);
        // we might check here that every positive triad still has at least three candidates, but
        // the check is a net loss.
        const Cells16 counts = triads.Popcounts9();

        // we might repeat the updating of triads below until we no longer trigger new triad 3/
        // clauses. however, just once delivers most of the benefit, and it's best not to branch.
        const Cells16 asserting = triads & counts.Equal(Cells16::All(3));
        const Cells08 lo = asserting.GetLo();
        const Cells08 hi = asserting.GetHi();
        band.configurations = band.configurations.and_not(
                lo.RotateCols().Shuffle(tables.shuffle_peer_x_shift_to_config_mask[0][1]) |
                lo.RotateCols().Shuffle(tables.shuffle_peer_x_shift_to_config_mask[0][2]));
        band.configurations = band.configurations.and_not(
                lo.Shuffle(tables.shuffle_peer_x_shift_to_config_mask[1][1]) |
                lo.Shuffle(tables.shuffle_peer_x_shift_to_config_mask[1][2]));
        band.configurations = band.configurations.and_not(
                hi.RotateCols().Shuffle(tables.shuffle_peer_x_shift_to_config_mask[2][1]) |
                hi.RotateCols().Shuffle(tables.shuffle_peer_x_shift_to_config_mask[2][2]));
        triads = ConfigurationsToPositiveTriads(band.configurations);

        // convert positive triads to box elimination messages and send to peers
        const Cells16 box_eliminations[3]{
                PositiveTriadsToBoxEliminations(triads.GetLo(), vertical),
                PositiveTriadsToBoxEliminations(triads.GetLo().RotateCols(), vertical),
                PositiveTriadsToBoxEliminations(triads.GetHi(), vertical)};
        const int order[10]{1, 2, 0, 1, 2, 0, 1, 2, 0, 1};
        const int peer[3]{order[from_peer], order[from_peer + 1], order[from_peer + 2]};
        return BoxEliminate(state, band.box_peers[peer[0]], box_eliminations[peer[0]], vertical) &&
               BoxEliminate(state, band.box_peers[peer[1]], box_eliminations[peer[1]], vertical) &&
               BoxEliminate(state, band.box_peers[peer[2]], box_eliminations[peer[2]], vertical);
    }

    static inline Cells16 ConfigurationsToPositiveTriads(const Cells08 &configurations) {
        Cells16 tmp{configurations, configurations};
        return tmp.Shuffle(tables.shuffle_configs_to_triads[0]) |
               tmp.Shuffle(tables.shuffle_configs_to_triads[1]);
    }

    static inline Cells16 PositiveTriadsToBoxEliminations(const Cells08 &triads, int orientation) {
        Cells08 tmp1 = Cells08::All(kAll) ^(triads.Shuffle(tables.shuffle_triads_temp[0]) |
                                            triads.Shuffle(tables.shuffle_triads_temp[1]));
        return Cells16{tmp1, tmp1}.Shuffle(tables.shuffle_temp_to_box_mask[orientation]);
    }

    ///////////////////////////////////////////////////////////////////////////////////

    const uint32_t div3[6]{0, 0, 0, 1, 1, 1};
    const uint32_t mod3[6]{0, 1, 2, 0, 1, 2};

    inline pair<uint32_t, uint32_t> ChooseBandAndValueToBranch(const State &state) {
        uint32_t best_band = UINT32_MAX, best_band_count = UINT32_MAX;
        uint32_t best_value = UINT32_MAX, best_value_count = UINT32_MAX;
        // first find the unfixed band with the fewest possible configurations across all values.
        for (auto i = 0u; i < 6; i++) {
            const Band &band = state.bands[div3[i]][mod3[i]];
            auto band_count = (uint32_t) band.configurations.Popcount();
            if (band_count > 9 && band_count < best_band_count) {
                best_band_count = band_count;
                best_band = i;
            }
        }
        // then choose the unfixed value with the fewest possible configurations within the band.
        if (best_band < UINT32_MAX) {
            const Band &band = state.bands[div3[best_band]][mod3[best_band]];
            for (uint32_t i = 0; i < 9; i++) {
                auto value_count = (uint32_t) (band.configurations & tables.one_value_mask[i]).Popcount();
                if (value_count > 1 && value_count < best_value_count) {
                    best_value_count = value_count;
                    best_value = i;
                    if (best_value_count == 2) break;
                }
            }
        }
        return {best_band, best_value};
    }

    void BranchOnBandAndValue(int orientation, int band_idx, int value, State &state) {
        Band &band = state.bands[orientation][band_idx];
        // we enter with two or more possible configurations for this value
        Cells08 configurations = band.configurations & tables.one_value_mask[value];

        num_guesses_++;
        State state_copy = state;
        Cells08 assignment1_mask = configurations.ClearLowBit();
        if (BandEliminate(state_copy, orientation, band_idx, assignment1_mask)) {
            CountSolutionsConsistentWithPartialAssignment(state_copy);
            if (num_solutions_ == limit_) return;
        }

        Cells08 negation1_mask = configurations ^assignment1_mask;
        if (BandEliminate(state, orientation, band_idx, negation1_mask)) {
            if ((band.configurations & tables.one_value_mask[value]).Popcount() == 1) {
                CountSolutionsConsistentWithPartialAssignment(state);
            } else {
                BranchOnBandAndValue(orientation, band_idx, value, state);
            }
        }
    }

    void CountSolutionsConsistentWithPartialAssignment(State &state) {
        auto band_and_value = ChooseBandAndValueToBranch(state);
        if (band_and_value.second == UINT32_MAX) {
            num_solutions_++;
            if (limit_ == 1) solution_ = state;
        } else {
            BranchOnBandAndValue(div3[band_and_value.first], mod3[band_and_value.first],
                                 band_and_value.second, state);
        }
    }

    ///////////////////////////////////////////////////////////////////////////////////

    static inline void InitClue(const char *input, State &state, int pos,
                                Cells08 (&h_eliminations)[3], Cells08 (&v_eliminations)[3]) {
        const BoxIndexing &indexing = tables.box_indexing[pos];
        char digit = input[pos];
        uint16_t candidate = 1u << (uint32_t) (digit - '1');
        // perform eliminations for digit in box, but don't propagate
        state.boxen[indexing.box].cells = state.boxen[indexing.box].cells.and_not(
                tables.cell_assignment_eliminations[indexing.elem][digit - '1']);
        // merge all band eliminations; we'll propagate these below.
        h_eliminations[indexing.box_i] |=
                tables.peer_x_elem_to_config_mask[indexing.box_j][indexing.elem_i] &
                Cells08::All(candidate);
        v_eliminations[indexing.box_j] |=
                tables.peer_x_elem_to_config_mask[indexing.box_i][indexing.elem_j] &
                Cells08::All(candidate);
    }

    // We could set the initial clues in other ways, including one box update for each clue, or
    // one batch box update for each box. But it's fastest to start with 6 batched band updates.
    static bool InitBandBatch(const char *input, State &state) {
        Cells08 h_eliminations[3]{};
        Cells08 v_eliminations[3]{};

        __m128i dots = _mm_set1_epi8('.');
        for (int i = 0; i < 5; i++) {
            __m128i str16 = _mm_loadu_si128((const __m128i *) &input[i * 16]);
            uint32_t clues = (uint32_t) _mm_movemask_epi8(_mm_cmpeq_epi8(str16, dots)) ^0xffffu;
            while (clues) {
                int cell_idx = i * 16 + LowOrderBitIndex(clues);
                InitClue(input, state, cell_idx, h_eliminations, v_eliminations);
                clues = ClearLowBit(clues);
            }
        }
        if (input[80] != '.') {
            InitClue(input, state, 80, h_eliminations, v_eliminations);
        }

        return BandEliminate(state, 0, 0, h_eliminations[0], 1) &&
               BandEliminate(state, 1, 0, v_eliminations[0], 1) &&
               BandEliminate(state, 0, 1, h_eliminations[1], 2) &&
               BandEliminate(state, 1, 1, v_eliminations[1], 2) &&
               BandEliminate(state, 0, 2, h_eliminations[2], 0) &&
               BandEliminate(state, 1, 2, v_eliminations[2], 0);
    }

    static inline void ExtractTriad(uint64_t triad, int triad_base, char *solution) {
        solution[triad_base + 0] = char('1' + LowOrderBitIndex(triad >> 0u));
        solution[triad_base + 1] = char('1' + LowOrderBitIndex(triad >> 16u));
        solution[triad_base + 2] = char('1' + LowOrderBitIndex(triad >> 32u));
    }

    static void ExtractSolution(const State &state, char *solution) {
        for (const Box &box : state.boxen) {
            auto box_triads = box.cells.As_4x64();
            int box_base = box.box_i * 27 + box.box_j * 3;
            ExtractTriad(box_triads.x0, box_base, solution);
            ExtractTriad(box_triads.x1, box_base + 9, solution);
            ExtractTriad(box_triads.x2, box_base + 18, solution);
        }
    }

    size_t SolveSudoku(const char *input, size_t limit,
                    char *solution, size_t *const num_guesses) {
        limit_ = limit;
        num_solutions_ = 0;
        num_guesses_ = 0;

        State state;
        if (InitBandBatch(input, state)) {
            CountSolutionsConsistentWithPartialAssignment(state);
            if (limit_ == 1) ExtractSolution(solution_, solution);
        }
        *num_guesses = num_guesses_;
        return num_solutions_;
    };
};

SolverDpllTriadSimd solver{};

} // namespace

extern "C"
size_t TdokuSolverDpllTriadSimd(const char *input, size_t limit,
                                uint32_t /* unused_configuration */,
                                char *solution, size_t *num_guesses) {
    return solver.SolveSudoku(input, limit, solution, num_guesses);
}
