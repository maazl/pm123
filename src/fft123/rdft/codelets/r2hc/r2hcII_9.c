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
/* Generated on Sat Jul  5 21:58:09 EDT 2003 */

#include "codelet_rdft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_r2hc -compact -variables 4 -n 9 -name r2hcII_9 -dft-II -include r2hcII.h */

/*
 * This function contains 42 FP additions, 30 FP multiplications,
 * (or, 25 additions, 13 multiplications, 17 fused multiply/add),
 * 39 stack variables, and 18 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: r2hcII_9.c,v 1.2 2006/06/14 11:56:49 glass Exp $
 * $Id: r2hcII_9.c,v 1.2 2006/06/14 11:56:49 glass Exp $
 * $Id: r2hcII_9.c,v 1.2 2006/06/14 11:56:49 glass Exp $
 */

#include "r2hcII.h"

static void r2hcII_9(const R *I, R *ro, R *io, stride is, stride ros, stride ios, int v, int ivs, int ovs)
{
     DK(KP663413948, +0.663413948168938396205421319635891297216863310);
     DK(KP642787609, +0.642787609686539326322643409907263432907559884);
     DK(KP556670399, +0.556670399226419366452912952047023132968291906);
     DK(KP766044443, +0.766044443118978035202392650555416673935832457);
     DK(KP852868531, +0.852868531952443209628250963940074071936020296);
     DK(KP173648177, +0.173648177666930348851716626769314796000375677);
     DK(KP984807753, +0.984807753012208059366743024589523013670643252);
     DK(KP150383733, +0.150383733180435296639271897612501926072238258);
     DK(KP813797681, +0.813797681349373692844693217248393223289101568);
     DK(KP342020143, +0.342020143325668733044099614682259580763083368);
     DK(KP939692620, +0.939692620785908384054109277324731469936208134);
     DK(KP296198132, +0.296198132726023843175338011893050938967728390);
     DK(KP866025403, +0.866025403784438646763723170752936183471402627);
     DK(KP500000000, +0.500000000000000000000000000000000000000000000);
     int i;
     for (i = v; i > 0; i = i - 1, I = I + ivs, ro = ro + ovs, io = io + ovs) {
	  E T1, T4, To, Ta, Tl, Tk, Tf, Ti, Th, T2, T3, T5, Tg;
	  T1 = I[0];
	  T2 = I[WS(is, 3)];
	  T3 = I[WS(is, 6)];
	  T4 = T2 - T3;
	  To = T2 + T3;
	  {
	       E T6, T7, T8, T9;
	       T6 = I[WS(is, 2)];
	       T7 = I[WS(is, 5)];
	       T8 = I[WS(is, 8)];
	       T9 = T7 - T8;
	       Ta = T6 - T9;
	       Tl = T7 + T8;
	       Tk = FMA(KP500000000, T9, T6);
	  }
	  {
	       E Tb, Tc, Td, Te;
	       Tb = I[WS(is, 4)];
	       Tc = I[WS(is, 1)];
	       Td = I[WS(is, 7)];
	       Te = Tc + Td;
	       Tf = Tb - Te;
	       Ti = FMA(KP500000000, Te, Tb);
	       Th = Tc - Td;
	  }
	  io[WS(ios, 1)] = KP866025403 * (Tf - Ta);
	  T5 = T1 - T4;
	  Tg = Ta + Tf;
	  ro[WS(ros, 1)] = FNMS(KP500000000, Tg, T5);
	  ro[WS(ros, 4)] = T5 + Tg;
	  {
	       E Tr, Tt, Tw, Tv, Tu, Tp, Tq, Ts, Tj, Tm, Tn;
	       Tr = FMA(KP500000000, T4, T1);
	       Tt = FMA(KP296198132, Th, KP939692620 * Ti);
	       Tw = FNMS(KP813797681, Th, KP342020143 * Ti);
	       Tv = FNMS(KP984807753, Tk, KP150383733 * Tl);
	       Tu = FMA(KP173648177, Tk, KP852868531 * Tl);
	       Tp = FNMS(KP556670399, Tl, KP766044443 * Tk);
	       Tq = FMA(KP852868531, Th, KP173648177 * Ti);
	       Ts = Tp + Tq;
	       Tj = FNMS(KP984807753, Ti, KP150383733 * Th);
	       Tm = FMA(KP642787609, Tk, KP663413948 * Tl);
	       Tn = Tj - Tm;
	       io[0] = FNMS(KP866025403, To, Tn);
	       ro[0] = Tr + Ts;
	       io[WS(ios, 3)] = FNMS(KP500000000, Tn, KP866025403 * ((Tp - Tq) - To));
	       ro[WS(ros, 3)] = FMA(KP866025403, Tm + Tj, Tr) - (KP500000000 * Ts);
	       io[WS(ios, 2)] = FMA(KP866025403, To - (Tu + Tt), KP500000000 * (Tw - Tv));
	       ro[WS(ros, 2)] = FMA(KP500000000, Tt - Tu, Tr) + (KP866025403 * (Tv + Tw));
	  }
     }
}

static const kr2hc_desc desc = { 9, "r2hcII_9", {25, 13, 17, 0}, &GENUS, 0, 0, 0, 0, 0 };

void X(codelet_r2hcII_9) (planner *p) {
     X(kr2hcII_register) (p, r2hcII_9, &desc);
}
