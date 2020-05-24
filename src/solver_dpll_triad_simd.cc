#include "bitutil.h"
#include "simd_vectors.h"
#include "util.h"

#include <array>
#include <cstring>

#define LIKELY(x) __builtin_expect(!!(x),1)

using namespace std;

namespace {

using Cells08 = Bitvec08x16;
using Cells16 = Bitvec16x16;

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
//  or column of the matrix corresponding to the biconditional defining the triad.
//
//  Each cell also has a minimum. So there are three sets of clauses represented here.
//
struct Box {
    Cells16 cells{Cells16::All(kAll)};
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
// We'll also store with the Band a vector of eliminations          +---+---+---+---+
// to be applied to the Band's configurations on the next
// call to BandEliminate. This allows us to apply all pending updates to a band at
// the first opportunity instead of individually depending on where in the call stack
// the update originates.
//
// Note that we might do the same thing for Boxes, and doing so is beneficial for easy
// puzzles. Unfortunately, it's a net loss for hard puzzles. The cost in State size
// is higher, and the benefit is lower (the benefit for Bands chiefly arises from the
// way we do puzzle initialization).
//
struct Band {
    Cells08 configurations{kAll, kAll, kAll, kAll, kAll, kAll, 0, 0};
    Cells08 eliminations{};
};

struct State {
    Band bands[2][3]{};
    Box boxen[9]{};
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

// We depend on low-level shuffle operations that address packed 8-bit integers, but we're
// always shuffling 16-bit logical cells. These constants are used for constructing shuffle
// control vectors that address these cells. We only require 8 of them since even 256-bit
// shuffles operate within 128-bit lanes.
constexpr uint16_t shuf00 = 0x0100, shuf01 = 0x0302, shuf02 = 0x0504, shuf03 = 0x0706;
constexpr uint16_t shuf04 = 0x0908, shuf05 = 0x0b0a, shuf06 = 0x0d0c, shuf07 = 0x0f0e;

struct Tables {
    // @formatter:off

    // used when assigning a candidate during initialization
    Cells16 cell_assignment_eliminations[16][16]{};

    //   config       0       1       2       3       4       5
    //    elem      0 1 2   0 1 2   0 1 2   0 1 2   0 1 2   0 1 2
    //            +-------+-------+-------+-------+-------+-------+
    //   peer0    | X . . | . X . | . . X | . . X | X . . | . X . |
    //   peer1    | . X . | . . X | X . . | . X . | . . X | X . . |
    //   peer2    | . . X | X . . | . X . | X . . | . X . | . . X |
    //            +-------+-------+-------+-------+-------+-------+
    //
    // tables for constructing band elimination messages from Cells08 containing
    // positive or negative triad views of a box stored positions 4, 5, and 6.
    // each table has three shuffle control vectors, one for each of the band's box
    // peers. there are three tables, each corresponding to a rotation of elements
    // in the peer. look first at the shift0 table to see the correspondence with
    // the configuration diagram reproduced above.
    //
    const Cells08 triads_shift0_to_config_elims[3]{
            {shuf04, shuf05, shuf06, shuf06, shuf04, shuf05, 0xffff, 0xffff},
            {shuf05, shuf06, shuf04, shuf05, shuf06, shuf04, 0xffff, 0xffff},
            {shuf06, shuf04, shuf05, shuf04, shuf05, shuf06, 0xffff, 0xffff}
    };
    const Cells08 triads_shift1_to_config_elims[3]{
            {shuf05, shuf06, shuf04, shuf04, shuf05, shuf06, 0xffff, 0xffff},
            {shuf06, shuf04, shuf05, shuf06, shuf04, shuf05, 0xffff, 0xffff},
            {shuf04, shuf05, shuf06, shuf05, shuf06, shuf04, 0xffff, 0xffff}
    };
    const Cells08 triads_shift2_to_config_elims[3]{
            {shuf06, shuf04, shuf05, shuf05, shuf06, shuf04, 0xffff, 0xffff},
            {shuf04, shuf05, shuf06, shuf04, shuf05, shuf06, 0xffff, 0xffff},
            {shuf05, shuf06, shuf04, shuf06, shuf04, shuf05, 0xffff, 0xffff}
    };

    // two Cells16 shuffle control vectors whose results are or'ed together to convert
    // a vector of configurations (reproduced across 128 bit lanes) into a 3x3 matrix of
    // positive triads (refer again to the configuration diagram above).
    const Cells16 shuffle_configs_to_triads[2]{
            {{shuf00, shuf01, shuf02, 0xffff,
              shuf02, shuf00, shuf01, 0xffff},
             {shuf01, shuf02, shuf00, 0xffff,
              0xffff, 0xffff, 0xffff, 0xffff}},
            {{shuf04, shuf05, shuf03, 0xffff,
              shuf05, shuf03, shuf04, 0xffff},
             {shuf03, shuf04, shuf05, 0xffff,
              0xffff, 0xffff, 0xffff, 0xffff}}
    };

    // two pairs of two Cells16 shuffle control vectors whose results are or'ed together to
    // convert vectors of positive triads in positions 0, 1, and 2 (reproduced across 128 bit
    // lanes) into box candidate sets. it is necessary to combine two shuffles because box
    // negative triads are eliminated when band positive triads have been eliminated in the
    // other two shifted positions. the shuffled input has 0xffff in position 3 to allow a
    // no-op for triads with opposite orientation.
    const Cells16 pos_triads_to_candidates[2][2]{
            // horizontal
            {{{shuf00, shuf00, shuf00, shuf01,
               shuf01, shuf01, shuf01, shuf02},
              {shuf02, shuf02, shuf02, shuf00,
               shuf03, shuf03, shuf03, shuf03}},
             {{shuf00, shuf00, shuf00, shuf02,
               shuf01, shuf01, shuf01, shuf00},
              {shuf02, shuf02, shuf02, shuf01,
               shuf03, shuf03, shuf03, shuf03}}},
            // vertical
            {{{shuf00, shuf01, shuf02, shuf03,
               shuf00, shuf01, shuf02, shuf03},
              {shuf00, shuf01, shuf02, shuf03,
               shuf01, shuf02, shuf00, shuf03}},
             {{shuf00, shuf01, shuf02, shuf03,
               shuf00, shuf01, shuf02, shuf03},
              {shuf00, shuf01, shuf02, shuf03,
               shuf02, shuf00, shuf01, shuf03}}}
    };

    const Cells16 cell3x3_mask{ kAll, kAll, kAll,    0,
                                kAll, kAll, kAll,    0,
                                kAll, kAll, kAll,    0,
                                   0,    0,    0,    0
    };
    // row rotation shuffle controls vectors for just the 3x3 submatrix of a Cells16
    const Cells16 row_rotate_3x3_1{
        shuf01, shuf02, shuf00, shuf03, shuf05, shuf06, shuf04, shuf07,
        shuf01, shuf02, shuf00, shuf03, shuf04, shuf05, shuf06, shuf07};
    const Cells16 row_rotate_3x3_2{
        shuf02, shuf00, shuf01, shuf03, shuf06, shuf04, shuf05, shuf07,
        shuf02, shuf00, shuf01, shuf03, shuf04, shuf05, shuf06, shuf07};


    //   config       0       1       2       3       4       5
    //    elem      0 1 2   0 1 2   0 1 2   0 1 2   0 1 2   0 1 2
    //            +-------+-------+-------+-------+-------+-------+
    //   peer0    | X . . | . X . | . . X | . . X | X . . | . X . |
    //   peer1    | . X . | . . X | X . . | . X . | . . X | X . . |
    //   peer2    | . . X | X . . | . X . | X . . | . X . | . . X |
    //            +-------+-------+-------+-------+-------+-------+
    //
    // A set of masks for eliminating band configurations inconsistent with the placement
    // of a digit in an element (minirow or minicol) of a box peer. Refer again to the
    // configuration diagram.
    //
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

    const int box_peers[2][3][3]{
            {{0, 1, 2}, {3, 4, 5}, {6, 7, 8}},
            {{0, 3, 6}, {1, 4, 7}, {2, 5, 8}}
    };
    const int div3[9]{ 0, 0, 0, 1, 1, 1, 2, 2, 2 };
    const int mod3[9]{ 0, 1, 2, 0, 1, 2, 0, 1, 2 };

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

template<int solution_mode>
struct SolverDpllTriadSimd {
    State solution_{};
    size_t limit_ = 1;
    size_t num_solutions_ = 0;
    size_t num_guesses_ = 0;
    void (*callback_)(const char *) = 0;

    // restrict the cell, minirow, and minicol clauses of the box to contain only the given
    // cell and triad candidates.
    template<int from_vertical>
    static bool BoxRestrict(State &state, int box_idx, const Cells16 &candidates) {
        // return immediately if there are no new eliminations
        Box &box = state.boxen[box_idx];
        auto eliminating = box.cells.and_not(candidates);
        if (eliminating.AllZero()) return true;

        int box_i = tables.div3[box_idx];
        int box_j = tables.mod3[box_idx];

        Band &h_band = state.bands[0][box_i];
        Band &v_band = state.bands[1][box_j];
        do {
            // apply eliminations and check that no cell clause now violates its minimum
            box.cells = box.cells.and_not(eliminating);
            Cells16 counts = box.cells.Popcounts9();
            const Cells16 box_minimums{1, 1, 1, 6, 1, 1, 1, 6, 1, 1, 1, 6, 6, 6, 6, 0};
            if (counts.AnyLessThan(box_minimums)) return false;

            // gather literals asserted by triggered cell clauses
            Cells16 triggered = counts.WhichEqual(box_minimums);
            Cells16 all_assertions = box.cells & triggered;
            // and add literals asserted by triggered triad definition clauses
            GatherTriadClauseAssertions(
                    box.cells, [](const Cells16 &x) { return x.RotateRows(); }, all_assertions);
            GatherTriadClauseAssertions(
                    box.cells, [](const Cells16 &x) { return x.RotateCols(); }, all_assertions);

            // construct elimination messages for this box and for our band peers
            AssertionsToEliminations(all_assertions, box_i, box_j, eliminating,
                                     h_band.eliminations, v_band.eliminations);

        } while (eliminating.Intersects(box.cells));

        // send elimination messages to horizontal and vertical peers. Prefer to send the first
        // of these messages to the peer whose orientation is opposite that of the inbound peer.
        if (from_vertical) {
            return BandEliminate<0>(state, box_i, box_j) &&
                   BandEliminate<1>(state, box_j, box_i);
        } else {
            return BandEliminate<1>(state, box_j, box_i) &&
                   BandEliminate<0>(state, box_i, box_j);
        }
    }

    // input Cells16 contains zeros where nothing is asserted, a single candidate for regular cells
    // that are being asserted, and either 1 or 6 candidates for negative triad literals that are
    // being asserted (due to an unsatisfiable triad definition, or due to a 6/ minimum).
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
        // compute matrices broadcasting assertions across rows and columns in which they occur.
        Cells16 across_rows = cell_assertions_only;
        across_rows |= across_rows.RotateRows();
        across_rows |= across_rows.RotateRows2();
        Cells16 across_cols = cell_assertions_only;
        across_cols |= across_cols.RotateCols();
        across_cols |= across_cols.RotateCols2();
        // let 3x3 submatrix have assertions occuring anywhere
        Cells16 new_box_eliminations = Cells16::or_X_Y_or_Z(across_cols,
                                                            across_cols.Shuffle(tables.row_rotate_3x3_1),
                                                            across_cols.Shuffle(tables.row_rotate_3x3_2));
        // join 3x3 submatrix, row/col margins, and all elimination bits in asserted cells
        new_box_eliminations = Cells16::or_X_Y_or_Z(
                new_box_eliminations, across_rows, cell_assertions_only.WhichNonZero());
        // then apply after clearing elimination bits for the asserted candidates.
        box_eliminations = Cells16::xor_X_Y_or_Z(
                new_box_eliminations, cell_assertions_only, box_eliminations);

        // below we'll update band eliminations to reflect assertion of negative triads or positive
        // literals within this box. in the case of asserted negative triads we'll eliminate the
        // corresponding positive triads in the band (at shift 0).
        Cells16 hv_neg_triad_assertions{HorizontalTriads(assertions),
                                        VerticalTriads(assertions)};
        // in the case of asserted positive literals, which imply the assertion of corresponding
        // shift 0 positive triads, we'll eliminate the triads at shifts 1 and 2 in the band.
        Cells16 hv_pos_triad_assertions{HorizontalTriads(new_box_eliminations),
                                        VerticalTriads(new_box_eliminations)};
        Cells16 new_eliminations = Cells16::or_X_Y_or_Z(
                hv_neg_triad_assertions.Shuffle(
                        Bitvec16x16{tables.triads_shift0_to_config_elims[box_j],
                                    tables.triads_shift0_to_config_elims[box_i]}),
                hv_pos_triad_assertions.Shuffle(
                        Bitvec16x16{tables.triads_shift1_to_config_elims[box_j],
                                    tables.triads_shift1_to_config_elims[box_i]}),
                hv_pos_triad_assertions.Shuffle(
                        Bitvec16x16{tables.triads_shift2_to_config_elims[box_j],
                                    tables.triads_shift2_to_config_elims[box_i]}));
        h_band_eliminations |= new_eliminations.GetLo();
        v_band_eliminations |= new_eliminations.GetHi();
    }

    // extracts a Cells08 containing (positive or negative) vertical triad literals in positions
    // 4, 5, and 6 for use in shuffling an elimination message to send the vertical band peer. the
    // contents of other cells are ignored by the shuffle.
    static inline Cells08 VerticalTriads(const Cells16 &cells) {
        return cells.GetHi();
    }

    // extracts a Cells08 containing (positive or negative) horizontal triad literals in positions
    // 4, 5, and 6 for use in shuffling an elimination message to send the horizontal band peer. we
    // use positions 4,5,6 so we can use the same tables in creating horizontal and vertical
    // elimination messages (and so the vertical triads can be extracted at no cost).
    static inline Cells08 HorizontalTriads(const Cells16 &cells) {
        Cells16 split_triads = cells.Shuffle(
                Bitvec16x16{{0xffff, 0xffff, 0xffff, 0xffff, shuf03, shuf07, 0xffff, 0xffff},
                            {0xffff, 0xffff, 0xffff, 0xffff, 0xffff, 0xffff, shuf03, 0xffff}});
        return split_triads.GetLo() | split_triads.GetHi();
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
        two_or_more = Cells16::and_X_Y_or_Z(one_or_more, rotated, two_or_more);
        one_or_more |= rotated;
        rotated = rotate(rotated);
        two_or_more = Cells16::and_X_Y_or_Z(one_or_more, rotated, two_or_more);
        one_or_more |= rotated;
        // we might check here that one_or_more == kAll, but the check is a net loss.
        // now assert (in cells where they remain) candidates that occur only once an a row/col.
        assertions |= Cells16::and_X_Y_andnot_Z(cells, one_or_more, two_or_more);
    }

    template<int vertical>
    static bool BandEliminate(State &state, int band_idx, int from_peer = 0) {
        Band &band = state.bands[vertical][band_idx];
        Cells08 eliminating = band.configurations & band.eliminations;
        if (LIKELY(eliminating.AllZero())) return true;
        // after eliminating we might check that every value is still consistent with some
        // configuration, but the check is a net loss.
        band.configurations ^= eliminating;

        Cells16 triads = ConfigurationsToPositiveTriads(band.configurations);
        // we might check here that every cell (corresponding to a minirow or minicol) still has
        // at least three triad candidates, but the check is a net loss.
        Cells16 counts = triads.Popcounts9();

        // we might repeat the updating of triads below until we no longer trigger new triad 3/
        // clauses. however, just once delivers most of the benefit, and it's best not to branch.
        Cells16 asserting = triads & counts.WhichEqual(Cells16::All(3));
        Cells08 lo = asserting.GetLo();
        Cells08 hi = asserting.GetHi();
        band.configurations = band.configurations.and_not(Cells08::or_X_Y_or_Z(
                lo.RotateCols().Shuffle(tables.triads_shift1_to_config_elims[0]),
                lo.RotateCols().Shuffle(tables.triads_shift2_to_config_elims[0]),
                lo.Shuffle(tables.triads_shift1_to_config_elims[1])));
        band.configurations = band.configurations.and_not(Cells08::or_X_Y_or_Z(
                lo.Shuffle(tables.triads_shift2_to_config_elims[1]),
                hi.RotateCols().Shuffle(tables.triads_shift1_to_config_elims[2]),
                hi.RotateCols().Shuffle(tables.triads_shift2_to_config_elims[2])));
        triads = ConfigurationsToPositiveTriads(band.configurations);

        // convert positive triads to box restriction messages and send to the three box peers.
        // send these messages in order so that we return to the inbound peer last.
        int peer[3]{tables.mod3[from_peer + 1], tables.mod3[from_peer + 2], from_peer};
        auto &box_peers = tables.box_peers[vertical][band_idx];
        Cells08 peer_triads[3]{ triads.GetLo(), triads.GetLo().RotateCols(), triads.GetHi() };
        return (BoxRestrict<vertical>(state, box_peers[peer[0]],
                        PositiveTriadsToBoxCandidates(peer_triads[peer[0]], vertical)) &&
                BoxRestrict<vertical>(state, box_peers[peer[1]],
                        PositiveTriadsToBoxCandidates(peer_triads[peer[1]], vertical)) &&
                BoxRestrict<vertical>(state, box_peers[peer[2]],
                        PositiveTriadsToBoxCandidates(peer_triads[peer[2]], vertical)));
    }

    // convert band configuration into an equivalent 3x3 matrix of positive triad candidates,
    // where each row represents the constraints the band imposes on a given box peer.
    static inline Cells16 ConfigurationsToPositiveTriads(const Cells08 &configurations) {
        Cells16 tmp{configurations, configurations};
        return tmp.Shuffle(tables.shuffle_configs_to_triads[0]) |
               tmp.Shuffle(tables.shuffle_configs_to_triads[1]);
    }

    // convert 3 sets of positive triads (found in cells 0,1,2 of the given Cells08) into a
    // mask for restricting the corresponding box peer.
    static inline Cells16 PositiveTriadsToBoxCandidates(const Cells08 &triads, int orientation) {
        Cells08 triads_with_kAll = triads | Cells08{0x0, 0x0, 0x0, kAll, 0x0, 0x0, 0x0, 0x0};
        Cells16 tmp{triads_with_kAll, triads_with_kAll};
        return tmp.Shuffle(tables.pos_triads_to_candidates[orientation][0]) |
               tmp.Shuffle(tables.pos_triads_to_candidates[orientation][1]);
    }

    ///////////////////////////////////////////////////////////////////////////////////

    static constexpr uint32_t NONE = UINT32_MAX;

    static inline pair<uint32_t, uint32_t> ChooseBandAndValueToBranch(const State &state) {
        uint32_t best_band = NONE, best_band_count = NONE;
        uint32_t best_value = NONE, best_value_count = NONE;

        // first find the unfixed band with the fewest possible configurations across all values.
        uint32_t config_minpos = Bitvec08x16{
                (uint16_t)state.bands[0][0].configurations.Popcount(),
                (uint16_t)state.bands[0][1].configurations.Popcount(),
                (uint16_t)state.bands[0][2].configurations.Popcount(),
                (uint16_t)state.bands[1][0].configurations.Popcount(),
                (uint16_t)state.bands[1][1].configurations.Popcount(),
                (uint16_t)state.bands[1][2].configurations.Popcount(),
                (uint16_t)0xffff,
                (uint16_t)0xffff
        }.MinPosGreaterThanOrEqual(10);

        if ((config_minpos & 0xff00u) == 0) {
            best_band = config_minpos >> 16u;
            const auto &configurations =
                    state.bands[tables.div3[best_band]][tables.mod3[best_band]].configurations;
            uint32_t value_minpos = Bitvec08x16{
                    (uint16_t)(configurations & tables.one_value_mask[0]).Popcount(),
                    (uint16_t)(configurations & tables.one_value_mask[1]).Popcount(),
                    (uint16_t)(configurations & tables.one_value_mask[2]).Popcount(),
                    (uint16_t)(configurations & tables.one_value_mask[3]).Popcount(),
                    (uint16_t)(configurations & tables.one_value_mask[4]).Popcount(),
                    (uint16_t)(configurations & tables.one_value_mask[5]).Popcount(),
                    (uint16_t)(configurations & tables.one_value_mask[6]).Popcount(),
                    (uint16_t)(configurations & tables.one_value_mask[7]).Popcount()
            }.MinPosGreaterThanOrEqual(2);

            best_value = value_minpos >> 16u;
            uint32_t excess_over_two = value_minpos & 0xffffu;
            if (excess_over_two) {
                uint32_t last_count = (configurations & tables.one_value_mask[8]).Popcount();
                if (last_count > 1 && last_count < excess_over_two + 2) {
                    best_value = 8;
                }
            }
        }
        return {best_band, best_value};
    }

    template<int vertical>
    void BranchOnBandAndValue(int band_idx, int value, State &state) {
        Band &band = state.bands[vertical][band_idx];
        // we enter with two or more possible configurations for this value
        Cells08 configurations = band.configurations & tables.one_value_mask[value];
        // assert the first configuration
        num_guesses_++;
        State state_copy = state;
        Cells08 assignment1_mask = configurations.ClearLowBit();
        state_copy.bands[vertical][band_idx].eliminations |= assignment1_mask;
        if (BandEliminate<vertical>(state_copy, band_idx)) {
            CountSolutionsConsistentWithPartialAssignment(state_copy);
            if (num_solutions_ == limit_) return;
        }
        // now negate the first configuration
        Cells08 negation1_mask = configurations ^assignment1_mask;
        state.bands[vertical][band_idx].eliminations |= negation1_mask;
        if (BandEliminate<vertical>(state, band_idx)) {
            if ((band.configurations & tables.one_value_mask[value]).Popcount() == 1) {
                CountSolutionsConsistentWithPartialAssignment(state);
            } else {
                // rarely after negating the first configuration we may still have more than one
                // left. in this case branch again on the same band and value, instead of going
                // through the process of choosing again.
                BranchOnBandAndValue<vertical>(band_idx, value, state);
            }
        }
    }

    // do not call this twice on the same state. for efficiency this count may modify the
    // given state instead of making copies. if called with limit > 1 this can leave the state
    // changed in a way that makes subsequent calls return different results.
    void CountSolutionsConsistentWithPartialAssignment(State &state) {
        auto band_and_value = ChooseBandAndValueToBranch(state);
        if (band_and_value.first == NONE) {
            num_solutions_++;
            if (solution_mode == 1 && num_solutions_ == limit_) solution_ = state;
            if (solution_mode == 2) ReportSolution(state);
        } else {
            if (band_and_value.first < 3) {
                BranchOnBandAndValue<0>(
                        tables.mod3[band_and_value.first], band_and_value.second, state);
            } else {
                BranchOnBandAndValue<1>(
                        tables.mod3[band_and_value.first], band_and_value.second, state);
            }
        }
    }

    size_t SafeCountSolutionsConsistentWithPartialAssignment(State state, size_t limit) {
        limit_ = limit;
        num_solutions_ = 0;
        CountSolutionsConsistentWithPartialAssignment(state);
        return num_solutions_;
    }

    ///////////////////////////////////////////////////////////////////////////////////

    static inline void InitClue(const char *input, State &state, int pos) {
        const BoxIndexing &indexing = tables.box_indexing[pos];
        char digit = input[pos];
        uint16_t candidate = 1u << (uint32_t) (digit - '1');
        // perform eliminations for the clue in its own box, but don't propagate. this is
        // not strictly necessary since band eliminations will constrain the puzzle, but it
        // turns out to be important for performance on invalid zero-solution puzzles.
        state.boxen[indexing.box].cells = state.boxen[indexing.box].cells.and_not(
                tables.cell_assignment_eliminations[indexing.elem][digit - '1']);
        // merge band eliminations; we'll propagate after all clue are processed.
        state.bands[0][indexing.box_i].eliminations = Cells08::and_X_Y_or_Z(
                tables.peer_x_elem_to_config_mask[indexing.box_j][indexing.elem_i],
                Cells08::All(candidate),
                state.bands[0][indexing.box_i].eliminations);
        state.bands[1][indexing.box_j].eliminations = Cells08::and_X_Y_or_Z(
                tables.peer_x_elem_to_config_mask[indexing.box_i][indexing.elem_j],
                Cells08::All(candidate),
                state.bands[1][indexing.box_j].eliminations);
    }

    // We could set the initial clues in other ways, including one box update for each clue, or
    // one batch box update for each box. But it's fastest to start with 6 batched band updates.
    static bool InitVanillaByBand(const char *input, State &state) {
        uint64_t clues64 = WhichDots64(input) ^ (uint64_t)-1ll;
        while (clues64) {
            int cell_idx = LowOrderBitIndex64(clues64);
            InitClue(input, state, cell_idx);
            clues64 = ClearLowBit64(clues64);
        }
        uint32_t clues16 = WhichDots16(input + 64) ^ 0xffffu;
        while (clues16) {
            int cell_idx = 64 + LowOrderBitIndex(clues16);
            InitClue(input, state, cell_idx);
            clues16 = ClearLowBit(clues16);
        }
        if (input[80] != '.') {
            InitClue(input, state, 80);
        }
        // thanks to the merging of band updates the puzzle is almost always fully initialized
        // after the first of these calls. most will be no-ops, but we've still got to do them
        // since this cannot be guaranteed.
        return BandEliminate<0>(state, 0, 1) && BandEliminate<1>(state, 0, 1) &&
               BandEliminate<0>(state, 1, 2) && BandEliminate<1>(state, 1, 2) &&
               BandEliminate<0>(state, 2, 0) && BandEliminate<1>(state, 2, 0);
    }

    static bool InitPencilmarkByBox(const char *input, State &state) {
        char buf[736]; // make sure unaligned 16 byte cell reads won't go past end of buffer
        memcpy(&buf, input, 729);
        for (int box_i = 0; box_i < 3; box_i++) {
            for (int box_j = 0; box_j < 3; box_j++) {
                Cells16 box_candidates = Cells16::All(kAll);
                for (int elm_i = 0; elm_i < 3; elm_i++) {
                    for (int elm_j = 0; elm_j < 3; elm_j++) {
                        int cell = box_i * 27 + elm_i * 9 + box_j * 3 + elm_j;
                        auto cell_eliminations = WhichDots16(&buf[cell * 9]);
                        box_candidates.Insert(elm_i * 4 + elm_j, kAll & ~cell_eliminations);
                    }
                }
                if (!BoxRestrict<0>(state, box_i * 3 + box_j, box_candidates)) return false;
            }
        }
        return true;
    }

    static inline void ExtractMiniRow(uint64_t minirow, int minirow_base, char *solution) {
        solution[minirow_base + 0] = char('1' + LowOrderBitIndex(minirow >> 0u));
        solution[minirow_base + 1] = char('1' + LowOrderBitIndex(minirow >> 16u));
        solution[minirow_base + 2] = char('1' + LowOrderBitIndex(minirow >> 32u));
    }

    static void ExtractSolution(const State &state, char *solution) {
        for (int box_idx = 0; box_idx < 9; box_idx++) {
            const Box &box = state.boxen[box_idx];
            auto box_minirows = box.cells.As_4x64();
            int box_base = tables.div3[box_idx] * 27 + tables.mod3[box_idx] * 3;
            ExtractMiniRow(box_minirows.x0, box_base, solution);
            ExtractMiniRow(box_minirows.x1, box_base + 9, solution);
            ExtractMiniRow(box_minirows.x2, box_base + 18, solution);
        }
    }

    void ReportSolution(const State &state) {
        char solution[81];
        if (callback_) {
            ExtractSolution(state, solution);
            callback_(solution);
        }
    }

    size_t SolveSudoku(const char *input, size_t limit,
                       char *solution, size_t *num_guesses) {
        limit_ = limit;
        num_solutions_ = 0;
        num_guesses_ = 0;
        bool pencilmark = input[81] >= '.';

        State state;
        if (pencilmark ? InitPencilmarkByBox(input, state) : InitVanillaByBand(input, state)) {
            CountSolutionsConsistentWithPartialAssignment(state);
            if (solution_mode == 1) ExtractSolution(solution_, solution);
        }
        if (solution_mode != 2) *num_guesses = num_guesses_;
        return num_solutions_;
    };
};


struct GeneratorDpllTriadSimd {
    SolverDpllTriadSimd<0> solver_{};
    Util util_;

    // takes a partial puzzle (vanilla or pencilmark) and adds random clues to reconstrain it
    // until there is a unique solution. this procedure is fast, but biased in the sense that
    // different puzzles may arise with widely varying probabilities and we make no effort
    // to adjust for these differences or to estimate these probabilities. it also does not
    // guarantee that the resulting puzzle is minimal.
    bool Constrain(bool pencilmark, char *puzzle) {
        State state;
        if (pencilmark) {
            SolverDpllTriadSimd<0>::InitPencilmarkByBox(puzzle, state);
        } else {
            SolverDpllTriadSimd<0>::InitVanillaByBand(puzzle, state);
        }
        vector<int> permutation = util_.Permutation(729);
        for (int literal : permutation) {
            int cell = literal / 9;

            // skip over clues that were given as part of the partial puzzle.
            if (pencilmark) {
                if (puzzle[literal] == '.') continue;
            } else {
                if (puzzle[cell] != '.') continue;
            }

            int row = cell / 9, col = cell % 9;
            int box_idx = (row / 3) * 3 + (col / 3);
            int elm_idx = (row % 3) * 4 + (col % 3);
            Box &box = state.boxen[box_idx];
            uint16_t candidates = box.cells.Extract(elm_idx);
            uint16_t candidate = 1u << (literal % 9u);

            // quick check that the candidate is not trivially included or excluded before
            // adding as a clue.
            if ((candidates & candidate) && (candidates != candidate)) {
                Cells16 restrict = box.cells;
                restrict.Insert(elm_idx, pencilmark ? candidates ^ candidate : candidate);
                State test_state = state;
                if (SolverDpllTriadSimd<0>::BoxRestrict<0>(test_state, box_idx, restrict)) {
                    int cell_or_literal = pencilmark ? literal : cell;
                    char prior_unconstrained_value = puzzle[cell_or_literal];
                    puzzle[cell_or_literal] = pencilmark ? '.' : (char)('1' + (literal % 9));
                    switch (solver_.SafeCountSolutionsConsistentWithPartialAssignment(
                            test_state, 2)) {
                        case 0:
                            puzzle[cell_or_literal] = prior_unconstrained_value;
                            continue;
                        case 1:
                            return true;
                        default:
                            state = test_state;
                            continue;
                    }
                }
            }
        }
        return false;
    }

    // minimizes a vanilla or pencilmark puzzle by testing removal of all clues in random order,
    // restoring any clue that's required to keep the solution unique. if the 'monotonic' flag
    // is passed, returns true only if we had a minimal puzzle after the first restored clue.
    bool Minimize(bool pencilmark, bool monotonic, char *puzzle) {
        bool restored_clue = false;
        vector<int> permutation = util_.Permutation(729);
        for (int cell_or_literal : permutation) {
            if (pencilmark) {
                if (puzzle[cell_or_literal] != '.') continue;
            } else {
                if (cell_or_literal >= 81 || puzzle[cell_or_literal] == '.') continue;
            }
            char constraint = puzzle[cell_or_literal];
            State state;
            if (pencilmark) {
                puzzle[cell_or_literal] = (char)('1' + (cell_or_literal % 9));
                SolverDpllTriadSimd<0>::InitPencilmarkByBox(puzzle, state);
            } else {
                puzzle[cell_or_literal] = '.';
                SolverDpllTriadSimd<0>::InitVanillaByBand(puzzle, state);
            }
            if (solver_.SafeCountSolutionsConsistentWithPartialAssignment(state, 2) > 1) {
                puzzle[cell_or_literal] = constraint;
                restored_clue = true;
            } else if (monotonic && restored_clue) {
                return false;
            }
        }
        return true;
    }
};


SolverDpllTriadSimd<0> solver_none{};
SolverDpllTriadSimd<1> solver_last{};
SolverDpllTriadSimd<2> solver_enum{};

GeneratorDpllTriadSimd generator{};

} // namespace

extern "C"
size_t TdokuSolverDpllTriadSimd(const char *puzzle, size_t limit,
                                uint32_t configuration,
                                char *solution, size_t *num_guesses) {
    bool return_last = limit == 1 || configuration > 0;
    if (return_last) {
        return solver_last.SolveSudoku(puzzle, limit, solution, num_guesses);
    } else {
        return solver_none.SolveSudoku(puzzle, limit, solution, num_guesses);
    }
}

extern "C"
size_t TdokuEnumerate(const char *puzzle, size_t limit, void (*callback)(const char *)) {
    solver_enum.callback_ = callback;
    return solver_enum.SolveSudoku(puzzle, limit, nullptr, nullptr);
}

extern "C"
bool TdokuConstrain(bool pencilmark, char *puzzle) {
    return generator.Constrain(pencilmark, puzzle);
}

extern "C"
bool TdokuMinimize(bool pencilmark, bool monotonic, char *puzzle) {
    return generator.Minimize(pencilmark, monotonic, puzzle);
}
