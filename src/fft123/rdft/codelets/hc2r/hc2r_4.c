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

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_hc2r -compact -variables 4 -sign 1 -n 4 -name hc2r_4 -include hc2r.h */

/*
 * This function contains 6 FP additions, 2 FP multiplications,
 * (or, 6 additions, 2 multiplications, 0 fused multiply/add),
 * 10 stack variables, and 8 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: hc2r_4.c,v 1.1 2005/07/26 17:37:06 glass Exp $
 * $Id: hc2r_4.c,v 1.1 2005/07/26 17:37:06 glass Exp $
 * $Id: hc2r_4.c,v 1.1 2005/07/26 17:37:06 glass Exp $
 */

#include "hc2r.h"

static void hc2r_4(const R *ri, const R *ii, R *O, stride ris, stride iis, stride os, int v, int ivs, int ovs)
{
     DK(KP2_000000000, +2.000000000000000000000000000000000000000000000);
     int i;
     for (i = v; i > 0; i = i - 1, ri = ri + ivs, ii = ii + ivs, O = O + ovs) {
	  E T5, T8, T3, T6;
	  {
	       E T4, T7, T1, T2;
	       T4 = ri[WS(ris, 1)];
	       T5 = KP2_000000000 * T4;
	       T7 = ii[WS(iis, 1)];
	       T8 = KP2_000000000 * T7;
	       T1 = ri[0];
	       T2 = ri[WS(ris, 2)];
	       T3 = T1 + T2;
	       T6 = T1 - T2;
	  }
	  O[WS(os, 2)] = T3 - T5;
	  O[WS(os, 3)] = T6 + T8;
	  O[0] = T3 + T5;
	  O[WS(os, 1)] = T6 - T8;
     }
}

static const khc2r_desc desc = { 4, "hc2r_4", {6, 2, 0, 0}, &GENUS, 0, 0, 0, 0, 0 };

void X(codelet_hc2r_4) (planner *p) {
     X(khc2r_register) (p, hc2r_4, &desc);
}
