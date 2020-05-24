#ifndef TDOKU_SIMD_VECTORS_H
#define TDOKU_SIMD_VECTORS_H

#include <cstring>
#include <immintrin.h>
#include <memory>
#include <tuple>
#include <utility>

/*
 * We'll support vectors targeting sse2, sse4_1, avx2, and avx512bitalg instruction sets.
 * While avx2 or avx512 will be ideal, sse4_1 should deliver solid performance. OTOH, sse2
 * performance is seriously handicapped because of our heavy reliance on fast ssse3 shuffles
 * for which there is no great sse2 alternative.
 *
 * sse2 - pentium4 2000
 *   has most of the instructions we'll use, with exceptions noted below
 *
 * ssse3 2006 - core2 2006
 *   _mm_shuffle_epi8      // sse2 alt: kind of a mess. see below.
 *
 * sse4_1 - penryn 2007
 *   _mm_testz_si128       // sse2 alt: movemask(cmpeq(setzero())) in sse2
 *   _mm_blend_epi16       // sse2 alt: &| with masks
 *   _mm_minpos_epu8
 *
 * sse4_2 - nehalem 2007
 *   _mm_cmpgt_epi64
 *
 * avx2 - haswell 2013
 *   _mm256 versions of most everything
 *
 * avx512vl - skylake 2017
 *  _mm(256)_ternarylogic_epi32
 *
 * avx512vpopcntdq, avx512bitalg - ice lake 2019
 *   _mm_popcnt_epi64
 *   _mm256_popcnt_epi16
 *
 * May 2020 Steam monthly hardware survey:
 *   SSE2        100.00%
 *   SSSE3        98.52%
 *   SSE4.1       97.80%
 *   SSE4.2       97.05%
 *   AVX          92.25%
 *   AVX2         73.88%
 */

// for functions like extract below where we use switches to determine which immediate to use
// we'll assume only valid values are passed and omit the default, thereby allowing the compiler's
// assumption of defined behavior to optimize away a branch.
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreturn-type"

struct TwoBy64 {
    uint64_t x0;
    uint64_t x1;
};

struct FourBy64 {
    uint64_t x0;
    uint64_t x1;
    uint64_t x2;
    uint64_t x3;
};

namespace {

struct Consts {
    __m128i popcount_mask4 = _mm_set1_epi16(0x0f);
    __m128i popcount_lookup = _mm_setr_epi8(0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4);
    __m128i rotate_rows1 = _mm_setr_epi8(2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9);
};

const Consts consts{};

} // namespace

struct Bitvec08x16 {
    __m128i vec;

    Bitvec08x16() : vec{} {}

    // non-explicit conversions intended
    Bitvec08x16(const __m128i &m128i) noexcept : vec{m128i} {}

    Bitvec08x16(const Bitvec08x16 &other) noexcept : vec(other.vec) {}

    Bitvec08x16(uint16_t x00, uint16_t x01, uint16_t x02, uint16_t x03,
                uint16_t x04, uint16_t x05, uint16_t x06, uint16_t x07) :
            vec{_mm_setr_epi16(x00, x01, x02, x03, x04, x05, x06, x07)} {}

    static inline Bitvec08x16 All(uint16_t value) {
        return _mm_set1_epi16(value);
    }

    static inline Bitvec08x16
    or_X_Y_or_Z(const Bitvec08x16 &x, const Bitvec08x16 &y, const Bitvec08x16 &z) {
#ifdef __AVX512VL__
        return _mm_ternarylogic_epi32(x.vec, y.vec, z.vec, 0b11111110);
#else
        return x | y | z;
#endif
    }

    static inline Bitvec08x16
    and_X_Y_or_Z(const Bitvec08x16 &x, const Bitvec08x16 &y, const Bitvec08x16 &z) {
#ifdef __AVX512VL__
        return _mm_ternarylogic_epi32(x.vec, y.vec, z.vec, 0b11101010);
#else
        return (x & y) | z;
#endif
    }

    static inline Bitvec08x16
    and_X_Y_andnot_Z(const Bitvec08x16 &x, const Bitvec08x16 &y, const Bitvec08x16 &z) {
#ifdef __AVX512VL__
        return _mm_ternarylogic_epi32(x.vec, y.vec, z.vec, 0b01000000);
#else
        return (x & y).and_not(z);
#endif
    }

    inline Bitvec08x16 &operator=(const __m128i &m128i) {
        vec = m128i;
        return *this;
    }

    inline Bitvec08x16 &operator=(const Bitvec08x16 &other) = default;

    inline bool operator==(const Bitvec08x16 &other) const {
        Bitvec08x16 which_equal = WhichEqual(other);
        return _mm_movemask_epi8(which_equal.vec) == 0xffff;
    }

    inline bool operator!=(const Bitvec08x16 &other) const {
        return !(*this == other);
    }

    inline TwoBy64 As_2x64() const {
        TwoBy64 out{};
        _mm_store_si128((__m128i *) &out, vec);
        return out;
    }

    inline Bitvec08x16 WhichEqual(const Bitvec08x16 &other) const {
        return _mm_cmpeq_epi16(vec, other.vec);
    }

    inline Bitvec08x16 WhichNonZero() const {
        return _mm_cmpgt_epi16(vec, _mm_setzero_si128());
    }

    inline bool AllZero() const {
#ifdef __SSE4_1__
        return _mm_test_all_zeros(vec, vec) != 0;
#else
        return _mm_movemask_epi8(WhichEqual(_mm_setzero_si128()).vec) == 0xffff;
#endif
    }

    inline bool AnyZero() const {
        Bitvec08x16 which_equal_zero = WhichEqual(_mm_setzero_si128());
        return _mm_movemask_epi8(which_equal_zero.vec) != 0;
    }

    inline bool AnyLessThan(const Bitvec08x16 &other) const {
        Bitvec08x16 which_less_than = _mm_cmpgt_epi16(other.vec, vec);
        return _mm_movemask_epi8(which_less_than.vec) != 0;
    }

    inline bool Intersects(const Bitvec08x16 &other) const {
#ifdef __SSE4_1__
        return !_mm_testz_si128(vec, other.vec);
#else
        return !(*this & other).AllZero();
#endif
    }

    inline Bitvec08x16 ClearLowBit() const {
#ifdef __SSE4_2__
        __m128i cmp = _mm_cmpgt_epi64(vec, _mm_setzero_si128());
#else
        __m128i cmp = _mm_cmpgt_epi32(vec, _mm_setzero_si128());
        cmp = _mm_or_si128(cmp, _mm_shuffle_epi32(cmp, 0b10110001));
#endif
        __m128i one = _mm_andnot_si128(_mm_slli_si128(cmp, 1), _mm_srli_epi64(cmp, 63));
        return _mm_and_si128(vec, _mm_sub_epi64(vec, one));
    }

    // counts the number of bits set among the 9 lowest order bits of each packed 16-bit integer
    // subject to the assumption that the 7 high bits are zero. results are undefined if any of
    // the 7 high bits are nonzero.
    inline Bitvec08x16 Popcounts9() const {
#ifdef __SSSE3__
        Bitvec08x16 sum_0_3 = Bitvec08x16{consts.popcount_lookup}.Shuffle(
                *this & consts.popcount_mask4);
        Bitvec08x16 sum_4_7 = Bitvec08x16{consts.popcount_lookup}.Shuffle(
                _mm_srli_epi16(vec, 4));
        Bitvec08x16 sum_0_7 = _mm_add_epi16(sum_0_3.vec, sum_4_7.vec);
        Bitvec08x16 result = _mm_add_epi16(sum_0_7.vec, _mm_srli_epi16(vec, 8));
        return result;
#else
        // SSE2 version following https://www.hackersdelight.org/hdcodetxt/pop.c.txt
        __m128i mask1 = _mm_set1_epi8(0x77);
        __m128i mask2 = _mm_set1_epi8(0x0f);
        __m128i mask3 = _mm_set1_epi16(0xff);
        __m128i x = vec;
        __m128i n = _mm_and_si128(mask1, _mm_srli_epi64(x, 1));
        x = _mm_sub_epi8(x, n);
        n = _mm_and_si128(mask1, _mm_srli_epi64(n, 1));
        x = _mm_sub_epi8(x, n);
        n = _mm_and_si128(mask1, _mm_srli_epi64(n, 1));
        x = _mm_sub_epi8(x, n);
        x = _mm_add_epi8(x, _mm_srli_epi16(x, 4));
        x = _mm_and_si128(mask2, x);
        x = _mm_add_epi16(_mm_and_si128(x, mask3),
                          _mm_and_si128(_mm_bsrli_si128(x, 1), mask3));
        return x;
#endif
    }

    inline int Popcount() const {
#ifdef __AVX512VPOPCNTDQ__
        __m128i counts = _mm_popcnt_epi64(vec);
        return _mm_cvtsi128_si64(counts) + _mm_cvtsi128_si64(_mm_unpackhi_epi64(counts, counts));
#else
        // unpackhi_epi64+cvtsi128_si64 compiles to the same instructions as extract_epi64,
        // but works on windows where extract_epi64 is missing.
        return NumBitsSet64((uint64_t) _mm_cvtsi128_si64(vec)) +
               NumBitsSet64((uint64_t) _mm_cvtsi128_si64(_mm_unpackhi_epi64(vec, vec)));
#endif
    }

    inline uint32_t MinPosGreaterThanOrEqual(uint16_t min_val) {
#ifdef __SSE4_1__
        return _mm_cvtsi128_si32(_mm_minpos_epu16(_mm_sub_epi16(vec, _mm_set1_epi16(min_val))));
#else
        uint32_t min = 0xffff;
        uint32_t pos = 0;
        uint64_t lo = _mm_cvtsi128_si64(vec);
        for (int i = 0; i < 4; i++) {
            uint32_t val = ((int32_t)(lo & 0xffffu) - min_val);
            if (val < min) {
                min = val;
                pos = i;
            }
            lo >>= 16u;
        }
        uint64_t hi = _mm_cvtsi128_si64(_mm_unpackhi_epi64(vec, vec));
        for (int i = 4; i < 8; i++) {
            uint32_t val = ((int32_t)(hi & 0xffffu) - min_val);
            if (val < min) {
                min = val;
                pos = i;
            }
            hi >>= 16u;
        }
        return (pos << 16u) | min;
#endif
    }

    inline Bitvec08x16 Shuffle(const Bitvec08x16 &control) const {
#ifdef __SSSE3__
        return _mm_shuffle_epi8(vec, control.vec);
#else
        // we'll rely on the assumption that all requested shuffles are for epi16s so each
        // pair of requested bytes are always adjacent like 0x0302.
        __m128i ctrl = control.vec & _mm_set1_epi16(0xf);

        // replicate low 16 bits of each epi32 to the high 16
        Bitvec08x16 lo16s = vec & _mm_set1_epi32(0x0000ffff);
        lo16s |= _mm_bslli_si128(lo16s.vec, 2);
        // and vice versa
        Bitvec08x16 hi16s = vec & _mm_set1_epi32(0xffff0000);
        hi16s |= _mm_bsrli_si128(hi16s.vec, 2);

        Bitvec08x16 z{};
        z |= _mm_shuffle_epi32(lo16s.vec, 0b00000000) & _mm_cmpeq_epi16(ctrl, _mm_set1_epi16(0x0));
        z |= _mm_shuffle_epi32(hi16s.vec, 0b00000000) & _mm_cmpeq_epi16(ctrl, _mm_set1_epi16(0x2));
        z |= _mm_shuffle_epi32(lo16s.vec, 0b01010101) & _mm_cmpeq_epi16(ctrl, _mm_set1_epi16(0x4));
        z |= _mm_shuffle_epi32(hi16s.vec, 0b01010101) & _mm_cmpeq_epi16(ctrl, _mm_set1_epi16(0x6));
        z |= _mm_shuffle_epi32(lo16s.vec, 0b10101010) & _mm_cmpeq_epi16(ctrl, _mm_set1_epi16(0x8));
        z |= _mm_shuffle_epi32(hi16s.vec, 0b10101010) & _mm_cmpeq_epi16(ctrl, _mm_set1_epi16(0xa));
        z |= _mm_shuffle_epi32(lo16s.vec, 0b11111111) & _mm_cmpeq_epi16(ctrl, _mm_set1_epi16(0xc));
        z |= _mm_shuffle_epi32(hi16s.vec, 0b11111111) & _mm_cmpeq_epi16(ctrl, _mm_set1_epi16(0xe));
        return z;
#endif
    }

    inline Bitvec08x16 RotateRows() const {
#ifdef __SSSE3__
        return Shuffle(consts.rotate_rows1);
#else
        __m128i mask1 = _mm_setr_epi16(0xffff, 0xffff, 0xffff, 0x0, 0xffff, 0xffff, 0xffff, 0x0);
        __m128i mask2 = _mm_setr_epi16(0x0, 0x0, 0x0, 0xffff, 0x0, 0x0, 0x0, 0xffff);
        return _mm_or_si128(
                _mm_and_si128(_mm_bsrli_si128(vec, 2), mask1),
                _mm_and_si128(_mm_bslli_si128(vec, 6), mask2));
#endif
    }

    inline Bitvec08x16 RotateRows2() const {
#ifdef __SSSE3__
        return _mm_shuffle_epi32(vec, 0b10110001);
#else
        __m128i mask1 = _mm_setr_epi16(0xffff, 0xffff, 0x0, 0x0, 0xffff, 0xffff, 0x0, 0x0);
        __m128i mask2 = _mm_setr_epi16(0x0, 0x0, 0xffff, 0xffff, 0x0, 0x0, 0xffff, 0xffff);
        return _mm_or_si128(
                _mm_and_si128(_mm_bsrli_si128(vec, 4), mask1),
                _mm_and_si128(_mm_bslli_si128(vec, 4), mask2));
#endif
    }

    inline Bitvec08x16 RotateCols() const {
        return _mm_shuffle_epi32(vec, 0b01001110);
    }

    inline uint16_t Extract(int index) const {
#define CASE(x) case x: return _mm_extract_epi16(vec, x);
        switch (index) {
            CASE(0)
            CASE(1)
            CASE(2)
            CASE(3)
            CASE(4)
            CASE(5)
            CASE(6)
            CASE(7)
        }
#undef CASE
    }

    void Insert(int index, uint16_t value) {
#define CASE(x) case x: vec = _mm_insert_epi16(vec, value, x); break;
        switch (index) {
            CASE(0)
            CASE(1)
            CASE(2)
            CASE(3)
            CASE(4)
            CASE(5)
            CASE(6)
            CASE(7)
        }
#undef CASE
    }

    inline Bitvec08x16 operator|(const Bitvec08x16 &other) const {
        return _mm_or_si128(vec, other.vec);
    }

    inline void operator|=(const Bitvec08x16 &other) {
        vec = (*this | other).vec;
    }

    inline Bitvec08x16 operator^(const Bitvec08x16 &other) const {
        return _mm_xor_si128(vec, other.vec);
    }

    inline void operator^=(const Bitvec08x16 &other) {
        vec = (*this ^ other).vec;
    }

    inline Bitvec08x16 operator&(const Bitvec08x16 &other) const {
        return _mm_and_si128(vec, other.vec);
    }

    inline void operator&=(const Bitvec08x16 &other) {
        vec = (*this & other).vec;
    }

    inline Bitvec08x16 and_not(const Bitvec08x16 &other) const {
        return _mm_andnot_si128(other.vec, vec);
    };
};


#ifndef __AVX2__

struct Bitvec16x16 {
    Bitvec08x16 lo_;
    Bitvec08x16 hi_;

    Bitvec16x16() noexcept : lo_{}, hi_{} {}

    // non-explicit conversions intended
    Bitvec16x16(const Bitvec16x16 &other) noexcept = default;

    Bitvec16x16(const Bitvec08x16 &lo, const Bitvec08x16 &hi) noexcept : lo_(lo.vec), hi_(hi.vec) {}

    Bitvec16x16(uint16_t x00, uint16_t x01, uint16_t x02, uint16_t x03,
                uint16_t x04, uint16_t x05, uint16_t x06, uint16_t x07,
                uint16_t x08, uint16_t x09, uint16_t x10, uint16_t x11,
                uint16_t x12, uint16_t x13, uint16_t x14, uint16_t x15) noexcept :
            lo_{_mm_setr_epi16(x00, x01, x02, x03, x04, x05, x06, x07)},
            hi_{_mm_setr_epi16(x08, x09, x10, x11, x12, x13, x14, x15)} {}

    static inline Bitvec16x16 All(uint16_t value) {
        return Bitvec16x16{Bitvec08x16::All(value), Bitvec08x16::All(value)};
    }

    static inline Bitvec16x16
    or_X_Y_or_Z(const Bitvec16x16 &x, const Bitvec16x16 &y, const Bitvec16x16 &z) {
        return Bitvec16x16{x.lo_ | y.lo_ | z.lo_, x.hi_ | y.hi_ | z.hi_};
    }

    static inline Bitvec16x16
    and_X_Y_or_Z(const Bitvec16x16 &x, const Bitvec16x16 &y, const Bitvec16x16 &z) {
        return Bitvec16x16{(x.lo_ & y.lo_) | z.lo_, (x.hi_ & y.hi_) | z.hi_};
    }

    static inline Bitvec16x16
    and_X_Y_andnot_Z(const Bitvec16x16 &x, const Bitvec16x16 &y, const Bitvec16x16 &z) {
        return Bitvec16x16{x.lo_ & y.lo_.and_not(z.lo_), x.hi_ & y.hi_.and_not(z.hi_)};
    }

    static inline Bitvec16x16
    xor_X_Y_or_Z(const Bitvec16x16 &x, const Bitvec16x16 &y, const Bitvec16x16 &z) {
        return Bitvec16x16{(x.lo_ ^ y.lo_) | z.lo_, (x.hi_ ^ y.hi_) | z.hi_};
    }

    inline Bitvec16x16 &operator=(const Bitvec16x16 &other) = default;

    inline bool operator==(const Bitvec16x16 &other) const {
        return lo_ == other.lo_ && hi_ == other.hi_;
    }

    inline bool operator!=(const Bitvec16x16 &other) const {
        return lo_ != other.lo_ || hi_ != other.hi_;
    }

    inline const Bitvec08x16 GetLo() const {
        return lo_;
    }

    inline const Bitvec08x16 GetHi() const {
        return hi_;
    }

    inline FourBy64 As_4x64() const {
        FourBy64 out{};
        TwoBy64 lo_2x64 = lo_.As_2x64();
        TwoBy64 hi_2x64 = hi_.As_2x64();
        memcpy(&out.x0, &lo_2x64, sizeof(lo_2x64));
        memcpy(&out.x2, &hi_2x64, sizeof(hi_2x64));
        return out;
    }

    inline Bitvec16x16 WhichEqual(const Bitvec16x16 &other) const {
        return Bitvec16x16{lo_.WhichEqual(other.lo_), hi_.WhichEqual(other.hi_)};
    }

    inline Bitvec16x16 WhichNonZero() const {
        return Bitvec16x16{lo_.WhichNonZero(), hi_.WhichNonZero()};
    }

    inline bool AllZero() const {
        return lo_.AllZero() && hi_.AllZero();
    }

    inline bool AnyZero() const {
        return lo_.AnyZero() || hi_.AnyZero();
    }

    inline bool AnyLessThan(const Bitvec16x16 &other) const {
        return lo_.AnyLessThan(other.lo_) || hi_.AnyLessThan(other.hi_);
    }

    inline bool Intersects(const Bitvec16x16 &other) const {
        return lo_.Intersects(other.lo_) || hi_.Intersects(other.hi_);
    }

    // counts the number of bits set among the 9 lowest order bits of each packed 16-bit integer
    // subject to the assumption that the 7 high bits are zero. results are undefined if any of
    // the 7 high bits are nonzero.
    inline Bitvec16x16 Popcounts9() const {
        return Bitvec16x16{lo_.Popcounts9(), hi_.Popcounts9()};
    }

    inline Bitvec16x16 Shuffle(const Bitvec16x16 &control) const {
        return Bitvec16x16{lo_.Shuffle(control.lo_), hi_.Shuffle(control.hi_)};
    }

    inline Bitvec16x16 RotateRows() const {
        return Bitvec16x16{lo_.RotateRows(), hi_.RotateRows()};
    }

    inline Bitvec16x16 RotateRows2() const {
        return Bitvec16x16{lo_.RotateRows2(), hi_.RotateRows2()};
    }

    inline Bitvec16x16 RotateCols() const {
#ifdef __SSSE3__
        return Bitvec16x16{_mm_alignr_epi8(hi_.vec, lo_.vec, 8),
                           _mm_alignr_epi8(lo_.vec, hi_.vec, 8)};
#else
        return Bitvec16x16{_mm_or_si128(_mm_srli_si128(lo_.vec, 8), _mm_slli_si128(hi_.vec, 8)),
                           _mm_or_si128(_mm_srli_si128(hi_.vec, 8), _mm_slli_si128(lo_.vec, 8))};
#endif
    }

    inline Bitvec16x16 RotateCols2() const {
        return Bitvec16x16{hi_, lo_};
    }

    inline uint16_t Extract(int index) const {
        if (index < 8) {
            return lo_.Extract(index);
        } else {
            return hi_.Extract(index - 8);
        }
    }

    inline void Insert(int index, uint16_t value) {
        if (index < 8) {
            lo_.Insert(index, value);
        } else {
            hi_.Insert(index - 8, value);
        }
    }

    inline Bitvec16x16 operator|(const Bitvec16x16 &other) const {
        return Bitvec16x16{lo_ | other.lo_, hi_ | other.hi_};
    }

    inline void operator|=(const Bitvec16x16 &other) {
        lo_ |= other.lo_;
        hi_ |= other.hi_;
    }

    inline Bitvec16x16 operator^(const Bitvec16x16 &other) const {
        return Bitvec16x16{lo_ ^ other.lo_, hi_ ^ other.hi_};
    }

    inline void operator^=(const Bitvec16x16 &other) {
        lo_ ^= other.lo_;
        hi_ ^= other.hi_;
    }

    inline Bitvec16x16 operator&(const Bitvec16x16 &other) const {
        return Bitvec16x16{lo_ & other.lo_, hi_ & other.hi_};
    }

    inline void operator&=(const Bitvec16x16 &other) {
        lo_ &= other.lo_;
        hi_ &= other.hi_;
    }

    inline Bitvec16x16 and_not(const Bitvec16x16 &other) const {
        return Bitvec16x16{lo_.and_not(other.lo_), hi_.and_not(other.hi_)};
    };
};

#endif //!__AVX2__


#ifdef __AVX2__

#ifdef __GNUC__
#if __GNUC__ < 8
#define _mm256_set_m128i(x, y) _mm256_permute2f128_si256(_mm256_castsi128_si256(x), \
                                                         _mm256_castsi128_si256(y), 2)
#endif
#endif

struct Bitvec16x16 {
    __m256i vec;

    Bitvec16x16() noexcept : vec{} {}

    // non-explicit conversions intended
    Bitvec16x16(const __m256i &m256i) noexcept : vec{m256i} {}

    Bitvec16x16(const Bitvec16x16 &other) noexcept = default;

    Bitvec16x16(const Bitvec08x16 &lo, const Bitvec08x16 &hi) noexcept :
            vec{_mm256_set_m128i(hi.vec, lo.vec)} {}

    Bitvec16x16(uint16_t x00, uint16_t x01, uint16_t x02, uint16_t x03,
                uint16_t x04, uint16_t x05, uint16_t x06, uint16_t x07,
                uint16_t x08, uint16_t x09, uint16_t x10, uint16_t x11,
                uint16_t x12, uint16_t x13, uint16_t x14, uint16_t x15) noexcept :
            vec{_mm256_setr_epi16(x00, x01, x02, x03, x04, x05, x06, x07,
                                  x08, x09, x10, x11, x12, x13, x14, x15)} {}

    static Bitvec16x16 All(uint16_t value) {
        return _mm256_set1_epi16(value);
    }

    static inline Bitvec16x16
    or_X_Y_or_Z(const Bitvec16x16 &x, const Bitvec16x16 &y, const Bitvec16x16 &z) {
#ifdef __AVX512VL__
        return _mm256_ternarylogic_epi32(x.vec, y.vec, z.vec, 0b11111110);
#else
        return x | y | z;
#endif
    }

    static inline Bitvec16x16
    and_X_Y_or_Z(const Bitvec16x16 &x, const Bitvec16x16 &y, const Bitvec16x16 &z) {
#ifdef __AVX512VL__
        return _mm256_ternarylogic_epi32(x.vec, y.vec, z.vec, 0b11101010);
#else
        return (x & y) | z;
#endif
    }

    static inline Bitvec16x16
    and_X_Y_andnot_Z(const Bitvec16x16 &x, const Bitvec16x16 &y, const Bitvec16x16 &z) {
#ifdef __AVX512VL__
        return _mm256_ternarylogic_epi32(x.vec, y.vec, z.vec, 0b01000000);
#else
        return x & y.and_not(z);
#endif
    }

static inline Bitvec16x16
    xor_X_Y_or_Z(const Bitvec16x16 &x, const Bitvec16x16 &y, const Bitvec16x16 &z) {
#ifdef __AVX512VL__
        return _mm256_ternarylogic_epi32(x.vec, y.vec, z.vec, 0b10111110);
#else
        return (x ^ y) | z;
#endif
    }

    inline Bitvec16x16 &operator=(const Bitvec16x16 &other) = default;

    inline bool operator==(const Bitvec16x16 &other) const {
        return (*this ^ other).AllZero();
    }

    inline bool operator!=(const Bitvec16x16 &other) const {
        return !(*this == other);
    }

    inline Bitvec08x16 GetLo() const {
        return _mm256_extracti128_si256(vec, 0);
    }

    inline Bitvec08x16 GetHi() const {
        return _mm256_extracti128_si256(vec, 1);
    }

    inline FourBy64 As_4x64() const {
        FourBy64 out{};
        out.x0 = _mm256_extract_epi64(vec, 0);
        out.x1 = _mm256_extract_epi64(vec, 1);
        out.x2 = _mm256_extract_epi64(vec, 2);
        out.x3 = _mm256_extract_epi64(vec, 3);
        return out;
    }

    inline Bitvec16x16 WhichEqual(const Bitvec16x16 &other) const {
        return _mm256_cmpeq_epi16(vec, other.vec);
    }

    inline Bitvec16x16 WhichNonZero() const {
        return _mm256_cmpgt_epi16(vec, _mm256_setzero_si256());
    }

    inline bool AllZero() const {
        return _mm256_testz_si256(vec, vec);
    }

    inline bool AnyZero() const {
        Bitvec16x16 which_equal_zero = WhichEqual(_mm256_setzero_si256());
        return _mm256_movemask_epi8(which_equal_zero.vec) != 0;
    }

    inline bool AnyLessThan(const Bitvec16x16 &other) const {
        Bitvec16x16 which_less_than = _mm256_cmpgt_epi16(other.vec, vec);
        return _mm256_movemask_epi8(which_less_than.vec) != 0;
    }

    inline bool Intersects(const Bitvec16x16 &other) const {
        return !_mm256_testz_si256(vec, other.vec);
    }

    // counts the number of bits set among the 9 lowest order bits of each packed 16-bit integer
    // subject to the assumption that the 7 high bits are zero. results are undefined if any of
    // the 7 high bits are nonzero.
    inline Bitvec16x16 Popcounts9() const {
#ifdef __AVX512BITALG__
        return _mm256_popcnt_epi16(vec);
#else
        __m256i lookup = _mm256_setr_epi8(0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4,
                                          0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4);
        __m256i mask4 = _mm256_set1_epi16(0x0f);
        __m256i sum_0_3 = _mm256_shuffle_epi8(lookup, _mm256_and_si256(vec, mask4));
        __m256i sum_4_7 = _mm256_shuffle_epi8(lookup, _mm256_srli_epi16(vec, 4));
        __m256i sum_0_7 = _mm256_add_epi16(sum_0_3, sum_4_7);
        return _mm256_add_epi16(sum_0_7, _mm256_srli_epi16(vec, 8));
#endif
    }

    inline Bitvec16x16 Shuffle(const Bitvec16x16 &control) const {
        return _mm256_shuffle_epi8(vec, control.vec);
    }

    inline Bitvec16x16 RotateRows() const {
        __m256i shuffle_control =
                _mm256_setr_epi8(2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9,
                                 2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9);
        return _mm256_shuffle_epi8(vec, shuffle_control);
    }

    inline Bitvec16x16 RotateRows2() const {
        return _mm256_shuffle_epi32(vec, 0b10110001);
    }

    inline Bitvec16x16 RotateCols() const {
        return _mm256_permute4x64_epi64(vec, 0b00111001);
    }

    inline Bitvec16x16 RotateCols2() const {
        return _mm256_permute4x64_epi64(vec, 0b01001110);
    }

    inline uint16_t Extract(int index) const {
#define CASE(x) case x: return _mm256_extract_epi16(vec, x);
        switch (index) {
            CASE(0)
            CASE(1)
            CASE(2)
            CASE(3)
            CASE(4)
            CASE(5)
            CASE(6)
            CASE(7)
            CASE(8)
            CASE(9)
            CASE(10)
            CASE(11)
            CASE(12)
            CASE(13)
            CASE(14)
            CASE(15)
        }
#undef CASE
    }

    inline void Insert(int index, uint16_t value) {
#define CASE(x) case x: vec = _mm256_insert_epi16(vec, value, x); break;
        switch (index) {
            CASE(0)
            CASE(1)
            CASE(2)
            CASE(3)
            CASE(4)
            CASE(5)
            CASE(6)
            CASE(7)
            CASE(8)
            CASE(9)
            CASE(10)
            CASE(11)
            CASE(12)
            CASE(13)
            CASE(14)
            CASE(15)
        }
#undef CASE
    }

    inline Bitvec16x16 operator|(const Bitvec16x16 &other) const {
        return _mm256_or_si256(vec, other.vec);
    }

    inline void operator|=(const Bitvec16x16 &other) {
        vec = (*this | other).vec;
    }

    inline Bitvec16x16 operator^(const Bitvec16x16 &other) const {
        return _mm256_xor_si256(vec, other.vec);
    }

    inline void operator^=(const Bitvec16x16 &other) {
        vec = (*this ^ other).vec;
    }

    inline Bitvec16x16 operator&(const Bitvec16x16 &other) const {
        return _mm256_and_si256(vec, other.vec);
    }

    inline void operator&=(const Bitvec16x16 &other) {
        vec = (*this & other).vec;
    }

    inline Bitvec16x16 and_not(const Bitvec16x16 &other) const {
        return _mm256_andnot_si256(other.vec, vec);
    };
};

#endif // __AVX2__

inline uint32_t WhichDots16(const char *x) {
    const __m128i dots = _mm_set1_epi8('.');
    return ((uint32_t) _mm_movemask_epi8(
            _mm_cmpeq_epi8(_mm_loadu_si128((const __m128i *) x), dots)));
}

inline uint32_t WhichDots32(const char *x) {
#ifdef __AVX2__
    const __m256i dots = _mm256_set1_epi8('.');
    return ((uint32_t) _mm256_movemask_epi8(
            _mm256_cmpeq_epi8(_mm256_loadu_si256((const __m256i *) x), dots)));
#else
    return WhichDots16(x) | (WhichDots16(x + 16) << 16u);
#endif // __AVX2__
}

inline uint64_t WhichDots64(const char *x) {
    return (uint64_t) WhichDots32(x) | ((uint64_t) WhichDots32(x + 32) << 32u);
}

#pragma GCC diagnostic pop

#endif //TDOKU_SIMD_VECTORS_H
