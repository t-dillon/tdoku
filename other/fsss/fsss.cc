// Dropped unneeded main(), added guess counting.     -td
// Concatenated two source files; renamed the file.   -ac

#include <cstdint>

/***************************************************************************
A Fast Simple Sudoku Solver by Mladen Dobrichev

For commercial users the actions allowed to this file are limited to erasing.
No limitations to other users, but citing somewhere my name is welcome.
There are parts of code taken from bb_sudoku by Brian Turner. Thank you, Brian!

The entry point is
	int solve(char* in, char* out, int mode).

Compiler flags:
-c -O2 -Ob1 -Oi -Ot -Oy -GA -MT -GS- -GR- -Wp64 -Gr
-Qopenmp -Qftz -QxSSE2 -Qopenmp-link:static
Which means:
compile_only, optimize_for_speed, expands_only_functions_marked_as_inline,
use_intrinsic_functions, favor_fast_code, omit_frame_pointers, fast_access_to_tls_storage,
multithreaded_executable, no_buffer_security_check, disable_runtime_type_info,
detect_64-bit_portability_issues, use__fastcall_calling_convention,
generate_parallel_code, flush_denormal_fp_result_to_zero, use_SSE2_instructions,
link_statically_to_openmp

Some counterintuitive compilation hints:
- no global optimizations, only for speed
- no inlining
- no interprocedural optimizations
- no Profile Guided Optimizations

Terminology:
"Game" is the puzzle along with the whole context during processing.
Puzzle consist of 81 "Cells".
Each cell is member of three "Groups" - its "Row", "Column", and "Square".
The rest of the cells within the same row, column, and square are "Affected Cells".
Segment is a combination of first/second/third three rows or columns.
"Triplets" are first/second/third subsequent 3 cells within the rows and columns.
"Affected Triplets" for a given triplet are the rest of the triplets in the same row/col and square.
***************************************************************************/

//No global variables
//No runtime integer division or multiplication
//Runtime Library function calls are limited to memcpy (hidden, when assigning the game struct), setjmp, and longjmp.

//There is an Intel Compiler specific pragma "unroll" which causes significant improvement in the SetDigit function.
//It is critical this loop to be unrolled to obtain good performance. Splitting the loop into chunks
//may cause other compilers to unroll them. Last resort is manual unrolling.
//#define __INTEL_COMPILER
#ifndef __INTEL_COMPILER
#define MANUAL_UNROLL
#endif //__INTEL_COMPILER

//A quick way to experiment how data representation affects the performance
typedef unsigned int cellIndex;
typedef unsigned char cellDigit;
typedef unsigned short bitmap;

//Use constants and tabular functions whenever possible

//Convert a number to a mask with only the appropriate bit set
static const unsigned int Digit2Bitmap[17] =
{
	0x00000000,
	0x00000001, 0x00000002, 0x00000004, 0x00000008,
	0x00000010, 0x00000020, 0x00000040, 0x00000080,
	0x00000100, 0x00000200, 0x00000400, 0x00000800,
	0x00001000, 0x00002000, 0x00004000, 0x00008000
};

//Convert mask to a number (1 to 9) if only the appropriate bit is set.
//Convert zero to 9, allowing cellDigit to char conversion using the same table.
//Convert all the rest to zero.
static const unsigned int Bitmap2Digit[512] =
{
        9,1,2,0,3,0,0,0,4,0,0,0,0,0,0,0,5,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        7,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        8,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        9,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

//I am using hardware w/o SSE4 and POPCNT instruction
static const int BitCount[512] =
{
        0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
        1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
        1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
        1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
        3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
        1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
        3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
        2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
        3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
        3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,
        4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8,5,6,6,7,6,7,7,8,6,7,7,8,7,8,8,9
};

/* Zero based cell enumeration
 9 10 11  12 13 14  15 16 17    Group

00 01 02  03 04 05  06 07 08	0
09 10 11  12 13 14  15 16 17	1	18	19	20
18 19 20  21 22 23  24 25 26	2

27 28 29  30 31 32  33 34 35	3
36 37 38  39 40 41  42 43 44	4	21	22	23
45 46 47  48 49 50  51 52 53	5

54 55 56  57 58 59  60 61 62	6
63 64 65  66 67 68  69 70 71	7	24	25	26
72 73 74  75 76 77  78 79 80	8
*/

//Cells' groups - row, column, square
static const int affectedGroups[81][3] =
{
	{0, 9,18},{0,10,18},{0,11,18},{0,12,19},{0,13,19},{0,14,19},{0,15,20},{0,16,20},{0,17,20},
	{1, 9,18},{1,10,18},{1,11,18},{1,12,19},{1,13,19},{1,14,19},{1,15,20},{1,16,20},{1,17,20},
	{2, 9,18},{2,10,18},{2,11,18},{2,12,19},{2,13,19},{2,14,19},{2,15,20},{2,16,20},{2,17,20},
	{3, 9,21},{3,10,21},{3,11,21},{3,12,22},{3,13,22},{3,14,22},{3,15,23},{3,16,23},{3,17,23},
	{4, 9,21},{4,10,21},{4,11,21},{4,12,22},{4,13,22},{4,14,22},{4,15,23},{4,16,23},{4,17,23},
	{5, 9,21},{5,10,21},{5,11,21},{5,12,22},{5,13,22},{5,14,22},{5,15,23},{5,16,23},{5,17,23},
	{6, 9,24},{6,10,24},{6,11,24},{6,12,25},{6,13,25},{6,14,25},{6,15,26},{6,16,26},{6,17,26},
	{7, 9,24},{7,10,24},{7,11,24},{7,12,25},{7,13,25},{7,14,25},{7,15,26},{7,16,26},{7,17,26},
	{8, 9,24},{8,10,24},{8,11,24},{8,12,25},{8,13,25},{8,14,25},{8,15,26},{8,16,26},{8,17,26}
};

//The cell indexes in each of the 9 rows, 9 columns, and 9 squares
static const cellIndex cellsInGroup[27][9] =
{
	{ 0, 1, 2, 3, 4, 5, 6, 7, 8},{ 9,10,11,12,13,14,15,16,17},{18,19,20,21,22,23,24,25,26},
	{27,28,29,30,31,32,33,34,35},{36,37,38,39,40,41,42,43,44},{45,46,47,48,49,50,51,52,53},
	{54,55,56,57,58,59,60,61,62},{63,64,65,66,67,68,69,70,71},{72,73,74,75,76,77,78,79,80},
	{ 0, 9,18,27,36,45,54,63,72},{ 1,10,19,28,37,46,55,64,73},{ 2,11,20,29,38,47,56,65,74},
	{ 3,12,21,30,39,48,57,66,75},{ 4,13,22,31,40,49,58,67,76},{ 5,14,23,32,41,50,59,68,77},
	{ 6,15,24,33,42,51,60,69,78},{ 7,16,25,34,43,52,61,70,79},{ 8,17,26,35,44,53,62,71,80},
	{ 0, 1, 2, 9,10,11,18,19,20},{ 3, 4, 5,12,13,14,21,22,23},{ 6, 7, 8,15,16,17,24,25,26},
	{27,28,29,36,37,38,45,46,47},{30,31,32,39,40,41,48,49,50},{33,34,35,42,43,44,51,52,53},
	{54,55,56,63,64,65,72,73,74},{57,58,59,66,67,68,75,76,77},{60,61,62,69,70,71,78,79,80}
};

//Determining the digit of a cell forbids the same digit to be placed within the affected cells.
//8 cells in the row + 8 in the column + rest 4 in the square = total of 20.
//Below is the tabular function with all 20 affected cell indexes for each of the 81 cells.
static const cellIndex affectedCells[81][20] =
{
	{ 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,18,19,20,27,36,45,54,63,72},
	{ 0, 2, 3, 4, 5, 6, 7, 8, 9,10,11,18,19,20,28,37,46,55,64,73},
	{ 0, 1, 3, 4, 5, 6, 7, 8, 9,10,11,18,19,20,29,38,47,56,65,74},
	{ 0, 1, 2, 4, 5, 6, 7, 8,12,13,14,21,22,23,30,39,48,57,66,75},
	{ 0, 1, 2, 3, 5, 6, 7, 8,12,13,14,21,22,23,31,40,49,58,67,76},
	{ 0, 1, 2, 3, 4, 6, 7, 8,12,13,14,21,22,23,32,41,50,59,68,77},
	{ 0, 1, 2, 3, 4, 5, 7, 8,15,16,17,24,25,26,33,42,51,60,69,78},
	{ 0, 1, 2, 3, 4, 5, 6, 8,15,16,17,24,25,26,34,43,52,61,70,79},
	{ 0, 1, 2, 3, 4, 5, 6, 7,15,16,17,24,25,26,35,44,53,62,71,80},
	{ 0, 1, 2,10,11,12,13,14,15,16,17,18,19,20,27,36,45,54,63,72},
	{ 0, 1, 2, 9,11,12,13,14,15,16,17,18,19,20,28,37,46,55,64,73},
	{ 0, 1, 2, 9,10,12,13,14,15,16,17,18,19,20,29,38,47,56,65,74},
	{ 3, 4, 5, 9,10,11,13,14,15,16,17,21,22,23,30,39,48,57,66,75},
	{ 3, 4, 5, 9,10,11,12,14,15,16,17,21,22,23,31,40,49,58,67,76},
	{ 3, 4, 5, 9,10,11,12,13,15,16,17,21,22,23,32,41,50,59,68,77},
	{ 6, 7, 8, 9,10,11,12,13,14,16,17,24,25,26,33,42,51,60,69,78},
	{ 6, 7, 8, 9,10,11,12,13,14,15,17,24,25,26,34,43,52,61,70,79},
	{ 6, 7, 8, 9,10,11,12,13,14,15,16,24,25,26,35,44,53,62,71,80},
	{ 0, 1, 2, 9,10,11,19,20,21,22,23,24,25,26,27,36,45,54,63,72},
	{ 0, 1, 2, 9,10,11,18,20,21,22,23,24,25,26,28,37,46,55,64,73},
	{ 0, 1, 2, 9,10,11,18,19,21,22,23,24,25,26,29,38,47,56,65,74},
	{ 3, 4, 5,12,13,14,18,19,20,22,23,24,25,26,30,39,48,57,66,75},
	{ 3, 4, 5,12,13,14,18,19,20,21,23,24,25,26,31,40,49,58,67,76},
	{ 3, 4, 5,12,13,14,18,19,20,21,22,24,25,26,32,41,50,59,68,77},
	{ 6, 7, 8,15,16,17,18,19,20,21,22,23,25,26,33,42,51,60,69,78},
	{ 6, 7, 8,15,16,17,18,19,20,21,22,23,24,26,34,43,52,61,70,79},
	{ 6, 7, 8,15,16,17,18,19,20,21,22,23,24,25,35,44,53,62,71,80},
	{ 0, 9,18,28,29,30,31,32,33,34,35,36,37,38,45,46,47,54,63,72},
	{ 1,10,19,27,29,30,31,32,33,34,35,36,37,38,45,46,47,55,64,73},
	{ 2,11,20,27,28,30,31,32,33,34,35,36,37,38,45,46,47,56,65,74},
	{ 3,12,21,27,28,29,31,32,33,34,35,39,40,41,48,49,50,57,66,75},
	{ 4,13,22,27,28,29,30,32,33,34,35,39,40,41,48,49,50,58,67,76},
	{ 5,14,23,27,28,29,30,31,33,34,35,39,40,41,48,49,50,59,68,77},
	{ 6,15,24,27,28,29,30,31,32,34,35,42,43,44,51,52,53,60,69,78},
	{ 7,16,25,27,28,29,30,31,32,33,35,42,43,44,51,52,53,61,70,79},
	{ 8,17,26,27,28,29,30,31,32,33,34,42,43,44,51,52,53,62,71,80},
	{ 0, 9,18,27,28,29,37,38,39,40,41,42,43,44,45,46,47,54,63,72},
	{ 1,10,19,27,28,29,36,38,39,40,41,42,43,44,45,46,47,55,64,73},
	{ 2,11,20,27,28,29,36,37,39,40,41,42,43,44,45,46,47,56,65,74},
	{ 3,12,21,30,31,32,36,37,38,40,41,42,43,44,48,49,50,57,66,75},
	{ 4,13,22,30,31,32,36,37,38,39,41,42,43,44,48,49,50,58,67,76},
	{ 5,14,23,30,31,32,36,37,38,39,40,42,43,44,48,49,50,59,68,77},
	{ 6,15,24,33,34,35,36,37,38,39,40,41,43,44,51,52,53,60,69,78},
	{ 7,16,25,33,34,35,36,37,38,39,40,41,42,44,51,52,53,61,70,79},
	{ 8,17,26,33,34,35,36,37,38,39,40,41,42,43,51,52,53,62,71,80},
	{ 0, 9,18,27,28,29,36,37,38,46,47,48,49,50,51,52,53,54,63,72},
	{ 1,10,19,27,28,29,36,37,38,45,47,48,49,50,51,52,53,55,64,73},
	{ 2,11,20,27,28,29,36,37,38,45,46,48,49,50,51,52,53,56,65,74},
	{ 3,12,21,30,31,32,39,40,41,45,46,47,49,50,51,52,53,57,66,75},
	{ 4,13,22,30,31,32,39,40,41,45,46,47,48,50,51,52,53,58,67,76},
	{ 5,14,23,30,31,32,39,40,41,45,46,47,48,49,51,52,53,59,68,77},
	{ 6,15,24,33,34,35,42,43,44,45,46,47,48,49,50,52,53,60,69,78},
	{ 7,16,25,33,34,35,42,43,44,45,46,47,48,49,50,51,53,61,70,79},
	{ 8,17,26,33,34,35,42,43,44,45,46,47,48,49,50,51,52,62,71,80},
	{ 0, 9,18,27,36,45,55,56,57,58,59,60,61,62,63,64,65,72,73,74},
	{ 1,10,19,28,37,46,54,56,57,58,59,60,61,62,63,64,65,72,73,74},
	{ 2,11,20,29,38,47,54,55,57,58,59,60,61,62,63,64,65,72,73,74},
	{ 3,12,21,30,39,48,54,55,56,58,59,60,61,62,66,67,68,75,76,77},
	{ 4,13,22,31,40,49,54,55,56,57,59,60,61,62,66,67,68,75,76,77},
	{ 5,14,23,32,41,50,54,55,56,57,58,60,61,62,66,67,68,75,76,77},
	{ 6,15,24,33,42,51,54,55,56,57,58,59,61,62,69,70,71,78,79,80},
	{ 7,16,25,34,43,52,54,55,56,57,58,59,60,62,69,70,71,78,79,80},
	{ 8,17,26,35,44,53,54,55,56,57,58,59,60,61,69,70,71,78,79,80},
	{ 0, 9,18,27,36,45,54,55,56,64,65,66,67,68,69,70,71,72,73,74},
	{ 1,10,19,28,37,46,54,55,56,63,65,66,67,68,69,70,71,72,73,74},
	{ 2,11,20,29,38,47,54,55,56,63,64,66,67,68,69,70,71,72,73,74},
	{ 3,12,21,30,39,48,57,58,59,63,64,65,67,68,69,70,71,75,76,77},
	{ 4,13,22,31,40,49,57,58,59,63,64,65,66,68,69,70,71,75,76,77},
	{ 5,14,23,32,41,50,57,58,59,63,64,65,66,67,69,70,71,75,76,77},
	{ 6,15,24,33,42,51,60,61,62,63,64,65,66,67,68,70,71,78,79,80},
	{ 7,16,25,34,43,52,60,61,62,63,64,65,66,67,68,69,71,78,79,80},
	{ 8,17,26,35,44,53,60,61,62,63,64,65,66,67,68,69,70,78,79,80},
	{ 0, 9,18,27,36,45,54,55,56,63,64,65,73,74,75,76,77,78,79,80},
	{ 1,10,19,28,37,46,54,55,56,63,64,65,72,74,75,76,77,78,79,80},
	{ 2,11,20,29,38,47,54,55,56,63,64,65,72,73,75,76,77,78,79,80},
	{ 3,12,21,30,39,48,57,58,59,66,67,68,72,73,74,76,77,78,79,80},
	{ 4,13,22,31,40,49,57,58,59,66,67,68,72,73,74,75,77,78,79,80},
	{ 5,14,23,32,41,50,57,58,59,66,67,68,72,73,74,75,76,78,79,80},
	{ 6,15,24,33,42,51,60,61,62,69,70,71,72,73,74,75,76,77,79,80},
	{ 7,16,25,34,43,52,60,61,62,69,70,71,72,73,74,75,76,77,78,80},
	{ 8,17,26,35,44,53,60,61,62,69,70,71,72,73,74,75,76,77,78,79}
};

/*
Segment enumeration
0 0 0     3 4 5
1 1 1     3 4 5
2 2 2     3 4 5

Triplets within a segment enumeration
000 111 222
333 444 555
666 777 888
*/

static const int affectedTriplets[9][4] =
  {{1,2,3,6},{0,2,4,7},{0,1,5,8},{4,5,0,6},{3,5,1,7},{3,4,2,8},{7,8,0,3},{6,8,1,4},{6,7,2,5}};

//6 segments * 9 triplets * 12 affected cells
static const int tripletAffectedCells[6][9][12] =
{
	{
	{ 3, 4, 5, 6, 7, 8, 9,10,11,18,19,20},{ 0, 1, 2, 6, 7, 8,12,13,14,21,22,23},{ 0, 1, 2, 3, 4, 5,15,16,17,24,25,26},
	{12,13,14,15,16,17, 0, 1, 2,18,19,20},{ 9,10,11,15,16,17, 3, 4, 5,21,22,23},{ 9,10,11,12,13,14, 6, 7, 8,24,25,26},
	{21,22,23,24,25,26, 0, 1, 2, 9,10,11},{18,19,20,24,25,26, 3, 4, 5,12,13,14},{18,19,20,21,22,23, 6, 7, 8,15,16,17}
	},{
	{30,31,32,33,34,35,36,37,38,45,46,47},{27,28,29,33,34,35,39,40,41,48,49,50},{27,28,29,30,31,32,42,43,44,51,52,53},
	{39,40,41,42,43,44,27,28,29,45,46,47},{36,37,38,42,43,44,30,31,32,48,49,50},{36,37,38,39,40,41,33,34,35,51,52,53},
	{48,49,50,51,52,53,27,28,29,36,37,38},{45,46,47,51,52,53,30,31,32,39,40,41},{45,46,47,48,49,50,33,34,35,42,43,44}
	},{
	{57,58,59,60,61,62,63,64,65,72,73,74},{54,55,56,60,61,62,66,67,68,75,76,77},{54,55,56,57,58,59,69,70,71,78,79,80},
	{66,67,68,69,70,71,54,55,56,72,73,74},{63,64,65,69,70,71,57,58,59,75,76,77},{63,64,65,66,67,68,60,61,62,78,79,80},
	{75,76,77,78,79,80,54,55,56,63,64,65},{72,73,74,78,79,80,57,58,59,66,67,68},{72,73,74,75,76,77,60,61,62,69,70,71}
	},{
	{27,36,45,54,63,72, 1,10,19, 2,11,20},{ 0, 9,18,54,63,72,28,37,46,29,38,47},{ 0, 9,18,27,36,45,55,64,73,56,65,74},
	{28,37,46,55,64,73, 0, 9,18, 2,11,20},{ 1,10,19,55,64,73,27,36,45,29,38,47},{ 1,10,19,28,37,46,54,63,72,56,65,74},
	{29,38,47,56,65,74, 0, 9,18, 1,10,19},{ 2,11,20,56,65,74,27,36,45,28,37,46},{ 2,11,20,29,38,47,54,63,72,55,64,73}
	},{
	{30,39,48,57,66,75, 4,13,22, 5,14,23},{ 3,12,21,57,66,75,31,40,49,32,41,50},{ 3,12,21,30,39,48,58,67,76,59,68,77},
	{31,40,49,58,67,76, 3,12,21, 5,14,23},{ 4,13,22,58,67,76,30,39,48,32,41,50},{ 4,13,22,31,40,49,57,66,75,59,68,77},
	{32,41,50,59,68,77, 3,12,21, 4,13,22},{ 5,14,23,59,68,77,30,39,48,31,40,49},{ 5,14,23,32,41,50,57,66,75,58,67,76}
	},{
	{33,42,51,60,69,78, 7,16,25, 8,17,26},{ 6,15,24,60,69,78,34,43,52,35,44,53},{ 6,15,24,33,42,51,61,70,79,62,71,80},
	{34,43,52,61,70,79, 6,15,24, 8,17,26},{ 7,16,25,61,70,79,33,42,51,35,44,53},{ 7,16,25,34,43,52,60,69,78,62,71,80},
	{35,44,53,62,71,80, 6,15,24, 7,16,25},{ 8,17,26,62,71,80,33,42,51,34,43,52},{ 8,17,26,35,44,53,60,69,78,61,70,79}
	}
};

//6 segments * 9 triplets * 3 cells in triplet
static const int tripletCells[6][9][3] =
{
	{{ 0, 1, 2},{ 3, 4, 5},{ 6, 7, 8},{ 9,10,11},{12,13,14},{15,16,17},{18,19,20},{21,22,23},{24,25,26}},
	{{27,28,29},{30,31,32},{33,34,35},{36,37,38},{39,40,41},{42,43,44},{45,46,47},{48,49,50},{51,52,53}},
	{{54,55,56},{57,58,59},{60,61,62},{63,64,65},{66,67,68},{69,70,71},{72,73,74},{75,76,77},{78,79,80}},
	{{ 0, 9,18},{27,36,45},{54,63,72},{ 1,10,19},{28,37,46},{55,64,73},{ 2,11,20},{29,38,47},{56,65,74}},
	{{ 3,12,21},{30,39,48},{57,66,75},{ 4,13,22},{31,40,49},{58,67,76},{ 5,14,23},{32,41,50},{59,68,77}},
	{{ 6,15,24},{33,42,51},{60,69,78},{ 7,16,25},{34,43,52},{61,70,79},{ 8,17,26},{35,44,53},{62,71,80}}
};

//game mode flags
#define MODE_SOLVING			0	//unused, keep solving
#define MODE_NO_OUTPUT			1	//only check for solution, do not write the result
#define MODE_ANY_SOLUTION		2	//stop on first solution
#define MODE_STOP_PROCESSING	4	//solved or errored
#define MODE_STOP_GUESSING		8	//necessary solutions found

//The whole game context, cloned before guessing, and copied back after successful guess.
struct game
{
	bitmap groupKnownDigits[27];	//initial 0
	bitmap cellPossibilities[81];	//0==known, initial 511=0x01FF
	int mode;						//combination of the game mode flags, initial 0
	int cellsLeft;					//initial 81
	int nSolutions;
	int lastGuess;					//initial somwhere in the middle
	cellDigit *cellDigits;			//pointer to a common array of the digits found, uninitialized
	game *masterGame;				//sometimes we are jumping directly to g->masterGame->done
};

//A template used for game structure initialization.
static const game defaultGame =
{
	{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}, //groupKnownDigits
	{ //cellPossibilities
		511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,
		511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,
		511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511,511
	},
	MODE_SOLVING,
	81, //cellsLeft
	0, //nSolutions
	40, //lastGuess
	0, //&cellDigits
	0, //&masterGame
};

extern int z0, z1;

//Set a known digit at a known cell.
//Update (clear) possibilities at affected cells and call recursively
//when a single possibility is found.
//Update some other context
static void setDigit(game* g, const cellIndex pi, const bitmap pbm)
{
	bitmap *gkd = g->groupKnownDigits;
	bitmap *cp = g->cellPossibilities;
	const int *ag;
	const cellIndex *ac;

	cellIndex qi[82];
	bitmap qbm[82];
	int qTop = 0;
	int n = 0;
	//qi[0] = pi;	qbm[0] = pbm;
	int i = pi;
	int bm = pbm;
	bitmap *bbm;
	//setting the same digit for second time may corrupt the rest of the data
	//if(cp[pi] == 0) { //puzzle is overdetermined
	//	return;
	//}

//process_queue:
	do {
		if(cp[i] == 0) continue;
		ag = affectedGroups[i];
		ac = affectedCells[i];

		//if the digit we are setting has been previously set for one of
		//the 3 groups the cell is member of, we are in a wrong way
		if(bm & (gkd[ag[0]] | gkd[ag[1]] | gkd[ag[2]])) {
			g->mode |= MODE_STOP_PROCESSING;
			return;
		}
		cp[i] = 0; //clear the mask to exclude this cell from further processing

		//if we want the final solution, store the digit for printing
		if(0 == (g->mode & MODE_NO_OUTPUT))
			g->cellDigits[i] = (cellDigit)bm; //set the digit. Bit 8 is lost, translating 9 to 0.

		if(0 == --g->cellsLeft) {//solved
			g->nSolutions++;
			if((g->mode & MODE_ANY_SOLUTION) || (g->nSolutions > 1)) {//no more guesses, everything is done
				g->masterGame->nSolutions = g->nSolutions;
				g->mode |= (MODE_STOP_PROCESSING | MODE_STOP_GUESSING);
				return;
			}
			g->mode |= MODE_STOP_PROCESSING;
			return;
		}

		//set the digit as solved for 3 groups
		gkd[ag[0]] |= bm; gkd[ag[1]] |= bm; gkd[ag[2]] |= bm;

		//clear the known digit from the possibilities of all related cells
#ifdef MANUAL_UNROLL
#define clp( ci ) {\
		bbm = &cp[ac[ci]];\
		if(*bbm & bm) {\
			*bbm ^= bm; \
			if(*bbm == 0) {g->mode |= MODE_STOP_PROCESSING;	return;}\
			if(Bitmap2Digit[*bbm]) {qi[qTop] = ac[ci], qbm[qTop++] = *bbm;}\
		}}

		clp(0);clp(1);clp(2);clp(3);clp(4);clp(5);clp(6);clp(7);clp(8);clp(9);
		clp(10);clp(11);clp(12);clp(13);clp(14);clp(15);clp(16);clp(17);clp(18);clp(19);
#else //MANUAL_UNROLL
#ifdef __INTEL_COMPILER
#pragma unroll(20)
#endif //__INTEL_COMPILER
		for(int ci = 0; ci < 20; ci++) {
			bbm = &cp[ac[ci]];
			if(*bbm & bm) { //skip already marked/solved cells
				*bbm ^= bm; //clear the appropriate possibility
				if(*bbm == 0) {
					g->mode |= MODE_STOP_PROCESSING;
					return;
				}
				if(Bitmap2Digit[*bbm]) { //single digit?
					qi[qTop] = ac[ci], qbm[qTop++] = *bbm;
				}
			}
		}
#endif //MANUAL_UNROLL
	} while (i = qi[n],	bm = qbm[n++], n <= qTop);
/*
	return;

do_at_once:
	//First pass: set the digits w/o updating the affected cells.
	for(; n < qTop; n++) {
		i = qi[n];
		bm = qbm[n];
		if(cp[i] == 0) continue;
		ag = affectedGroups[i];
		ac = affectedCells[i];
		//if the digit we are setting has been previously set for one of
		//the 3 groups the cell is member of, we are in wrong way
		if(bm & (gkd[ag[0]] | gkd[ag[1]] | gkd[ag[2]])) {
			g->mode |= MODE_STOP_PROCESSING;
			return;
		}
		cp[i] = 0; //clear the mask to exclude this cell from further processing

		//if we need the final solution, store the digit for printing
		if(0 == (g->mode & MODE_NO_OUTPUT))
			g->cellDigits[i] = (cellDigit)bm; //set the digit. Bit 8 is lost, translating 9 to 0.

		if(0 == --g->cellsLeft) {//solved
			g->nSolutions++;
			if((g->mode & MODE_ANY_SOLUTION) || (g->nSolutions > 1)) {//no more guesses, everything is done
				g->masterGame->nSolutions = g->nSolutions;
				g->mode |= (MODE_STOP_PROCESSING | MODE_STOP_GUESSING);
				return;
			}
			g->mode |= MODE_STOP_PROCESSING;
			return;
		}

		//set the digit as solved for all 3 groups
		gkd[ag[0]] |= bm; gkd[ag[1]] |= bm; gkd[ag[2]] |= bm;
	}
	//Second pass: update the affected cells.
	//Overall performance improvement is about 9% compared to setDigit calls.
	//checkBoard(g, gkd, cp);
	for(int j = 0; j < 81; j++) {
		bbm = &cp[j];
		ag = affectedGroups[j];
		bitmap knowns = gkd[ag[0]] | gkd[ag[1]] | gkd[ag[2]];
		if(*bbm & knowns) { //there are possibilities to be cleared
			*bbm &= ~knowns;
			if(*bbm == 0) { //no possibilities for this cell
				g->mode |= MODE_STOP_PROCESSING;
				return;
			}
			if(Bitmap2Digit[*bbm]) { //single digit?
				qi[qTop] = i, qbm[qTop++] = *bbm;
			}
		}
	}
	goto process_queue; //if new digits found process them
	*/
}

//Loop trough the groups, find the only cell containing particular digit in the group if any,
//then set the digit found.
//Since setting the digit changes the context, repeat until nothing found.
//On exit, the slower algorithms are expected to be run (i.e. guessing). 
static void checkForLastOccurenceInGroup(game* g)
{
	//checks for group having some posible digit in only one cell
	int gi, duplicates, ci, cellPoss, groupPoss, uniques, newPoss;
	bitmap *cp = g->cellPossibilities;
	bitmap *gkd = g->groupKnownDigits;
	const cellIndex *gc;
restart:
	for (gi = 0; gi < 27; gi++) {
		//if(g->groupKnownDigits[gi] == 511) continue;
		groupPoss = 0;
		duplicates = 0;
		gc = cellsInGroup[gi];
		for (ci = 0; ci < 9; ci++) {
			cellPoss = cp[gc[ci]];
			duplicates |= (groupPoss & cellPoss); //this tricky code is taken from bb_sudoku by Brian Turner
			groupPoss |= cellPoss;
		}
		if((groupPoss ^ gkd[gi]) != 511) { //no place for some of the unknown digits
			g->mode |= MODE_STOP_PROCESSING;
			return;
		}
		uniques = groupPoss ^ duplicates;
		if(uniques == 0) continue;
		//clear the unique possibilities from the group and process the unique cells
		for (ci = 0; ci < 9; ci++) {
			newPoss = cp[gc[ci]] & uniques;
			if(newPoss == 0) continue;
			//one of the cells of interest found
			if (Bitmap2Digit[newPoss] == 0) { //error: the same cell introduced > 1 unique digits in the group
				g->mode |= MODE_STOP_PROCESSING;
				return;
				}
			setDigit(g, gc[ci], newPoss);
			if(g->mode & MODE_STOP_PROCESSING) return;
			//the benefit from the following optimization is questionable
			uniques ^= newPoss; //clear the already processed bit
			if (uniques == 0) goto restart; //no more bits
		}
		//goto restart;
	} //group loop
}

//Based on bb_sudoku by Brian Turner.
static void FindLockedCandidates (game* g)
{
	int i, j, k, b, ci;
	bitmap *cp;
	bitmap *gcp = g->cellPossibilities;
	bitmap tripletPossibilities[9];
restart:
	//int partialFound = 0; //some bits cleared but no digit found
	for (i = 0; i < 6; i++) {
		for(j = 0; j < 9; j++)
			tripletPossibilities[j] = gcp[tripletCells[i][j][0]] | gcp[tripletCells[i][j][1]] | gcp[tripletCells[i][j][2]];
		for (j = 0; j < 9; j++) {
			b = (//found in the current triplet, and found in exactly one of the affected triplet pairs
				((tripletPossibilities[affectedTriplets[j][0]] | tripletPossibilities[affectedTriplets[j][1]]) ^ //row or column pair
				 (tripletPossibilities[affectedTriplets[j][2]] | tripletPossibilities[affectedTriplets[j][3]]))  //square pair
				& tripletPossibilities[j]);
			if (b == 0) continue;
			//don't care which bit where came from
			for (k = 0; k < 12; k++) { //6 from the row/col + 6 from the square
				ci = tripletAffectedCells[i][j][k];
				cp = &gcp[ci];
				if (*cp & b) { //there is something to clear
					*cp = (bitmap)(*cp & ~b);
					if (*cp == 0) { //no any possibility for this cell
						g->mode |= MODE_STOP_PROCESSING;
						return;
					}
					if (Bitmap2Digit[*cp]) { //single possibility remained?
						setDigit(g, ci, *cp);
						if(g->mode & MODE_STOP_PROCESSING) return;
						checkForLastOccurenceInGroup(g);
						if(g->mode & MODE_STOP_PROCESSING) return;
						goto restart;
					}
				}
			}
			//clearing some bits is not enough to repeat the whole checkings
			//partialFound = 1;
			//goto restart;
			//return;
		} //for j
	} //for i
	//if(partialFound) { //slower
	//	int oldCellsLeft = g->cellsLeft;
	//	checkForLastOccurenceInGroup(g);
	//	if(g->mode & MODE_STOP_PROCESSING) return;
	//	if(oldCellsLeft != g->cellsLeft) { //digit found, restart the loop
	//		goto restart;
	//	}
	//}
}

int fsss_num_guesses = 0;

//Perform any solving algorithms.
//Finally make a guess and call recursively until success (solved) or failure (no solution).
static void attempt(game* g)
{
	game gg;
	cellIndex chosenCell;
	int ci, nGuesses, chosenValue, cp, bc;
restart:
	checkForLastOccurenceInGroup(g); //the first algorithm performs internal repeating
	if(g->mode & MODE_STOP_PROCESSING) return;
	FindLockedCandidates(g); //bb_sudoku by Brian Turner
	if(g->mode & MODE_STOP_PROCESSING) return;

	//Prepare a guess

	//Findout an unsolved cell with less possibilities
	nGuesses = 10;
	//Randomizing the guess sequence causes 9.3% false improvement.
	ci = g->lastGuess;
	do {
		ci++;
		if(ci > 80)
			ci = 0;
		cp = g->cellPossibilities[ci];
		if(0 == cp) continue; //skip solved cells
		if((bc = BitCount[cp]) < nGuesses) {
			chosenCell = ci;
			if(2 == (nGuesses = bc)) //guessing a cell with 2 possibilities is OK
				break;
		}
	} while(ci != g->lastGuess);
	g->lastGuess = chosenCell;

	int restValues = g->cellPossibilities[chosenCell];
	for(chosenValue = restValues & -restValues; --nGuesses; chosenValue = restValues & -restValues) {
		restValues ^= chosenValue; //rest of the possibilities
		gg = *g; //copy the solution context
		fsss_num_guesses++;
		setDigit(&gg, chosenCell, (bitmap)chosenValue);
		if(0 == (gg.mode & MODE_STOP_PROCESSING)) {
			attempt(&gg);
		}
		if(gg.mode & MODE_STOP_GUESSING) {
			g->mode = gg.mode;
			g->nSolutions = gg.nSolutions;
			return;
		}
		g->nSolutions = gg.nSolutions;
		if(nGuesses == 1) { //stop working with copies of the context
			setDigit(g, chosenCell, restValues);
			if(g->mode & MODE_STOP_PROCESSING) return;
			goto restart;
		}
		//Note that the next guess will destroy the content of g->cellDigits.
	}
}

//Convert the solution to ASCII and write to the buffer.
static void writeBack(const game* g, char* out)
{
	for(int i = 0; i < 81; i++)
		out[i] = (char)(Bitmap2Digit[(unsigned char)g->cellDigits[i]] + '0');
}

//Set the initially known digits.
//The context is updated on 2 passes.
static inline void init(game* g, const char* in)
{
	bitmap *gkd = g->groupKnownDigits;
	bitmap *cp = g->cellPossibilities;
	//First pass: set the digits w/o updating the affected cells.
	for(int i = 0; i < 81; i++) {
		if(in[i] == 0) continue;
		//...additional input checking here...
		//setDigit(g, i, Digit2Bitmap[in[i]]);
		//...check for error here if this is the goal...

		bitmap bm = Digit2Bitmap[in[i]];

		const int *ag = affectedGroups[i];
		//if the digit we are setting has been previously set for one of
		//the 3 groups the cell is member of, we are in wrong way
		if(bm & (gkd[ag[0]] | gkd[ag[1]] | gkd[ag[2]])) {
			g->mode |= MODE_STOP_PROCESSING;
			return;
		}
		cp[i] = 0; //clear the mask to exclude this cell from further processing

		//if we need the final solution, store the digit for printing
		if(0 == (g->mode & MODE_NO_OUTPUT))
			g->cellDigits[i] = (cellDigit)bm; //set the digit. Bit 8 is lost, translating 9 to 0.

		if(0 == --g->cellsLeft) {//lucky, 81 givens
			g->nSolutions = 1;
			g->mode |= MODE_STOP_PROCESSING;
			return;
		}

		//set the digit as solved for all 3 groups
		gkd[ag[0]] |= bm; gkd[ag[1]] |= bm; gkd[ag[2]] |= bm;
	}
	//Second pass: update the affected cells.
	//Overall performance improvement is about 9% compared to setDigit calls.
	//checkBoard(g, gkd, cp);
	for(int i = 0; i < 81; i++) {
		bitmap *bm = &cp[i];
		const int *ag = affectedGroups[i];
		bitmap knowns = gkd[ag[0]] | gkd[ag[1]] | gkd[ag[2]];
		if(*bm & knowns) { //there are possibilities to be cleared
			*bm &= ~knowns;
			if(*bm == 0) { //no possibilities for this cell
				g->mode |= MODE_STOP_PROCESSING;
				return;
			}
			if(Bitmap2Digit[*bm]) { //single digit?
				setDigit(g, i, *bm);
				if(g->mode & MODE_STOP_PROCESSING) return;
			}
		}
	}
	g->lastGuess = 81 - g->cellsLeft; //randomize guessing positions
}

//Attempt solving the game
//Parameters:
// in - the input buffer, char[81] with values from 0x00 to 0x09
// out - the output buffer, char[81] with values from '1' to '9'. May be NULL.
// mode - 0=find first solution and write to the buffer. Return 0 or 1.
//        1=search for second solution and do not write. Return 0, 1, or 2.
//Return values:
// -1 - invalid parameters.
//  0 - no solution.
//  1 - some solution found in mode=0 or there is only one solution in mode 1.
//  2 - second solution found. No write back.
//The in and out buffers may overlap or be the same buffer.
extern "C"
int fsss_solve(const char* in, char* out, std::size_t *num_guesses, const int mode=0)
{
	if(0 == in) return -1; //wrong parameters
	game g = defaultGame; //start from a copy of the empty game
	switch(mode) {
		case 0: //find first solution and write it back if the out buffer is not NULL
			g.mode |= MODE_ANY_SOLUTION;
			if(!out) g.mode |= MODE_NO_OUTPUT; //do not copy back the solution
			break;
		case 1: //search for second solution; do not write back the result
			g.mode |= MODE_NO_OUTPUT; //do not copy back the solution
			break;
		default:
			return -1; //wrong parameters
	}
	cellDigit cellDigits[81]; //0==9, uninitialized
	g.cellDigits = cellDigits;
	g.masterGame = &g;
	init(&g, in); //set all known digits
    fsss_num_guesses = 0;
	if(0 == (g.mode & MODE_STOP_PROCESSING)) {
		attempt(&g); //do the job
	}
    *num_guesses = fsss_num_guesses;
	if (g.nSolutions) {
		if(0 == (g.mode & MODE_NO_OUTPUT))
			writeBack(&g, out);
		if(g.nSolutions > 1)
			return 2; //2 or more solutions
		return 1; //success or exactly one solution
	}
	return 0; //no solution
}

