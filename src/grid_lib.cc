#include "grid_lib.h"
#include "tdoku.h"

#include <cmath>
#include <cstdint>
#include <cstring>

namespace {

const int permutations[6][3] {{0,1,2},{0,2,1},{1,0,2},{1,2,0},{2,0,1},{2,1,0}};

// TODO: clean up this mess
void BandInit(int configuration, int (*Indexing)(int, int), char *pattern) {
    int p[3][2] { { 0, 0 },
                  { configuration % 6, (configuration / 6) % 6 },
                  { (configuration / 36) % 6, (configuration / 216) % 6}};
    int picks = configuration / 1296;
    int pick[] { picks % 3, (picks / 3) % 3, picks / 9 };
    for (int i = 0; i < 3; i++) {
        if (picks == 27) {
            for (int j = 0; j < 3; j++) {
                pattern[Indexing(i, permutations[p[i][0]][j] + 3)] = pattern[Indexing((i + 1) % 3, j)];
                pattern[Indexing(i, permutations[p[i][1]][j] + 6)] = pattern[Indexing((i + 2) % 3, j)];
            }
        } else {
            pattern[Indexing(i, permutations[p[i][0]][((pick[i] + 0) % 3)] + 3)] = pattern[Indexing((i + 2) % 3, (pick[(i + 2) % 3] + 0) % 3)];
            pattern[Indexing(i, permutations[p[i][0]][((pick[i] + 1) % 3)] + 3)] = pattern[Indexing((i + 1) % 3, (pick[(i + 1) % 3] + 1) % 3)];
            pattern[Indexing(i, permutations[p[i][0]][((pick[i] + 2) % 3)] + 3)] = pattern[Indexing((i + 1) % 3, (pick[(i + 1) % 3] + 2) % 3)];

            pattern[Indexing(i, permutations[p[i][1]][((pick[i] + 0) % 3)] + 6)] = pattern[Indexing((i + 1) % 3, (pick[(i + 1) % 3] + 0) % 3)];
            pattern[Indexing(i, permutations[p[i][1]][((pick[i] + 1) % 3)] + 6)] = pattern[Indexing((i + 2) % 3, (pick[(i + 2) % 3] + 1) % 3)];
            pattern[Indexing(i, permutations[p[i][1]][((pick[i] + 2) % 3)] + 6)] = pattern[Indexing((i + 2) % 3, (pick[(i + 2) % 3] + 2) % 3)];
        }
    }
}

const char box1_pattern_template[] =
        "123......456......789............................................................";
constexpr int n_band_configs = 28 * 6 * 6 * 6 * 6;

int HorizIndexing(int x, int y) { return x * 9 + y; }
int VertiIndexing(int x, int y) { return y * 9 + x; }

}  // namespace

extern "C"
void GetPattern(int pattern_id, char *pattern) {
    strncpy(pattern, box1_pattern_template, 81);
    BandInit(pattern_id % n_band_configs, HorizIndexing, pattern);
    BandInit(pattern_id / n_band_configs, VertiIndexing, pattern);
    pattern[81] = '\0';
}

extern "C"
void GetGrid(size_t grid_idx, const void *index, const void *table, char *grid) {
    size_t indexed_grid_idx = grid_idx & ~((1ull << 20u) - 1);
    uint32_t current_pattern_idx = *(uint32_t *)(((char *)index) + (grid_idx >> 20u)* 6);
    uint16_t indexed_grid_offset = *(uint16_t *)(((char *)index) + (grid_idx  >> 20u)* 6 + 4);

    size_t to_skip = indexed_grid_offset + (grid_idx  - indexed_grid_idx);
    uint16_t pattern_count = *(((uint16_t *) table) + current_pattern_idx);
    while (to_skip >= pattern_count) {
        to_skip -= pattern_count;
        current_pattern_idx++;
        pattern_count = *(((uint16_t *) table) + current_pattern_idx);
    }
    char pattern[82];
    GetPattern(current_pattern_idx, pattern);
    size_t guesses;
    SolveSudoku(pattern, to_skip + 1, 1, grid, &guesses);
}

extern "C"
void EnumerateGrids(size_t first_grid_idx, size_t count,
                    const void *index, const void *table,
                    void (*callback)(const char *)) {
    size_t indexed_grid_idx = first_grid_idx & ~((1ull << 20u) - 1);
    uint32_t current_pattern_idx = *(uint32_t *)(((char *)index) + (first_grid_idx >> 20u)* 6);
    uint16_t indexed_grid_offset = *(uint16_t *)(((char *)index) + (first_grid_idx  >> 20u)* 6 + 4);

    size_t to_skip = indexed_grid_offset + (first_grid_idx  - indexed_grid_idx);
    uint16_t pattern_count = *(((uint16_t *) table) + current_pattern_idx);
    while (to_skip >= pattern_count) {
        to_skip -= pattern_count;
        current_pattern_idx++;
        pattern_count = *(((uint16_t *) table) + current_pattern_idx);
    }
    size_t remaining = count;
    while (remaining > 0) {
        size_t limit = to_skip + remaining;
        if (limit > pattern_count) limit = pattern_count;

        char pattern[82];
        GetPattern(current_pattern_idx, pattern);

        auto skipping_callback=[&](const char *grid){
            if (to_skip > 0) {
                to_skip--;
            } else {
                callback(grid);
                remaining--;
            }
        };
        // pass a thunk since we can't pass a capturing lambda as a function pointer
        TdokuEnumerate(pattern, limit, [](const char *grid, void *thunked_callback) {
            (*static_cast<decltype(skipping_callback)*>(thunked_callback))(grid);
        }, &skipping_callback);

        current_pattern_idx++;
        pattern_count = *(((uint16_t *) table) + current_pattern_idx);
    }
}

















