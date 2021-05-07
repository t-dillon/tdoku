#!/bin/bash

for f in results_*; do
    python3 ./best.py $f/* > $f/best_results.txt
    python3 ./best.py -best $f/* | sort > $f/best_compilers.txt
done
