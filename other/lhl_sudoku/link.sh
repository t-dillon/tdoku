#!/bin/bash

[ -d "$1" ] || (echo "lhl sudoku directory '$1' does not exist" ; exit 1)

lhl=$(cd $1; pwd)
cd $(dirname "${BASH_SOURCE[0]}")

ln -s ${lhl}/Sudoku2.cpp
