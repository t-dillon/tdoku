#include "grid_lib.h"
#include "tdoku.h"

#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <random>
#include <sys/mman.h>
#include <sys/stat.h>

using namespace std;

void ListPatterns(uint64_t pattern_id, uint64_t limit) {
    char pattern[82];
    for (int i = 0; i < limit; i++) {
        GetPattern(pattern_id + i, pattern);
        printf("%.81s\n", pattern);
    }
}

void CountGrids(int start, int limit) {
    char pattern[82];
    char solution[82];
    size_t guesses;

    for (int pattern_idx = start; pattern_idx < start + limit; pattern_idx++) {
        GetPattern(pattern_idx, pattern);
        int count = SolveSudoku(pattern, 100000, 0, solution, &guesses);
        printf("%d\t%d\n", pattern_idx, count);
    }
}

constexpr int index_step = 1024 * 1024;

void MakeTables() {
    char *line = nullptr;
    size_t size;

    size_t pattern_index = 0;
    size_t pattern_base_puzzle_index = 0;
    size_t next_puzzle_index = 0;

    ofstream table_fast("grid.counts", ios::out | ios::binary);
    ofstream table_index("grid.index", ios::out | ios::binary);

    while (getline(&line, &size, stdin) != -1) {
        string s(line);
        uint16_t pattern_count = stoi(s.substr(s.find_last_of('\t')));

        if (next_puzzle_index < pattern_base_puzzle_index + pattern_count) {
            uint16_t puzzle_offset = next_puzzle_index - pattern_base_puzzle_index;
            table_index.write(reinterpret_cast<const char *>(&pattern_index), sizeof(uint32_t));
            table_index.write(reinterpret_cast<const char *>(&puzzle_offset), sizeof(uint16_t));
            next_puzzle_index += index_step;
        }

        pattern_index++;
        pattern_base_puzzle_index += pattern_count;

        table_fast.write(reinterpret_cast<const char *>(&pattern_count), sizeof(uint16_t));
    }
    table_fast.close();
    table_index.close();
}

void *MMapFile(const char *file_path) {
    int fd = open(file_path, O_RDONLY);
    if (fd < 0) {
        cout << "Could not open file: " << file_path << endl;
        exit(1);
    }
    struct stat statbuf;
    fstat(fd, &statbuf);
    void *mapped = mmap(nullptr, statbuf.st_size, PROT_READ, MAP_SHARED, fd, 0);
    if (mapped == MAP_FAILED) {
        cout << "Could not memory map file: " << file_path << endl;
        exit(1);
    }
    madvise(mapped, statbuf.st_size, MADV_WILLNEED);
    return mapped;
}

void ListGrids(uint64_t grid_id, uint64_t limit) {
    void *table = MMapFile("grid.counts");
    void *index = MMapFile("grid.index");

    char grid[81];
    for (int i = 0; i < limit; i++) {
        GetGrid(grid_id + i, index, table, grid);
        printf("%.81s\t%zu\n", grid, grid_id + i);
    }
}

constexpr size_t num_equivalence_classes = 27704267971ll * 128;

namespace {
    std::random_device rd{};
    std::mt19937_64 rng{rd()};
    std::uniform_int_distribution<uint64_t> random_uint{};
}

void SampleGrids(int64_t limit) {
    void *table = MMapFile("grid.counts");
    void *index = MMapFile("grid.index");

    char grid[81];
    while (limit-- != 0) {
        size_t grid_id = random_uint(rng) % num_equivalence_classes;
        GetGrid(grid_id, index, table, grid);
        printf("%.81s\t%zu\n", grid, grid_id);
    }
}

// when sampling a grid each equally probable permutation leads to exactly zero or one
// minimal puzzles. however, different puzzles may arise with different probabilities since
// there are different numbers of permutations leading to them. for a minimal puzzle with k
// clues there are exactly k!(81-k)! permutations that lead to it, a fact which favors
// sampling of low-clue puzzles. here we compute a weighting factor that down-weights
// oversampled puzzles to put them all on the same footing. the weight is scaled so that
// the least likely puzzles (with 40 or 41 clues) get a weight of 1.0.
static double SamplingWeight(int clues) {
    return exp(lgamma(41) + lgamma(42) - lgamma(clues + 1) - lgamma(82 - clues));
}

void SamplePuzzles(int64_t limit) {
    void *table = MMapFile("grid.counts");
    void *index = MMapFile("grid.index");

    char grid[81];
    while (limit != 0) {
        size_t grid_id = random_uint(rng) % num_equivalence_classes;
        GetGrid(grid_id, index, table, grid);
        if (TdokuMinimize(false, true, grid)) {
            int num_clues = 0;
            for (char c : grid) num_clues += (c != '.');
            double weight = SamplingWeight(num_clues);
            printf("%.81s\t%f\n", grid, weight);
            limit--;
        }
    }
}

void usage() {
    cout << R"USAGE(
This program provides tools for counting the set of essentially different
Sudoku grids and sampling from this set with uniform probability.

It also supports sampling from the set of essentially different minimal
Sudoku puzzles with uniform conditional probability given clue count. The
sampler is biased towards producing puzzles with fewer clues, but this
bias is quantified and puzzles are produced along with weights that can
be used to adjust statistics if desired.

To achieve these goals we first enumerate all the essentially different
configurations of the first band and stack of the board (leaving the 6x6
cells at the bottom right unconstrained). For each of these configurations
we then use the solver to count the number of possible grid completions.
Finally we use these counts to construct a table and index that allow mapping
any of the 27704267971*2^7 grid classes to a configuration and solution offset.

To construct these tables use the command below. If you have downloaded
pre-generated tables you can skip this step and the next.

  chunksize=$((36228 * 36228 / 64))
  for i in {0..63}; do
    build/grid_tools count_grids $(( i * chunksize )) $(( (i + 1) * chunksize )) > chunk.$i &
  done
  wait

Adjust the parallelism used above as appropriate for your platform. On a
Threadripper 2990WX with 64 processes this takes about 8 hours.

Now make the tables, taking care to consume the chunks of counts in order:

  build/grid_tools make_tables < <(cat chunk.{0..63})

With the generated tables and index in the current directory you can now get
any numbered grid in range(27704267971*2^7), or you can sample grids randomly:

  build/grid_tools list_grids <first_gird_id> [<limit>=1]
  build/grid_tools sample_grids [<limit>=-1]

You can also sample minimal puzzles via a very slow rejection sampling procedure
like so:

  build/grid_gools sample_puzzles [<limit>=-1]

)USAGE";
    exit(0);
}

int main(int argc, char **argv) {
    if (argc > 1) {
        string command(argv[1]);
        if (command == "pattern") {
            if (argc > 2) {
                uint64_t start = stoull(argv[2]);
                uint64_t limit = argc > 3 ? stoull(argv[3]) : 1u;
                ListPatterns(start, limit);
                return 0;
            }
        } else if (command == "count_grids") {
            if (argc == 4) {
                int start = stoi(argv[2]);
                int limit = stoi(argv[3]);
                CountGrids(start, limit);
                return 0;
            }
        } else if (command == "make_tables") {
            MakeTables();
            return 0;
        } else if (command == "list_grids") {
            if (argc > 2) {
                uint64_t start = stoull(argv[2]);
                uint64_t limit = argc > 3 ? stoull(argv[3]) : 1u;
                ListGrids(start, limit);
                return 0;
            }
        } else if (command == "sample_grids") {
            int64_t limit = argc > 2 ? stoll(argv[2]) : -1;
            SampleGrids(limit);
            return 0;
        } else if (command == "sample_puzzles") {
            int64_t limit = argc > 2 ? stoll(argv[2]) : -1;
            SamplePuzzles(limit);
            return 0;
        }
    }
    usage();
}
