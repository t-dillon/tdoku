#!/bin/bash

[ -d "$1" ] || ! echo "rust sudoku directory '$1' does not exist" || exit 1

dest=$(cd $(dirname "${BASH_SOURCE[0]}"); pwd)
rust_sudoku=$(cd $1; pwd)

setup() {
    cd $rust_sudoku
    mkdir -p $dest/clang-$1
    rustup toolchain install $2
    rustup default $2
    cargo clean
    cargo build --release
    cp target/release/libsudoku.so $dest/clang-$1
}

setup 9  1.38.0-x86_64-unknown-linux-gnu
setup 10 1.45.0-x86_64-unknown-linux-gnu
setup 11 1.47.0-x86_64-unknown-linux-gnu

