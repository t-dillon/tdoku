#!/bin/bash

[ -d "$1" ] || (echo "gss directory '$1' does not exist" ; exit 1)

gss=$(cd $1; pwd)
cd $(dirname "${BASH_SOURCE[0]}")

for f in sudoku.c \
         sudoku.h \
         sudoku_parser.c \
         sudokusolver.h \
         sudoku_parser.h
do
    ln -s ${gss}/${f}
done


