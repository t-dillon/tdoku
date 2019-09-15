#!/bin/bash

[ -d "$1" ] || (echo "norvig sudoku directory '$1' does not exist" ; exit 1)

norvig=$(cd $1; pwd)
cd $(dirname "${BASH_SOURCE[0]}")

ln -s ${norvig}/sudoku.en.cc
