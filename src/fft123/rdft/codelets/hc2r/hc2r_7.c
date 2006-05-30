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

#include "codelet-rdft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_hc2r -compact -variables 4 -sign 1 -n 7 -name hc2r_7 -include hc2r.h */

/*
 * This function contains 24 FP additions, 19 FP multiplications,
 * (or, 11 additions, 6 multiplications, 13 fused multiply/add),
 * 21 stack variables, and 14 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: hc2r_7.c,v 1.1 2005/07/26 17:37:06 glass Exp $
 * $Id: hc2r_7.c,v 1.1 2005/07/26 17:37:06 glass Exp $
 * $Id: hc2r_7.c,v 1.1 2005/07/26 17:37:06 glass Exp $
 */

#include "hc2r.h"

static void hc2r_7(const R *ri, const R *ii, R *O, stride ris, stride iis, stride os, int v, int ivs, int ovs)
{
     DK(KP2_000000000, +2.000000000000000000000000000000000000000000000);
     DK(KP1_801937735, +1.801937735804838252472204639014890102331838324);
     DK(KP445041867, +0.445041867912628808577805128993589518932711138);
     DK(KP1_246979603, +1.246979603717467061050009768008479621264549462);
     DK(KP867767478, +0.867767478235116240951536665696717509219981456);
     DK(KP1_949855824, +1.949855824363647214036263365987862434465571601);
     DK(KP1_563662964, +1.563662964936059617416889053348115500464669037);
     int i;
     for (i = v; i > 0; i = i - 1, ri = ri + ivs, ii = ii + ivs, O = O + ovs) {
	  E T9, Td, Tb, T1, T4, T2, T3, T5, Tc, Ta, T6, T8, T7;
	  T6 = ii[WS(iis, 2)];
	  T8 = ii[WS(iis, 1)];
	  T7 = ii[WS(iis, 3)];
	  T9 = FNMS(KP1_949855824, T7, KP1_563662964 * T6) - (KP867767478 * T8);
	  Td = FMA(KP867767478, T6, KP1_563662964 * T7) - (KP1_949855824 * T8);
	  Tb = FMA(KP1_563662964, T8, KP1_949855824 * T6) + (KP867767478 * T7);
	  T1 = ri[0];
	  T4 = ri[WS(ris, 3)];
	  T2 = ri[WS(ris, 1)];
	  T3 = ri[WS(ris, 2)];
	  T5 = FMA(KP1_246979603, T3, T1) + FNMA(KP445041867, T4, KP1_801937735 * T2);
	  Tc = FMA(KP1_246979603, T4, T1) + FNMA(KP1_801937735, T3, KP445041867 * T2);
	  Ta = FMA(KP1_246979603, T2, T1) + FNMA(KP1_801937735, T4, KP445041867 * T3);
	  O[WS(os, 4)] = T5 - T9;
	  O[WS(os, 3)] = T5 + T9;
	  O[WS(os, 2)] = Tc + Td;
	  O[WS(os, 5)] = Tc - Td;
	  O[WS(os, 6)] = Ta + Tb;
	  O[WS(os, 1)] = Ta - Tb;
	  O[0] = FMA(KP2_000000000, T2 + T3 + T4, T1);
     }
}

static const khc2r_desc desc = { 7, "hc2r_7", {11, 6, 13, 0}, &GENUS, 0, 0, 0, 0, 0 };

void X(codelet_hc2r_7) (planner *p) {
     X(khc2r_register) (p, hc2r_7, &desc);
}
