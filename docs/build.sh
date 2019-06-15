#!/bin/sh

# the columns arg is necessary to get the tables to format to the same width.
pandoc -o index.html -s -c pandoc.css --columns=50 --mathjax sudoku.md
