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
/* Generated on Sat Jul  5 22:11:11 EDT 2003 */

#include "codelet_rdft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_hc2r -compact -variables 4 -sign 1 -n 5 -name hc2r_5 -include hc2r.h */

/*
 * This function contains 12 FP additions, 7 FP multiplications,
 * (or, 8 additions, 3 multiplications, 4 fused multiply/add),
 * 18 stack variables, and 10 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: hc2r_5.c,v 1.2 2006/06/14 11:55:17 glass Exp $
 * $Id: hc2r_5.c,v 1.2 2006/06/14 11:55:17 glass Exp $
 * $Id: hc2r_5.c,v 1.2 2006/06/14 11:55:17 glass Exp $
 */

#include "hc2r.h"

static void hc2r_5(const R *ri, const R *ii, R *O, stride ris, stride iis, stride os, int v, int ivs, int ovs)
{
     DK(KP2_000000000, +2.000000000000000000000000000000000000000000000);
     DK(KP1_118033988, +1.118033988749894848204586834365638117720309180);
     DK(KP500000000, +0.500000000000000000000000000000000000000000000);
     DK(KP1_902113032, +1.902113032590307144232878666758764286811397268);
     DK(KP1_175570504, +1.175570504584946258337411909278145537195304875);
     int i;
     for (i = v; i > 0; i = i - 1, ri = ri + ivs, ii = ii + ivs, O = O + ovs) {
	  E Ta, Tc, T1, T4, T5, T6, Tb, T7;
	  {
	       E T8, T9, T2, T3;
	       T8 = ii[WS(iis, 1)];
	       T9 = ii[WS(iis, 2)];
	       Ta = FNMS(KP1_902113032, T9, KP1_175570504 * T8);
	       Tc = FMA(KP1_902113032, T8, KP1_175570504 * T9);
	       T1 = ri[0];
	       T2 = ri[WS(ris, 1)];
	       T3 = ri[WS(ris, 2)];
	       T4 = T2 + T3;
	       T5 = FNMS(KP500000000, T4, T1);
	       T6 = KP1_118033988 * (T2 - T3);
	  }
	  O[0] = FMA(KP2_000000000, T4, T1);
	  Tb = T6 + T5;
	  O[WS(os, 1)] = Tb - Tc;
	  O[WS(os, 4)] = Tb + Tc;
	  T7 = T5 - T6;
	  O[WS(os, 2)] = T7 - Ta;
	  O[WS(os, 3)] = T7 + Ta;
     }
}

static const khc2r_desc desc = { 5, "hc2r_5", {8, 3, 4, 0}, &GENUS, 0, 0, 0, 0, 0 };

void X(codelet_hc2r_5) (planner *p) {
     X(khc2r_register) (p, hc2r_5, &desc);
}
