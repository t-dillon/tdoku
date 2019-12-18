/*
    Generic Sudoku Solver (gss) link to tdoku benchmark
    Copyright (C) 2019 B. Pieters
	BSD 2-Clause License

	Copyright (c) 2019, Tom Dillon
	All rights reserved.

	Redistribution and use in source and binary forms, with or without
	modification, are permitted provided that the following conditions are met:

	1. Redistributions of source code must retain the above copyright notice, this
	   list of conditions and the following disclaimer.

	2. Redistributions in binary form must reproduce the above copyright notice,
	   this list of conditions and the following disclaimer in the documentation
	   and/or other materials provided with the distribution.

	THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
	AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
	DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
	FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
	DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
	SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
	CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
	OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
	OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "sudoku.h"
#include "sudoku_parser.h"
static int init=0;
static Sudoku S;
static int STRAT=0;
int gss_verbose=0;
int S_ReadCompactStandard(const char *s, Sudoku *S)
{
	int i, k;
	char c;
	k=0;		
	while ((*s)&&(k<81))
	{
		c=*s;
		s++;
		if ((c=='0')||(c=='.'))
		{
			S->M[k]=0;
			for (i=0;i<S->BS;i++)
				S->M[k]|=VX[i];
			k++;
		}
		else if (isdigit(c))
		{
			S->M[k]=VX[(int)(c-'1')];
			k++;
		}	
	}
	if (k!=81)
		return 0;
	if (Check(S))
		return 0;
	return 1;
}

size_t OtherSolverGss(const char *input, size_t limit, uint32_t configuration, char *solution, size_t *num_guesses) 
{
	int maxlevel;
	int ns;
	int i;
	MASKINT **sol;
	SSTATE state;
	if (!init)
	{
		S_init();
		S=S_InitStdSudoku(1);
		/* load all logic */
		STRAT|=(1<<MASK);
		STRAT|=(1<<MASKHIDDEN);
		STRAT|=(1<<MASKINTER);
		STRAT|=(1<<BRUTE);			
		init=1;
	}
		
	if (!S_ReadCompactStandard(input, &S))
		return 0;
		
	sol=malloc(sizeof(MASKINT *));
	state=Solve(&S, &maxlevel, &sol, &ns, (int)limit, (int)configuration, STRAT);
	solution[81]='\0';
	switch(state)
	{
		case SOLVED:
			for (i=0;i<81;i++)
				solution[i]='0'+(char)EL_V(S.M[i], S.BS);
			free(sol);		
			return 1;
		case MULTISOL:
			for (i=0;i<81;i++)
				solution[i]='0'+(char)EL_V(sol[0][i], S.BS);
			for (i=0;i<ns;i++)
				free(sol[i]);
			free(sol);		
			return (size_t)ns;
		case INVALID:
		case UNSOLVED:	
		default:
			free(sol);
			return 0;
	}
}

