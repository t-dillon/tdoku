/* JSolve.h - A very fast Sudoku solver

Version 1.2 of January 22, 2010

Copyright (c) 2009-2010, Jason T. Linhart
All rights reserved.
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
	¥	Redistributions of source code must retain the above copyright notice,
		this list of conditions and the following disclaimer.
	¥	Redistributions in binary form must reproduce the above copyright notice,
		this list of conditions and the following disclaimer in the documentation
		and/or other materials provided with the distribution.
	¥	Neither the name Jason T. Linhart nor the names of other contributors
		may be used to endorse or promote products derived from this software
		without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

*/

#ifdef __cplusplus
extern "C" {
#endif

/*
	JSolve - a very fast Sudoku solver.

	clues - a null terminated 81 character string with '1' through '9' for
		digits and either '.' or '0' for empty cells
	result - If you want the soltuion, pass in a pointer to 82 characters
		of storage where you want the solution written. Pass in 0 to
		ignore the solution.
	max_solutions - the largest number of solutions to be searched for
		Typically 2 to check for soluble, not soluble, or multipule solutions.
		Pass in a very large number to count all solutions.
	<returns> - the number of solutions found
*/


int JSolve(const char *clues,char *result,int max_solutions);

#ifdef __cplusplus
	}
#endif

/* END OF JSolve.h - A very fast Sudoku solver */
