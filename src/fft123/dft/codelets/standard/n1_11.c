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
/* Generated on Sat Jul  5 21:29:32 EDT 2003 */

#include "codelet_dft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_notw -compact -variables 4 -n 11 -name n1_11 -include n.h */

/*
 * This function contains 140 FP additions, 100 FP multiplications,
 * (or, 60 additions, 20 multiplications, 80 fused multiply/add),
 * 41 stack variables, and 44 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: n1_11.c,v 1.2 2006/06/14 10:56:04 glass Exp $
 * $Id: n1_11.c,v 1.2 2006/06/14 10:56:04 glass Exp $
 * $Id: n1_11.c,v 1.2 2006/06/14 10:56:04 glass Exp $
 */

#include "n.h"

static void n1_11(const R *ri, const R *ii, R *ro, R *io, stride is, stride os, int v, int ivs, int ovs)
{
     DK(KP654860733, +0.654860733945285064056925072466293553183791199);
     DK(KP142314838, +0.142314838273285140443792668616369668791051361);
     DK(KP959492973, +0.959492973614497389890368057066327699062454848);
     DK(KP415415013, +0.415415013001886425529274149229623203524004910);
     DK(KP841253532, +0.841253532831181168861811648919367717513292498);
     DK(KP989821441, +0.989821441880932732376092037776718787376519372);
     DK(KP909631995, +0.909631995354518371411715383079028460060241051);
     DK(KP281732556, +0.281732556841429697711417915346616899035777899);
     DK(KP540640817, +0.540640817455597582107635954318691695431770608);
     DK(KP755749574, +0.755749574354258283774035843972344420179717445);
     int i;
     for (i = v; i > 0; i = i - 1, ri = ri + ivs, ii = ii + ivs, ro = ro + ovs, io = io + ovs) {
	  E T1, TM, T4, TG, Tk, TR, Tw, TN, T7, TK, Ta, TH, Tn, TQ, Td;
	  E TJ, Tq, TO, Tt, TP, Tg, TI;
	  {
	       E T2, T3, Ti, Tj;
	       T1 = ri[0];
	       TM = ii[0];
	       T2 = ri[WS(is, 1)];
	       T3 = ri[WS(is, 10)];
	       T4 = T2 + T3;
	       TG = T3 - T2;
	       Ti = ii[WS(is, 1)];
	       Tj = ii[WS(is, 10)];
	       Tk = Ti - Tj;
	       TR = Ti + Tj;
	       {
		    E Tu, Tv, T5, T6;
		    Tu = ii[WS(is, 2)];
		    Tv = ii[WS(is, 9)];
		    Tw = Tu - Tv;
		    TN = Tu + Tv;
		    T5 = ri[WS(is, 2)];
		    T6 = ri[WS(is, 9)];
		    T7 = T5 + T6;
		    TK = T6 - T5;
	       }
	  }
	  {
	       E T8, T9, To, Tp;
	       T8 = ri[WS(is, 3)];
	       T9 = ri[WS(is, 8)];
	       Ta = T8 + T9;
	       TH = T9 - T8;
	       {
		    E Tl, Tm, Tb, Tc;
		    Tl = ii[WS(is, 3)];
		    Tm = ii[WS(is, 8)];
		    Tn = Tl - Tm;
		    TQ = Tl + Tm;
		    Tb = ri[WS(is, 4)];
		    Tc = ri[WS(is, 7)];
		    Td = Tb + Tc;
		    TJ = Tc - Tb;
	       }
	       To = ii[WS(is, 4)];
	       Tp = ii[WS(is, 7)];
	       Tq = To - Tp;
	       TO = To + Tp;
	       {
		    E Tr, Ts, Te, Tf;
		    Tr = ii[WS(is, 5)];
		    Ts = ii[WS(is, 6)];
		    Tt = Tr - Ts;
		    TP = Tr + Ts;
		    Te = ri[WS(is, 5)];
		    Tf = ri[WS(is, 6)];
		    Tg = Te + Tf;
		    TI = Tf - Te;
	       }
	  }
	  {
	       E Tx, Th, TZ, T10;
	       ro[0] = T1 + T4 + T7 + Ta + Td + Tg;
	       io[0] = TM + TR + TN + TQ + TO + TP;
	       Tx = FMA(KP755749574, Tk, KP540640817 * Tn) + FNMS(KP909631995, Tt, KP281732556 * Tq) - (KP989821441 * Tw);
	       Th = FMA(KP841253532, Ta, T1) + FNMS(KP959492973, Td, KP415415013 * Tg) + FNMA(KP142314838, T7, KP654860733 * T4);
	       ro[WS(os, 7)] = Th - Tx;
	       ro[WS(os, 4)] = Th + Tx;
	       TZ = FMA(KP755749574, TG, KP540640817 * TH) + FNMS(KP909631995, TI, KP281732556 * TJ) - (KP989821441 * TK);
	       T10 = FMA(KP841253532, TQ, TM) + FNMS(KP959492973, TO, KP415415013 * TP) + FNMA(KP142314838, TN, KP654860733 * TR);
	       io[WS(os, 4)] = TZ + T10;
	       io[WS(os, 7)] = T10 - TZ;
	       {
		    E TX, TY, Tz, Ty;
		    TX = FMA(KP909631995, TG, KP755749574 * TK) + FNMA(KP540640817, TI, KP989821441 * TJ) - (KP281732556 * TH);
		    TY = FMA(KP415415013, TR, TM) + FNMS(KP142314838, TO, KP841253532 * TP) + FNMA(KP959492973, TQ, KP654860733 * TN);
		    io[WS(os, 2)] = TX + TY;
		    io[WS(os, 9)] = TY - TX;
		    Tz = FMA(KP909631995, Tk, KP755749574 * Tw) + FNMA(KP540640817, Tt, KP989821441 * Tq) - (KP281732556 * Tn);
		    Ty = FMA(KP415415013, T4, T1) + FNMS(KP142314838, Td, KP841253532 * Tg) + FNMA(KP959492973, Ta, KP654860733 * T7);
		    ro[WS(os, 9)] = Ty - Tz;
		    ro[WS(os, 2)] = Ty + Tz;
	       }
	  }
	  {
	       E TB, TA, TT, TU;
	       TB = FMA(KP540640817, Tk, KP909631995 * Tw) + FMA(KP989821441, Tn, KP755749574 * Tq) + (KP281732556 * Tt);
	       TA = FMA(KP841253532, T4, T1) + FNMS(KP959492973, Tg, KP415415013 * T7) + FNMA(KP654860733, Td, KP142314838 * Ta);
	       ro[WS(os, 10)] = TA - TB;
	       ro[WS(os, 1)] = TA + TB;
	       {
		    E TV, TW, TD, TC;
		    TV = FMA(KP540640817, TG, KP909631995 * TK) + FMA(KP989821441, TH, KP755749574 * TJ) + (KP281732556 * TI);
		    TW = FMA(KP841253532, TR, TM) + FNMS(KP959492973, TP, KP415415013 * TN) + FNMA(KP654860733, TO, KP142314838 * TQ);
		    io[WS(os, 1)] = TV + TW;
		    io[WS(os, 10)] = TW - TV;
		    TD = FMA(KP989821441, Tk, KP540640817 * Tq) + FNMS(KP909631995, Tn, KP755749574 * Tt) - (KP281732556 * Tw);
		    TC = FMA(KP415415013, Ta, T1) + FNMS(KP654860733, Tg, KP841253532 * Td) + FNMA(KP959492973, T7, KP142314838 * T4);
		    ro[WS(os, 8)] = TC - TD;
		    ro[WS(os, 3)] = TC + TD;
	       }
	       TT = FMA(KP989821441, TG, KP540640817 * TJ) + FNMS(KP909631995, TH, KP755749574 * TI) - (KP281732556 * TK);
	       TU = FMA(KP415415013, TQ, TM) + FNMS(KP654860733, TP, KP841253532 * TO) + FNMA(KP959492973, TN, KP142314838 * TR);
	       io[WS(os, 3)] = TT + TU;
	       io[WS(os, 8)] = TU - TT;
	       {
		    E TL, TS, TF, TE;
		    TL = FMA(KP281732556, TG, KP755749574 * TH) + FNMS(KP909631995, TJ, KP989821441 * TI) - (KP540640817 * TK);
		    TS = FMA(KP841253532, TN, TM) + FNMS(KP142314838, TP, KP415415013 * TO) + FNMA(KP654860733, TQ, KP959492973 * TR);
		    io[WS(os, 5)] = TL + TS;
		    io[WS(os, 6)] = TS - TL;
		    TF = FMA(KP281732556, Tk, KP755749574 * Tn) + FNMS(KP909631995, Tq, KP989821441 * Tt) - (KP540640817 * Tw);
		    TE = FMA(KP841253532, T7, T1) + FNMS(KP142314838, Tg, KP415415013 * Td) + FNMA(KP654860733, Ta, KP959492973 * T4);
		    ro[WS(os, 6)] = TE - TF;
		    ro[WS(os, 5)] = TE + TF;
	       }
	  }
     }
}

static const kdft_desc desc = { 11, "n1_11", {60, 20, 80, 0}, &GENUS, 0, 0, 0, 0 };
void X(codelet_n1_11) (planner *p) {
     X(kdft_register) (p, n1_11, &desc);
}
