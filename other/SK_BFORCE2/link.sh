#!/bin/bash

[ -d "$1" ] || (echo "SK_BFORCE2 directory '$1' does not exist" ; exit 1)

skbforce2=$(cd $1; pwd)
cd $(dirname "${BASH_SOURCE[0]}")

for f in sk_bitfields.cpp \
         sk_bitfields.h \
         sk_t.cpp \
         sk_t.h \
         Zhn_cpp.h \
         Zhn.h \
         Zhtables_cpp.h
do
    ln -s ${skbforce2}/${f}
done

