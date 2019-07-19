#!/bin/bash

[ -d "$1" ] || (echo "rust sudoku directory '$1' does not exist" ; exit 1)

rust_sudoku=$(cd $1; pwd)
cd $(dirname "${BASH_SOURCE[0]}")
ln -s ${rust_sudoku}/target/release/libsudoku.so
