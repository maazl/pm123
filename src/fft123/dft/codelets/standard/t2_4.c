/*
 * Copyright (c) 2003 Matteo Frigo
 * Copyright (c) 2003 Massachusetts Institute of Technology
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

/* This file was automatically generated --- DO NOT EDIT */
/* Generated on Sat Jul  5 21:30:08 EDT 2003 */

#include "codelet-dft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_twiddle -compact -variables 4 -twiddle-log3 -n 4 -name t2_4 -include t.h */

/*
 * This function contains 24 FP additions, 16 FP multiplications,
 * (or, 16 additions, 8 multiplications, 8 fused multiply/add),
 * 21 stack variables, and 16 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: t2_4.c,v 1.1 2005/07/26 17:36:55 glass Exp $
 * $Id: t2_4.c,v 1.1 2005/07/26 17:36:55 glass Exp $
 * $Id: t2_4.c,v 1.1 2005/07/26 17:36:55 glass Exp $
 */

#include "t.h"

static const R *t2_4(R *ri, R *ii, const R *W, stride ios, int m, int dist)
{
     int i;
     for (i = m; i > 0; i = i - 1, ri = ri + dist, ii = ii + dist, W = W + 4) {
	  E T1, Tp, Ta, Te, To, Tl, Tk, Th;
	  T1 = ri[0];
	  Tp = ii[0];
	  {
	       E T7, T9, Tc, Td, Tg, Tf, T2, T4, T3, T5, T6, T8;
	       T7 = ri[WS(ios, 2)];
	       T9 = ii[WS(ios, 2)];
	       Tc = ri[WS(ios, 1)];
	       Td = ii[WS(ios, 1)];
	       Tg = ii[WS(ios, 3)];
	       Tf = ri[WS(ios, 3)];
	       T2 = W[2];
	       T4 = W[3];
	       T3 = W[0];
	       T5 = W[1];
	       T6 = FMA(T2, T3, T4 * T5);
	       T8 = FNMS(T4, T3, T2 * T5);
	       Ta = FNMS(T8, T9, T6 * T7);
	       Te = FMA(T3, Tc, T5 * Td);
	       To = FMA(T8, T7, T6 * T9);
	       Tl = FNMS(T4, Tf, T2 * Tg);
	       Tk = FNMS(T5, Tc, T3 * Td);
	       Th = FMA(T2, Tf, T4 * Tg);
	  }
	  {
	       E Tb, Ti, Tn, Tq;
	       Tb = T1 + Ta;
	       Ti = Te + Th;
	       ri[WS(ios, 2)] = Tb - Ti;
	       ri[0] = Tb + Ti;
	       Tn = Tk + Tl;
	       Tq = To + Tp;
	       ii[0] = Tn + Tq;
	       ii[WS(ios, 2)] = Tq - Tn;
	  }
	  {
	       E Tj, Tm, Tr, Ts;
	       Tj = T1 - Ta;
	       Tm = Tk - Tl;
	       ri[WS(ios, 3)] = Tj - Tm;
	       ri[WS(ios, 1)] = Tj + Tm;
	       Tr = Tp - To;
	       Ts = Te - Th;
	       ii[WS(ios, 1)] = Tr - Ts;
	       ii[WS(ios, 3)] = Ts + Tr;
	  }
     }
     return W;
}

static const tw_instr twinstr[] = {
     {TW_COS, 0, 1},
     {TW_SIN, 0, 1},
     {TW_COS, 0, 3},
     {TW_SIN, 0, 3},
     {TW_NEXT, 1, 0}
};

static const ct_desc desc = { 4, "t2_4", twinstr, {16, 8, 8, 0}, &GENUS, 0, 0, 0 };

void X(codelet_t2_4) (planner *p) {
     X(kdft_dit_register) (p, t2_4, &desc);
}
