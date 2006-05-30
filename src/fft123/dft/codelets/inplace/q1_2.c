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
/* Generated on Sat Jul  5 21:39:14 EDT 2003 */

#include "codelet-dft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_twidsq -compact -variables 4 -reload-twiddle -dif -n 2 -name q1_2 -include q.h */

/*
 * This function contains 12 FP additions, 8 FP multiplications,
 * (or, 8 additions, 4 multiplications, 4 fused multiply/add),
 * 17 stack variables, and 16 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: q1_2.c,v 1.1 2005/07/26 17:36:52 glass Exp $
 * $Id: q1_2.c,v 1.1 2005/07/26 17:36:52 glass Exp $
 * $Id: q1_2.c,v 1.1 2005/07/26 17:36:52 glass Exp $
 */

#include "q.h"

static const R *q1_2(R *rio, R *iio, const R *W, stride is, stride vs, int m, int dist)
{
     int i;
     for (i = m; i > 0; i = i - 1, rio = rio + dist, iio = iio + dist, W = W + 2) {
	  E T1, T2, T4, T6, T7, T8, T9, Ta, Tc, Te, Tf, Tg;
	  T1 = rio[0];
	  T2 = rio[WS(is, 1)];
	  T4 = T1 - T2;
	  T6 = iio[0];
	  T7 = iio[WS(is, 1)];
	  T8 = T6 - T7;
	  T9 = rio[WS(vs, 1)];
	  Ta = rio[WS(vs, 1) + WS(is, 1)];
	  Tc = T9 - Ta;
	  Te = iio[WS(vs, 1)];
	  Tf = iio[WS(vs, 1) + WS(is, 1)];
	  Tg = Te - Tf;
	  rio[0] = T1 + T2;
	  iio[0] = T6 + T7;
	  rio[WS(is, 1)] = T9 + Ta;
	  iio[WS(is, 1)] = Te + Tf;
	  {
	       E Tb, Td, T3, T5;
	       Tb = W[0];
	       Td = W[1];
	       rio[WS(vs, 1) + WS(is, 1)] = FMA(Tb, Tc, Td * Tg);
	       iio[WS(vs, 1) + WS(is, 1)] = FNMS(Td, Tc, Tb * Tg);
	       T3 = W[0];
	       T5 = W[1];
	       rio[WS(vs, 1)] = FMA(T3, T4, T5 * T8);
	       iio[WS(vs, 1)] = FNMS(T5, T4, T3 * T8);
	  }
     }
     return W;
}

static const tw_instr twinstr[] = {
     {TW_FULL, 0, 2},
     {TW_NEXT, 1, 0}
};

static const ct_desc desc = { 2, "q1_2", twinstr, {8, 4, 4, 0}, &GENUS, 0, 0, 0 };

void X(codelet_q1_2) (planner *p) {
     X(kdft_difsq_register) (p, q1_2, &desc);
}
