/*
 * SUDOKU FAST SOLVER v.9 r2
 * Copyright (c) 2007,2008, Ignacio Zelaya
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the author nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Ignacio Zelaya ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL Ignacio Zelaya BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * primer release publico: (r y r2)
 * http://www.setbb.com/sudoku/viewtopic.php?p=9689&mforum=sudoku#9689
 */

#include <stdio.h>   // printf(), putchar(), fgets(), <stdin>
#include <stdlib.h>  // atoi()
#include <string.h>  // strlen()
#include <unistd.h>  // getopt()

typedef struct nodo {
    struct nodo *R, *L, *U, *D;
    struct columna *C;
} nodo;

typedef struct columna {
    nodo link;
    int S;
    int N;
    struct columna *R, *L;
} columna;


#define max_elem_sol 81+1
#define max_cols 325
#define max_nodos 2916
#define head arr_col[0]

// GLOBAL vars
// (1) user opts:
int verbose = 0;       // default=1 (para que imprima solucion)
int SEARCH_LIMIT = 1;  // soluciones SEARCH_LIMIT:  1 para velocidad  2: para verificar validez >2 busca mas soluciones

// (2) dlx stuff   all initializad in make_links()
static columna arr_col[max_cols];   // columnas dlx
static nodo arr_nod[max_nodos];     // nodos dlx
static int max_rows = 0;            // max rows es la cantidad maxima de filas/nodos bajo la columna con mayor filas/nodos
static columna *col_adr[1 << 12];   // en la pos "nombre de col" guarda el puntero a la columan de ese nombre
nodo *sol[max_elem_sol];            // array donde se guargan los nodos solucion (solo se gurada un nodo del vector)
int matrix[81];                     // matriz con clues/solucion. solve() lee esta matriz antes de resolver;

// declaracion de funciones
int make_links(void);

int solve(void);

static void cover_col(columna *);

static void uncover_col(columna *);

void prn_sol(void);

void uso(void);

// Benchmark entry ------------------------------------------------------
int fast_solv_92r_undone = 0;

int fast_solv_9r2_solve(const char *buffer, int limit) {
    SEARCH_LIMIT = limit;
    fast_solv_92r_undone = 0;
    register int k, c;
    for (c = 0, k = 0; c < strlen(buffer); c++) {
        if (k < 81) {
            matrix[k] = buffer[c] - 46;
            if (matrix[k]) {   // si no es un .
                matrix[k] -= 2;
                if (matrix[k] < 0 || matrix[k] > 9) { k = -1; }  // caracter invalido, reseteamos k
            }
            k++;
        }
    }
    return solve();
}

// Main -----------------------------------------------------------------
int fast_solv_9r2_main(int argc, char *argv[]) {
    char buffer[300];                 // 300 chars space to read a line
    char ch;
    register int k, c, prob = 0;
    register int soluc = 0;
    register int games = 0;             // sudokus generados
    register int status = 0;

    //analizamos la linea de comando
    while ((ch = getopt(argc, argv, "s:qvh")) != -1) {
        switch (ch) {
            case 's':
                if (1 > (SEARCH_LIMIT = atoi(optarg))) status = 1;
                break;
            case 'q':
                verbose = 0;
                break;
            case 'v':
                verbose++;
                break;
            case 'h':
            default: {
                uso();
                return (0);
            }
        }
    }
    if (status) {
        (void) printf("ERROR: parametros incorrectos.\t-h (help).\n");
        return (status);
    }

    if (make_links()) { return (1); }

    while (fgets(buffer, 300, stdin)) {

        if (strlen(buffer) < 81) { continue; } // linea muy corta
        for (c = 0, k = 0; c < strlen(buffer); c++) {
            if (k < 81) {
                matrix[k] = buffer[c] - 46;
                if (matrix[k]) {   // si no es un .
                    matrix[k] -= 2;
                    if (matrix[k] < 0 ||
                        matrix[k] > 9) { k = -1; }  // caracter invalido, reseteamos k
                }
                k++;
            }
        }
        if (k !=
            81) { continue; } // terminamos de leer la linea. Si no hay 81 caracteres validos, continuamos

        prob++;
        status = 0;
        status = solve();
        if (status == 1) {
            games++;
            soluc++;
        }
        if (status > 1) {
            soluc += status;
            (void) printf("^ above problem, more than 1 sol\n");
        } // evitar sumar status negativos
        if (status < 0) { (void) printf("Invalid: incorrect data in problem # %d\n", prob); }
        if (status == 0) { (void) printf("Invalid: no solution for problem # %d\n", prob); }
    }
    if (verbose > 1) {
        (void) printf("\nProblems loaded: %d  Solutions found: %d\n", prob, soluc);
        if (SEARCH_LIMIT > 1) {
            (void) printf("Valid sudokus (with exact 1 solution): %d\n", games);
        } else {
            (void) printf(
                    "Warning: some problems could have more than one 1 solutions (use -s2 to verify)\n");
        }
    }
    if (!verbose) { (void) printf("Total: %d\n", games); }

    return (0);
}

void uso(void) {
    (void) printf("usage:\n\
 \t -h help\n\
 \t -s<num> solve buscando num soluciones (default 1)\n\
 \t -v verbose (prints more info ans stats)\n\
 \t -q quiet (count solutions without printing them)\n");
}

//-----------------------------------------------
int solve(void) {

    register columna *col, *n;
    register nodo *ve, *ho;
    register int i, j, x, f, c, b, nu;
    int tapadas[max_cols]; // en orden, guarda el nombre de cada col tapada. para usar con col_adr
    int tcount = 0;            // contador de tapadas
    int min;
    int soluciones = 0;
    int level = 0;             // identificador de nivel de recursion


    // carga un problema que se encuentra en matrix
    for (i = 9; i > 0; i--) {
        for (j = 9; j > 0; j--) {
            if ((nu = matrix[9 * (i - 1) + (j - 1)])) {
                x = (0x100) + (i << 4) + j;
                f = (0x200) + (i << 4) + nu;
                c = (0x400) + (j << 4) + nu;
                b = (0x800) + ((3 * ((i - 1) / 3) + ((j - 1) / 3) + 1) << 4) + nu;
                if (0x100 & (col_adr[x]->S | col_adr[f]->S | col_adr[c]->S | col_adr[b]->S)) {
                    soluciones = -1;    // seteamos soluciones en -1 para indicar error en los datos.
                    goto FIN;           // No retornamos aqui, vamos a fin para deshacer enlaces hechos antes del error
                }
                tapadas[++tcount] = x;
                cover_col(col_adr[x]);
                tapadas[++tcount] = f;
                cover_col(col_adr[f]);
                tapadas[++tcount] = c;
                cover_col(col_adr[c]);
                tapadas[++tcount] = b;
                cover_col(col_adr[b]);
            }
        }
    }

    NEXT:
    min = max_rows;
    col = head.R;
    for (n = head.R; n != &(head); n = n->R) {
        if (n->S < min) {
            min = n->S;
            col = n;
            if (min < 2) { break; }
        }
    }
    if (min > 1) fast_solv_92r_undone++;
    cover_col(col);
    for (ve = col->link.D; ve != &(col->link); ve = ve->D) {
        sol[level] = ve;
        for (ho = ve->R; ho != ve; ho = ho->R) {
            cover_col(ho->C);
        }
        level++;
        if (head.R == &head) {
            soluciones++;
            if (verbose && soluciones <= SEARCH_LIMIT) { prn_sol(); }
            goto UNDO;
        }
        if (soluciones <
            SEARCH_LIMIT) { goto NEXT; }  // solo buscar hasta encontrar SEARCH_LIMIT soluciones
        UNDO:
        level--;
        ve = sol[level];
        sol[level] = NULL;
        col = ve->C;
        for (ho = ve->L; ho != ve; ho = ho->L) {
            uncover_col(ho->C);
        }
    }
    uncover_col(col);
    if (level == 0) { goto FIN; }
    goto UNDO;
    FIN:
    do {
        uncover_col(col_adr[tapadas[tcount--]]);
    } while (tcount > 0);
    return (soluciones);
}


// cover ----------------------------------------------------------------
static void cover_col(columna *col) {
    register nodo *nodo_d;
    register nodo *nodo_r;

    col->R->L = col->L;
    col->L->R = col->R;
    col->S |= 0x100;
    for (nodo_d = col->link.D; nodo_d != &(col->link); nodo_d = nodo_d->D) {

        nodo_r = nodo_d->R;  // abajo derecha
        nodo_r->D->U = nodo_r->U;
        nodo_r->U->D = nodo_r->D;
        nodo_r->C->S -= 1;

        nodo_r = nodo_r->R; // derecha
        nodo_r->D->U = nodo_r->U;
        nodo_r->U->D = nodo_r->D;
        nodo_r->C->S -= 1;

        nodo_r = nodo_r->R; // derecha
        nodo_r->D->U = nodo_r->U;
        nodo_r->U->D = nodo_r->D;
        nodo_r->C->S -= 1;
    }
}


// uncover ----------------------------------------------------------------
static void uncover_col(columna *col) {
    register nodo *nodo_u;
    register nodo *nodo_l;

    for (nodo_u = col->link.U; nodo_u != &(col->link); nodo_u = nodo_u->U) {

        nodo_l = nodo_u->L; // arriba izquierda
        nodo_l->C->S += 1;
        nodo_l->U->D = nodo_l;
        nodo_l->D->U = nodo_l;

        nodo_l = nodo_l->L; // izquierda
        nodo_l->C->S += 1;
        nodo_l->U->D = nodo_l;
        nodo_l->D->U = nodo_l;

        nodo_l = nodo_l->L; // izquierda
        nodo_l->C->S += 1;
        nodo_l->U->D = nodo_l;
        nodo_l->D->U = nodo_l;
    }
    col->S &= 0x0FF;
    col->L->R = col;
    col->R->L = col;
}


// prn_sol ----------------------------------------------------------------
void prn_sol(void) {
    register int i, j = 0, f, x, nu = 0;
    //  register nodo *s;
    int prn_arr[81];
    char c;

    for (f = 0; sol[f] != NULL; f++) {
        x = sol[f]->C->N;
        if (x < 0x200) {      // es coord 1ij   1<<9 = 0x200
            i = (x & 0xF0) >> 4;
            j = x & 0x0F;
            nu = sol[f]->R->C->N & 0x0F;
        } else if (x < 0x400) { // es coord 2fn  1<<10 = 0x400
            i = (x & 0xF0) >> 4;
            nu = x & 0x0F;
            j = sol[f]->L->C->N & 0x0F;
        } else if (x < 0x800) { // es coord 3cn 1<<10 = 0x800
            nu = x & 0x0F;
            j = (x & 0xF0) >> 4;
            i = (sol[f]->L->C->N & 0xF0) >> 4;
        } else { // es box
            i = (sol[f]->R->C->N & 0xF0) >> 4;
            j = sol[f]->R->C->N & 0x0F;
            nu = sol[f]->R->R->C->N & 0x0F;
        }
        prn_arr[9 * (i - 1) + (j - 1)] = nu; //  agregamos dato  solucion
    }
    for (i = 0; i < 81; i++) {
        if ((c = matrix[i])) {
            putchar(c + 48);  // imprimimos solucion
        } else { putchar(prn_arr[i] + 48); }
    }
    putchar(10);
}

// make_links ------------------------------------------------------------
int make_links(void) {
    int i, j, k, f, c, b, n;
    int id_nodo = 0;        // ultimo nodo colocado
    columna *col;

    // data para  sudoku
    int Pcols = 324;          // columnas principales
    int data_rows = 729;      // cantidad de filas (verctores)
    int nodes_per_row = 4;    // cantidad de datos por cada fila (vector)

    int filas[729 * 4];
    int c1[324];

    // generamos los datos de las columnas.
    // Casillas=1ij  Filas=2fn  Columnas=3cn  Box=4bn
    c = 0;
    for (k = 8; k < 12; k++) {
        for (f = 1; f < 10; f++) {
            for (n = 1; n < 10; n++) {
                c1[c] = (1 << k) + (f << 4) + n;
                c++;
            }
        }
    }

    // generamos los datos de las filas filas
    k = 0;
    for (f = 1; f < 10; f++) {
        for (c = 1; c < 10; c++) {
            b = (3 * ((f - 1) / 3) + ((c - 1) / 3) + 1);
            for (n = 1; n < 10; n++) { // para los numeros 1..9
                filas[k] = (1 << 8) + (f << 4) + c;
                filas[k + 1] = (1 << 9) + (f << 4) + n;
                filas[k + 2] = (1 << 10) + (c << 4) + n;
                filas[k + 3] = (1 << 11) + (b << 4) + n;
                k += 4;
            }
        }
    }
    // fin generar datos sudoku

    head.R = &head;
    head.L = &head;
    head.S = 0;
    head.N = 0;

    // columnas principales
    for (i = 1; i <= Pcols; i++) {
        arr_col[i].R = &head;
        arr_col[i].L = head.L;
        arr_col[i].R->L = &arr_col[i];
        arr_col[i].L->R = &arr_col[i];
        arr_col[i].link.U = &arr_col[i].link;
        arr_col[i].link.D = &arr_col[i].link;
        arr_col[i].link.C = &arr_col[i];
        arr_col[i].S = 0;
        arr_col[i].N = c1[i - 1];
        col_adr[arr_col[i].N] = &arr_col[i];
    }

    // rows
    id_nodo = 0;
    for (i = 0; i < data_rows; i++) {
        for (j = 0; j < nodes_per_row; j++) {
            col = &(head);
            while (col->N != filas[i * 4 + j]) {
                col = col->R;
                if (col == &(head)) {
                    (void) printf("columna no encontrada: %d\n", filas[i * 4 + j]);
                    return (1);
                }
            }
            arr_nod[id_nodo].C = col;
            arr_nod[id_nodo].U = col->link.U;
            arr_nod[id_nodo].D = &(col->link);
            arr_nod[id_nodo].U->D = &(arr_nod[id_nodo]);
            arr_nod[id_nodo].D->U = &(arr_nod[id_nodo]);

            if (j == 0) { // si es el primero
                arr_nod[id_nodo].R = &(arr_nod[id_nodo]);
                arr_nod[id_nodo].L = &(arr_nod[id_nodo]);
            } else {
                arr_nod[id_nodo].R = arr_nod[id_nodo - 1].R;
                arr_nod[id_nodo].L = &(arr_nod[id_nodo - 1]);
                arr_nod[id_nodo].R->L = &(arr_nod[id_nodo]);
                arr_nod[id_nodo].L->R = &(arr_nod[id_nodo]);
            }
            col->S += 1;
            if (col->S > max_rows) {
                max_rows = col->S;
            }
            id_nodo++;
        }
    }
    max_rows++; // incrementamos en uno para que sea mayor siempre

    i = max_elem_sol - 1;
    do {
        sol[i] = NULL;
        i--;
    } while (i >= 0);

    for (i = 0; i < 81; i++) { // inicializar matriz
        matrix[i] = '0';
    }

    return (0);
}

