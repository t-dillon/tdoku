#!/bin/bash

[ -d "$1" ] || (echo "fsss2 directory '$1' does not exist" ; exit 1)

fsss2=$(cd $1; pwd)
cd $(dirname "${BASH_SOURCE[0]}")

for f in fsss2.cpp \
         fsss2.h \
         t_128.cpp \
         t_128.h
do
    ln -s ${fsss2}/${f}
done

