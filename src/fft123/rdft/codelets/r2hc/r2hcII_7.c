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
/* Generated on Sat Jul  5 21:58:04 EDT 2003 */

#include "codelet-rdft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_r2hc -compact -variables 4 -n 7 -name r2hcII_7 -dft-II -include r2hcII.h */

/*
 * This function contains 24 FP additions, 18 FP multiplications,
 * (or, 12 additions, 6 multiplications, 12 fused multiply/add),
 * 20 stack variables, and 14 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: r2hcII_7.c,v 1.1 2005/07/26 17:37:09 glass Exp $
 * $Id: r2hcII_7.c,v 1.1 2005/07/26 17:37:09 glass Exp $
 * $Id: r2hcII_7.c,v 1.1 2005/07/26 17:37:09 glass Exp $
 */

#include "r2hcII.h"

static void r2hcII_7(const R *I, R *ro, R *io, stride is, stride ros, stride ios, int v, int ivs, int ovs)
{
     DK(KP900968867, +0.900968867902419126236102319507445051165919162);
     DK(KP222520933, +0.222520933956314404288902564496794759466355569);
     DK(KP623489801, +0.623489801858733530525004884004239810632274731);
     DK(KP433883739, +0.433883739117558120475768332848358754609990728);
     DK(KP974927912, +0.974927912181823607018131682993931217232785801);
     DK(KP781831482, +0.781831482468029808708444526674057750232334519);
     int i;
     for (i = v; i > 0; i = i - 1, I = I + ivs, ro = ro + ovs, io = io + ovs) {
	  E T1, Ta, Td, T4, Tb, T7, Tc, T8, T9;
	  T1 = I[0];
	  T8 = I[WS(is, 1)];
	  T9 = I[WS(is, 6)];
	  Ta = T8 - T9;
	  Td = T8 + T9;
	  {
	       E T2, T3, T5, T6;
	       T2 = I[WS(is, 2)];
	       T3 = I[WS(is, 5)];
	       T4 = T2 - T3;
	       Tb = T2 + T3;
	       T5 = I[WS(is, 3)];
	       T6 = I[WS(is, 4)];
	       T7 = T5 - T6;
	       Tc = T5 + T6;
	  }
	  io[0] = -(FMA(KP781831482, Tb, KP974927912 * Tc) + (KP433883739 * Td));
	  io[WS(ios, 1)] = FNMS(KP974927912, Td, KP781831482 * Tc) - (KP433883739 * Tb);
	  ro[0] = FMA(KP623489801, T4, T1) + FMA(KP222520933, T7, KP900968867 * Ta);
	  io[WS(ios, 2)] = FNMS(KP781831482, Td, KP974927912 * Tb) - (KP433883739 * Tc);
	  ro[WS(ros, 2)] = FMA(KP900968867, T7, T1) + FNMA(KP623489801, Ta, KP222520933 * T4);
	  ro[WS(ros, 1)] = FMA(KP222520933, Ta, T1) + FNMA(KP623489801, T7, KP900968867 * T4);
	  ro[WS(ros, 3)] = T1 + T4 - (T7 + Ta);
     }
}

static const kr2hc_desc desc = { 7, "r2hcII_7", {12, 6, 12, 0}, &GENUS, 0, 0, 0, 0, 0 };

void X(codelet_r2hcII_7) (planner *p) {
     X(kr2hcII_register) (p, r2hcII_7, &desc);
}
