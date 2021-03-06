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
/* Generated on Sat Jul  5 21:56:43 EDT 2003 */

#include "codelet_rdft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_r2hc -compact -variables 4 -n 16 -name r2hc_16 -include r2hc.h */

/*
 * This function contains 58 FP additions, 12 FP multiplications,
 * (or, 54 additions, 8 multiplications, 4 fused multiply/add),
 * 34 stack variables, and 32 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: r2hc_16.c,v 1.2 2006/06/14 11:56:49 glass Exp $
 * $Id: r2hc_16.c,v 1.2 2006/06/14 11:56:49 glass Exp $
 * $Id: r2hc_16.c,v 1.2 2006/06/14 11:56:49 glass Exp $
 */

#include "r2hc.h"

static void r2hc_16(const R *I, R *ro, R *io, stride is, stride ros, stride ios, int v, int ivs, int ovs)
{
     DK(KP923879532, +0.923879532511286756128183189396788286822416626);
     DK(KP382683432, +0.382683432365089771728459984030398866761344562);
     DK(KP707106781, +0.707106781186547524400844362104849039284835938);
     int i;
     for (i = v; i > 0; i = i - 1, I = I + ivs, ro = ro + ovs, io = io + ovs) {
	  E T3, T6, T7, Tz, Ti, Ta, Td, Te, TA, Th, Tq, TV, TF, TP, Tx;
	  E TU, TE, TM, Tg, Tf, TJ, TQ;
	  {
	       E T1, T2, T4, T5;
	       T1 = I[0];
	       T2 = I[WS(is, 8)];
	       T3 = T1 + T2;
	       T4 = I[WS(is, 4)];
	       T5 = I[WS(is, 12)];
	       T6 = T4 + T5;
	       T7 = T3 + T6;
	       Tz = T1 - T2;
	       Ti = T4 - T5;
	  }
	  {
	       E T8, T9, Tb, Tc;
	       T8 = I[WS(is, 2)];
	       T9 = I[WS(is, 10)];
	       Ta = T8 + T9;
	       Tg = T8 - T9;
	       Tb = I[WS(is, 14)];
	       Tc = I[WS(is, 6)];
	       Td = Tb + Tc;
	       Tf = Tb - Tc;
	  }
	  Te = Ta + Td;
	  TA = KP707106781 * (Tg + Tf);
	  Th = KP707106781 * (Tf - Tg);
	  {
	       E Tm, TN, Tp, TO;
	       {
		    E Tk, Tl, Tn, To;
		    Tk = I[WS(is, 15)];
		    Tl = I[WS(is, 7)];
		    Tm = Tk - Tl;
		    TN = Tk + Tl;
		    Tn = I[WS(is, 3)];
		    To = I[WS(is, 11)];
		    Tp = Tn - To;
		    TO = Tn + To;
	       }
	       Tq = FNMS(KP923879532, Tp, KP382683432 * Tm);
	       TV = TN + TO;
	       TF = FMA(KP923879532, Tm, KP382683432 * Tp);
	       TP = TN - TO;
	  }
	  {
	       E Tt, TK, Tw, TL;
	       {
		    E Tr, Ts, Tu, Tv;
		    Tr = I[WS(is, 1)];
		    Ts = I[WS(is, 9)];
		    Tt = Tr - Ts;
		    TK = Tr + Ts;
		    Tu = I[WS(is, 5)];
		    Tv = I[WS(is, 13)];
		    Tw = Tu - Tv;
		    TL = Tu + Tv;
	       }
	       Tx = FMA(KP382683432, Tt, KP923879532 * Tw);
	       TU = TK + TL;
	       TE = FNMS(KP382683432, Tw, KP923879532 * Tt);
	       TM = TK - TL;
	  }
	  ro[WS(ros, 4)] = T7 - Te;
	  io[WS(ios, 4)] = TV - TU;
	  {
	       E Tj, Ty, TD, TG;
	       Tj = Th - Ti;
	       Ty = Tq - Tx;
	       io[WS(ios, 1)] = Tj + Ty;
	       io[WS(ios, 7)] = Ty - Tj;
	       TD = Tz + TA;
	       TG = TE + TF;
	       ro[WS(ros, 7)] = TD - TG;
	       ro[WS(ros, 1)] = TD + TG;
	  }
	  {
	       E TB, TC, TH, TI;
	       TB = Tz - TA;
	       TC = Tx + Tq;
	       ro[WS(ros, 5)] = TB - TC;
	       ro[WS(ros, 3)] = TB + TC;
	       TH = Ti + Th;
	       TI = TF - TE;
	       io[WS(ios, 3)] = TH + TI;
	       io[WS(ios, 5)] = TI - TH;
	  }
	  TJ = T3 - T6;
	  TQ = KP707106781 * (TM + TP);
	  ro[WS(ros, 6)] = TJ - TQ;
	  ro[WS(ros, 2)] = TJ + TQ;
	  {
	       E TR, TS, TT, TW;
	       TR = Td - Ta;
	       TS = KP707106781 * (TP - TM);
	       io[WS(ios, 2)] = TR + TS;
	       io[WS(ios, 6)] = TS - TR;
	       TT = T7 + Te;
	       TW = TU + TV;
	       ro[WS(ros, 8)] = TT - TW;
	       ro[0] = TT + TW;
	  }
     }
}

static const kr2hc_desc desc = { 16, "r2hc_16", {54, 8, 4, 0}, &GENUS, 0, 0, 0, 0, 0 };

void X(codelet_r2hc_16) (planner *p) {
     X(kr2hc_register) (p, r2hc_16, &desc);
}
