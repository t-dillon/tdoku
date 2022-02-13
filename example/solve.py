
import sys
from ctypes import *

class Tdoku:
    def __init__(self):
        self.__tdoku = CDLL("build/libtdoku_shared.so")

        self.__solve = self.__tdoku.TdokuSolverDpllTriadSimd
        self.__solve.restype = c_ulonglong

        self.__constrain = self.__tdoku.TdokuConstrain
        self.__constrain.restype = c_bool

        self.__minimize = self.__tdoku.TdokuMinimize
        self.__minimize.restype = c_bool

    def Solve(self, puzzle):
        if type(puzzle) is str:
            puzzle = str.encode(puzzle)
        limit = c_ulonglong(1)
        config = c_ulong(0)
        solution = create_string_buffer(81)
        guesses = c_ulonglong(0)
        count = self.__solve(c_char_p(puzzle), limit, config, solution, pointer(guesses))
        if count:
            return count, solution.value.decode(), guesses.value
        else:
            return 0, "", guesses

    def Count(self, puzzle, limit=2):
        if type(puzzle) is str:
            puzzle = str.encode(puzzle)
        limit = c_ulonglong(limit)
        config = c_ulong(0)
        solution = create_string_buffer(81)
        guesses = c_ulonglong(0)
        count = self.__solve(c_char_p(puzzle), limit, config, solution, pointer(guesses))
        return count

    def Constrain(self, partial_puzzle):
        if type(partial_puzzle) is str:
            buffer = create_string_buffer(str.encode(partial_puzzle))
        else:
            buffer = create_string_buffer(partial_puzzle)
        pencilmark = c_bool(False)
        self.__constrain(pencilmark, buffer)
        return buffer.value

    def Minimize(self, non_minimal_puzzle):
        if type(non_minimal_puzzle) is str:
            buffer = create_string_buffer(str.encode(non_minimal_puzzle))
        else:
            buffer = create_string_buffer(non_minimal_puzzle)
        pencilmark = c_bool(False)
        monotonic = c_bool(False)
        self.__minimize(pencilmark, monotonic, buffer)
        return buffer.value


if __name__ == '__main__':
    tdoku = Tdoku()

    if len(sys.argv) > 1:
        filename = sys.argv[1]
    else:
        filename = 'data/puzzles2_17_clue'

    with open(filename, 'r') as f:
        for puzzle in f.readlines():
            if len(puzzle) >= 81 and not puzzle.startswith('#'):
                count, solution, guesses = tdoku.Solve(puzzle)
                print("%.81s:%lu:%.81s:%lu" % (puzzle, count, solution, guesses))

