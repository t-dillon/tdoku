import sys

def merge_results(filename, results):
    f = open(filename)
    for line in f:
        if 'puzzles' in line:
            name = line.split("|")[1].strip()
            if name not in results:
                results[name] = {}
        elif line and line[0] == '|' and ':' not in line:
            toks = line.split("|")
            solver = toks[1].strip()
            val = float(toks[2].replace(",",""))
            if solver not in results[name] or results[name][solver] < val:
                results[name][solver] = val

solvers = ['_tdev_basic', '_tdev_basic_heuristic', 'minisat_minimal_01', 'minisat_natural_01',
           'minisat_complete_01', 'minisat_augmented_01', '_tdev_dpll_triad', '_tdev_dpll_triad_scc_i',
           '_tdev_dpll_triad_scc_h', '_tdev_dpll_triad_scc_ih', 'norvig', 'fast_solv_9r2', 'kudoku',
           'bb_sudoku', 'jsolve', 'fsss2', 'fsss2_locked', 'jczsolve', 'sk_bforce2', 'rust_sudoku', 'tdoku']

def main(args):
    results = {}
    for f in args:
        merge_results(f, results)

    for s in solvers:
        print("%30s" % s, end = " ")
        for d in sorted(results):
            print("%10.1f" % (results[d][s]), end = " ")
        print()

if __name__ == '__main__':
    main(sys.argv[1:])
