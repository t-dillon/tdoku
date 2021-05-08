#!/bin/bash

script_dir=$(cd $(dirname "${BASH_SOURCE[0]}"); pwd)
jczsolve_dir=$script_dir/jczsolve

mkdir -p $jczsolve_dir
cd $jczsolve_dir

curl "http://forum.enjoysudoku.com/download/file.php?id=436&sid=2906ffe2e2c5cf10c4d004f184eafe10" > jczsolve.zip
unzip -jo jczsolve.zip JCZSolve10/JCZSolve.?

patch JCZSolve.c JCZSolve.c.diff
