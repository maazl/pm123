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
/* Generated on Sat Jul  5 21:58:16 EDT 2003 */

#include "codelet-rdft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_r2hc -compact -variables 4 -n 15 -name r2hcII_15 -dft-II -include r2hcII.h */

/*
 * This function contains 72 FP additions, 33 FP multiplications,
 * (or, 54 additions, 15 multiplications, 18 fused multiply/add),
 * 37 stack variables, and 30 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: r2hcII_15.c,v 1.1 2005/07/26 17:37:08 glass Exp $
 * $Id: r2hcII_15.c,v 1.1 2005/07/26 17:37:08 glass Exp $
 * $Id: r2hcII_15.c,v 1.1 2005/07/26 17:37:08 glass Exp $
 */

#include "r2hcII.h"

static void r2hcII_15(const R *I, R *ro, R *io, stride is, stride ros, stride ios, int v, int ivs, int ovs)
{
     DK(KP500000000, +0.500000000000000000000000000000000000000000000);
     DK(KP866025403, +0.866025403784438646763723170752936183471402627);
     DK(KP809016994, +0.809016994374947424102293417182819058860154590);
     DK(KP309016994, +0.309016994374947424102293417182819058860154590);
     DK(KP250000000, +0.250000000000000000000000000000000000000000000);
     DK(KP559016994, +0.559016994374947424102293417182819058860154590);
     DK(KP587785252, +0.587785252292473129168705954639072768597652438);
     DK(KP951056516, +0.951056516295153572116439333379382143405698634);
     int i;
     for (i = v; i > 0; i = i - 1, I = I + ivs, ro = ro + ovs, io = io + ovs) {
	  E T1, T2, Tx, TR, TE, T7, TD, Th, Tm, Tr, TQ, TA, TB, Tf, Te;
	  E Tu, TS, Td, TH, TO;
	  T1 = I[WS(is, 10)];
	  {
	       E T3, Tv, T6, Tw, T4, T5;
	       T2 = I[WS(is, 4)];
	       T3 = I[WS(is, 1)];
	       Tv = T2 + T3;
	       T4 = I[WS(is, 7)];
	       T5 = I[WS(is, 13)];
	       T6 = T4 + T5;
	       Tw = T4 - T5;
	       Tx = FMA(KP951056516, Tv, KP587785252 * Tw);
	       TR = FNMS(KP587785252, Tv, KP951056516 * Tw);
	       TE = KP559016994 * (T3 - T6);
	       T7 = T3 + T6;
	       TD = KP250000000 * T7;
	  }
	  {
	       E Ti, Tl, Tj, Tk, Tp, Tq;
	       Th = I[0];
	       Ti = I[WS(is, 9)];
	       Tl = I[WS(is, 12)];
	       Tj = I[WS(is, 3)];
	       Tk = I[WS(is, 6)];
	       Tp = Tk + Ti;
	       Tq = Tl + Tj;
	       Tm = Ti + Tj - (Tk + Tl);
	       Tr = FMA(KP951056516, Tp, KP587785252 * Tq);
	       TQ = FNMS(KP951056516, Tq, KP587785252 * Tp);
	       TA = FMA(KP250000000, Tm, Th);
	       TB = KP559016994 * (Tl + Ti - (Tk + Tj));
	  }
	  {
	       E T9, Tt, Tc, Ts, Ta, Tb, TG;
	       Tf = I[WS(is, 5)];
	       T9 = I[WS(is, 14)];
	       Te = I[WS(is, 11)];
	       Tt = T9 + Te;
	       Ta = I[WS(is, 2)];
	       Tb = I[WS(is, 8)];
	       Tc = Ta + Tb;
	       Ts = Ta - Tb;
	       Tu = FNMS(KP951056516, Tt, KP587785252 * Ts);
	       TS = FMA(KP951056516, Ts, KP587785252 * Tt);
	       Td = T9 + Tc;
	       TG = KP559016994 * (T9 - Tc);
	       TH = FNMS(KP309016994, Te, TG) + FNMA(KP250000000, Td, Tf);
	       TO = FMS(KP809016994, Te, Tf) + FNMA(KP250000000, Td, TG);
	  }
	  {
	       E Tn, T8, Tg, To;
	       Tn = Th - Tm;
	       T8 = T1 + T2 - T7;
	       Tg = Td - Te - Tf;
	       To = T8 + Tg;
	       io[WS(ios, 2)] = KP866025403 * (T8 - Tg);
	       ro[WS(ros, 2)] = FNMS(KP500000000, To, Tn);
	       ro[WS(ros, 7)] = Tn + To;
	  }
	  {
	       E TM, TX, TT, TV, TP, TU, TN, TW;
	       TM = TB + TA;
	       TX = KP866025403 * (TR + TS);
	       TT = TR - TS;
	       TV = FMS(KP500000000, TT, TQ);
	       TN = T1 + TE + FNMS(KP809016994, T2, TD);
	       TP = TN + TO;
	       TU = KP866025403 * (TO - TN);
	       ro[WS(ros, 1)] = TM + TP;
	       io[WS(ios, 1)] = TQ + TT;
	       io[WS(ios, 6)] = TU - TV;
	       io[WS(ios, 3)] = TU + TV;
	       TW = FNMS(KP500000000, TP, TM);
	       ro[WS(ros, 3)] = TW - TX;
	       ro[WS(ros, 6)] = TW + TX;
	  }
	  {
	       E Tz, TC, Ty, TK, TI, TL, TF, TJ;
	       Tz = KP866025403 * (Tx + Tu);
	       TC = TA - TB;
	       Ty = Tu - Tx;
	       TK = FMS(KP500000000, Ty, Tr);
	       TF = FMA(KP309016994, T2, T1) + TD - TE;
	       TI = TF + TH;
	       TL = KP866025403 * (TH - TF);
	       io[WS(ios, 4)] = Tr + Ty;
	       ro[WS(ros, 4)] = TC + TI;
	       io[WS(ios, 5)] = TK - TL;
	       io[0] = TK + TL;
	       TJ = FNMS(KP500000000, TI, TC);
	       ro[0] = Tz + TJ;
	       ro[WS(ros, 5)] = TJ - Tz;
	  }
     }
}

static const kr2hc_desc desc = { 15, "r2hcII_15", {54, 15, 18, 0}, &GENUS, 0, 0, 0, 0, 0 };

void X(codelet_r2hcII_15) (planner *p) {
     X(kr2hcII_register) (p, r2hcII_15, &desc);
}
