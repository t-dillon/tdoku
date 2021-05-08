#!/bin/bash

script_dir=$(cd $(dirname "${BASH_SOURCE[0]}"); pwd)

setup() {
    cd $script_dir/module_rust_sudoku
    mkdir clang-$1
    rustup toolchain install $2
    rustup default $2
    cargo clean
    cargo build --release
    cp target/release/libsudoku.so clang-$1
}

setup 9  1.38.0-x86_64-unknown-linux-gnu
setup 10 1.45.0-x86_64-unknown-linux-gnu
setup 11 1.47.0-x86_64-unknown-linux-gnu

