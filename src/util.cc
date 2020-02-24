#include "util.h"
#include <algorithm>
#include <array>
#include <vector>
#include <cstring>

using namespace std;

void Util::RandomSeed(uint64_t seed) {
    rng_.seed(seed);
}

uint32_t Util::RandomUInt() {
    return random_uint_(rng_);
}

double Util::RandomDouble() {
    return random_double_(rng_);
}

// generate a permutation of integers in range(0,size)
vector<int> Util::Permutation(size_t size) {
    vector<int> permutation;
    permutation.reserve(size);
    for (size_t i = 0; i < size; i++) permutation.push_back(i);
    shuffle(permutation.begin(), permutation.end(), rng_);
    return permutation;
}

// permute indices s.t. bands may be reordered and rows or columns may be reordered
// within a band, but rows or columns may not be exchanged between bands.
void Util::BlockShuffle(array<int, 9> *vec) {
    array<int, 3> blocks{0, 1, 2};
    shuffle(blocks.begin(), blocks.end(), rng_);
    for (int i = 0; i < 3; i++) {
        array<int, 3> block{0, 1, 2};
        shuffle(block.begin(), block.end(), rng_);
        for (int j = 0; j < 3; j++) {
            (*vec)[i * 3 + j] = blocks[i] * 3 + block[j];
        }
    }
}

// permute rows, columns, bands, and digits to produce a randomly transformed but
// equivalent puzzle.
void Util::PermuteSudoku(char *puzzle, bool pencilmark) {
    array<int, 9> digit_permutation{0, 1, 2, 3, 4, 5, 6, 7, 8};
    shuffle(digit_permutation.begin(), digit_permutation.end(), rng_);

    array<int, 9> row_permutation{0, 1, 2, 3, 4, 5, 6, 7, 8};
    array<int, 9> col_permutation{0, 1, 2, 3, 4, 5, 6, 7, 8};
    BlockShuffle(&col_permutation);
    BlockShuffle(&row_permutation);

    size_t row_size = pencilmark ? 81 : 9;
    size_t puzzle_size = row_size * 9;

    vector<char> out_puzzle;
    out_puzzle.resize(puzzle_size);

    for (int row = 0; row < 9; row++) {
        for (int col = 0; col < 9; col++) {
            if (pencilmark) {
                for (int digit = 0; digit < 9; digit++) {
                    bool eliminated = puzzle[row * 81 + col * 9 + digit] == '.';
                    out_puzzle[row_permutation[row] * 81 +
                               col_permutation[col] * 9 +
                               digit_permutation[digit]] =
                            eliminated ? '.' : (char) ('1' + digit_permutation[digit]);
                }
            } else {
                char digit = puzzle[row * 9 + col];
                if (digit != '.') {
                    digit = (char) ('1' + digit_permutation[digit - '1']);
                }
                out_puzzle[row_permutation[row] * 9 + col_permutation[col]] = digit;
            }
        }
    }
    strncpy(puzzle, &out_puzzle[0], puzzle_size);
}
