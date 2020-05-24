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
void GetGrid(size_t grid_id, const void *index, const void *table, char *grid) {
    size_t indexed_grid_id = grid_id & ~((1ull << 20u) - 1);
    size_t lookup = grid_id >> 20u;
    size_t offset = grid_id - indexed_grid_id;

    uint32_t pattern_id = *(uint32_t *)(((char *)index) + lookup * 6);
    uint16_t indexed_offset = *(uint16_t *)(((char *)index) + lookup * 6 + 4);

    offset += indexed_offset;
    do {
        uint16_t pattern_count = *(((uint16_t *) table) + pattern_id);
        if (offset < pattern_count) {
            char pattern[82];
            GetPattern(pattern_id, pattern);
            size_t guesses;
            SolveSudoku(pattern, offset + 1, 1, grid, &guesses);
            break;
        } else {
            offset -= pattern_count;
            pattern_id++;
        }
    } while (true);
}

