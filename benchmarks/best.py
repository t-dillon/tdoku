import sys

solver_order = ['minisat_minimal',
                'minisat_natural',
                'minisat_complete',
                'minisat_augmented',
                '_tdev_dpll_triad',
                '_tdev_dpll_triad_scc_i',
                '_tdev_dpll_triad_scc_h',
                '_tdev_dpll_triad_scc_ih',
                '_tdev_basic',
                '_tdev_basic_heuristic',
                'lhl_sudoku',
                'zerodoku',
                'fast_solv_9r2',
                'kudoku',
                'norvig',
                'bb_sudoku',
                'fsss',
                'jsolve',
                'fsss2',
                'fsss2_locked',
                'jczsolve',
                'sk_bforce2',
                'rust_sudoku',
                'tdoku']

solvers_found = set()

def merge_results(filename, results, best):
    f = open(filename)
    for line in f:
        if 'puzzles' in line:
            name = line.split("|")[1].split(" ")[0]
            if name not in results:
                results[name] = {}
        elif line and line[0] == '|' and ':' not in line:
            toks = line.split("|")
            solver = toks[1].split(" ")[0]
            solvers_found.add(solver)
            val = float(toks[2].replace(",",""))
            if solver not in results[name] or results[name][solver] < val:
                results[name][solver] = val
                best[solver + ':' + name] = filename

def main(args):
    results = {}
    best = {}
    mode = 'summary'

    if len(args) == 0:
        print("python3 best.py [-best] prefix*")
        exit(0)

    if args[0] == "-best":
        mode = 'best'
        args = args[1:]

    for f in args:
        if 'best' not in f:
            merge_results(f, results, best)

    if mode == 'best':
        print("\nBest compiler by solver+dataset:")
        for k in best:
            print("%-30s %-40s  %s" % (k.split(":")[0], k.split(":")[1], best[k].split("/")[-1]))
    else:
        print("\nPerformance (puzzles/sec) by solver X dataset given each's best compiler configuration")
        print("-" * 124)
        print(" " * 25, end='')
        for k in sorted(results):
            print("%11s" % k.split("/")[-1][0:8], end='')
        print()
        print("-" * 124)
        for s in solver_order:
            if s in solvers_found:
                print("%-25s" % s, end = " ")
                for d in sorted(results):
                    print("%10.1f" % (results[d][s]), end = " ")
                print()

if __name__ == '__main__':
    main(sys.argv[1:])
