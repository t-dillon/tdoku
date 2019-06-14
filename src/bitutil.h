#ifndef TDOKU_BIT_TWIDDLING_H
#define TDOKU_BIT_TWIDDLING_H

#include <stdint.h>

inline int NumBitsSet(uint32_t x) {
    return __builtin_popcount(x);
}

inline int NumBitsSet64(uint64_t x) {
    return __builtin_popcountll(x);
}

inline uint32_t GetLowBit(uint32_t x) {
    return x & -x;
}

inline uint64_t GetLowBit64(uint64_t x) {
    return  x & (uint64_t)-x;
}

inline uint32_t ClearLowBit(uint32_t nonzero_x) {
    return nonzero_x & (nonzero_x - 1);
}

inline uint64_t ClearLowBit64(uint64_t nonzero_x) {
    return nonzero_x & (nonzero_x - 1);
}

inline int LowOrderBitIndex(uint32_t nonzero_x) {
    return __builtin_ffs(nonzero_x) - 1;
}

inline int LowOrderBitIndex64(uint64_t nonzero_x) {
    return __builtin_ffsll(nonzero_x) - 1;
}

inline int HighOrderBitIndex(uint32_t nonzero_x) {
    return sizeof(uint32_t) * 8 - __builtin_clz(nonzero_x) - 1;
}

inline int HighOrderBitIndex64(uint64_t nonzero_x) {
    return sizeof(uint64_t) * 8 - __builtin_clzll(nonzero_x) - 1;
}

#endif //TDOKU_BIT_TWIDDLING_H
