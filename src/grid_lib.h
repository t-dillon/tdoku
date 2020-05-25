#ifndef TDOKU_GRID_LIB_H
#define TDOKU_GRID_LIB_H

#include <cstddef>

#ifdef __cplusplus
extern "C"
#endif
void GetPattern(int pattern_id, char *pattern);

#ifdef __cplusplus
extern "C"
#endif
void GetGrid(size_t grid_id, const void *index, const void *table, char *grid);

#ifdef __cplusplus
extern "C"
#endif
void EnumerateGrids(size_t first_grid_idx, size_t count, const void *index, const void *table,
                    void (*callback)(const char *));

#endif  // TDOKU_GRID_LIB_H

