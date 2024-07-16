emcc example/solve.c build/libtdoku_static.a -O3 -o solve -lstdc++ -lm -o example/solve.js -sEXPORTED_FUNCTIONS=_solve -sEXPORTED_RUNTIME_METHODS=ccall
