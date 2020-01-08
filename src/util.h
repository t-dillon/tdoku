#include <array>
#include <random>
#include <vector>

class Util {
private:
    std::random_device rd{};
    std::mt19937_64 rng_{rd()};
    std::uniform_int_distribution<uint32_t> random_uint_{};
    std::uniform_real_distribution<> random_double_{0.0, 1.0};

public:
    void RandomSeed(uint64_t seed);
    uint32_t RandomUInt();
    double RandomDouble();

    // generate a permutation of integers in range(0,size)
    std::vector<int> Permutation(size_t size);

    // permute indices s.t. bands may be reordered and rows or columns may be reordered
    // within a band, but rows or columns may not be exchanged between bands.
    void BlockShuffle(std::array<int, 9> *vec);

    // permute rows, columns, bands, and digits to produce a randomly transformed but
    // equivalent puzzle.
    void PermuteSudoku(char *puzzle, bool pencilmark);
};
