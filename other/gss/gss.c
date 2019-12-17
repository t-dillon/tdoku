/*
    Generic Sudoku Solver (gss) link to tdoku benchmark
    Copyright (C) 2019 B. Pieters

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
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

size_t OtherSolverGss(const char *input, size_t limit /* unused */,
                         uint32_t configuration /* unused */,
                         char *solution, size_t *num_guesses) 
{
	int limitlevel;
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
		
		
	limitlevel=(int)configuration;
	if (!S_ReadCompactStandard(input, &S))
		return 0;
		
	sol=malloc(sizeof(MASKINT *));
	state=Solve(&S, &maxlevel, &sol, &ns, 2, limitlevel, STRAT);
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
			return 0;
	}
}

