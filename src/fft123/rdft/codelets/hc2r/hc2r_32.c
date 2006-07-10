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
/* Generated on Sat Jul  5 22:11:14 EDT 2003 */

#include "codelet_rdft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_hc2r -compact -variables 4 -sign 1 -n 32 -name hc2r_32 -include hc2r.h */

/*
 * This function contains 156 FP additions, 50 FP multiplications,
 * (or, 140 additions, 34 multiplications, 16 fused multiply/add),
 * 54 stack variables, and 64 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: hc2r_32.c,v 1.2 2006/06/14 11:55:17 glass Exp $
 * $Id: hc2r_32.c,v 1.2 2006/06/14 11:55:17 glass Exp $
 * $Id: hc2r_32.c,v 1.2 2006/06/14 11:55:17 glass Exp $
 */

#include "hc2r.h"

static void hc2r_32(const R *ri, const R *ii, R *O, stride ris, stride iis, stride os, int v, int ivs, int ovs)
{
     DK(KP1_662939224, +1.662939224605090474157576755235811513477121624);
     DK(KP1_111140466, +1.111140466039204449485661627897065748749874382);
     DK(KP1_961570560, +1.961570560806460898252364472268478073947867462);
     DK(KP390180644, +0.390180644032256535696569736954044481855383236);
     DK(KP765366864, +0.765366864730179543456919968060797733522689125);
     DK(KP1_847759065, +1.847759065022573512256366378793576573644833252);
     DK(KP707106781, +0.707106781186547524400844362104849039284835938);
     DK(KP1_414213562, +1.414213562373095048801688724209698078569671875);
     DK(KP2_000000000, +2.000000000000000000000000000000000000000000000);
     int i;
     for (i = v; i > 0; i = i - 1, ri = ri + ivs, ii = ii + ivs, O = O + ovs) {
	  E T9, T2c, TB, T1y, T6, T2b, Ty, T1v, Th, T2e, T2f, TD, TK, T1C, T1F;
	  E T1h, Tp, T2i, T2m, TN, T13, T1K, T1Y, T1k, Tw, TU, T1l, TW, T1V, T2j;
	  E T1R, T2l;
	  {
	       E T7, T8, T1w, Tz, TA, T1x;
	       T7 = ri[WS(ris, 4)];
	       T8 = ri[WS(ris, 12)];
	       T1w = T7 - T8;
	       Tz = ii[WS(iis, 4)];
	       TA = ii[WS(iis, 12)];
	       T1x = Tz + TA;
	       T9 = KP2_000000000 * (T7 + T8);
	       T2c = KP1_414213562 * (T1w + T1x);
	       TB = KP2_000000000 * (Tz - TA);
	       T1y = KP1_414213562 * (T1w - T1x);
	  }
	  {
	       E T5, T1u, T3, T1s;
	       {
		    E T4, T1t, T1, T2;
		    T4 = ri[WS(ris, 8)];
		    T5 = KP2_000000000 * T4;
		    T1t = ii[WS(iis, 8)];
		    T1u = KP2_000000000 * T1t;
		    T1 = ri[0];
		    T2 = ri[WS(ris, 16)];
		    T3 = T1 + T2;
		    T1s = T1 - T2;
	       }
	       T6 = T3 + T5;
	       T2b = T1s + T1u;
	       Ty = T3 - T5;
	       T1v = T1s - T1u;
	  }
	  {
	       E Td, T1A, TG, T1E, Tg, T1D, TJ, T1B;
	       {
		    E Tb, Tc, TE, TF;
		    Tb = ri[WS(ris, 2)];
		    Tc = ri[WS(ris, 14)];
		    Td = Tb + Tc;
		    T1A = Tb - Tc;
		    TE = ii[WS(iis, 2)];
		    TF = ii[WS(iis, 14)];
		    TG = TE - TF;
		    T1E = TE + TF;
	       }
	       {
		    E Te, Tf, TH, TI;
		    Te = ri[WS(ris, 10)];
		    Tf = ri[WS(ris, 6)];
		    Tg = Te + Tf;
		    T1D = Te - Tf;
		    TH = ii[WS(iis, 10)];
		    TI = ii[WS(iis, 6)];
		    TJ = TH - TI;
		    T1B = TH + TI;
	       }
	       Th = KP2_000000000 * (Td + Tg);
	       T2e = T1A + T1B;
	       T2f = T1E - T1D;
	       TD = Td - Tg;
	       TK = TG - TJ;
	       T1C = T1A - T1B;
	       T1F = T1D + T1E;
	       T1h = KP2_000000000 * (TJ + TG);
	  }
	  {
	       E Tl, T1I, TZ, T1X, To, T1W, T12, T1J;
	       {
		    E Tj, Tk, TX, TY;
		    Tj = ri[WS(ris, 1)];
		    Tk = ri[WS(ris, 15)];
		    Tl = Tj + Tk;
		    T1I = Tj - Tk;
		    TX = ii[WS(iis, 1)];
		    TY = ii[WS(iis, 15)];
		    TZ = TX - TY;
		    T1X = TX + TY;
	       }
	       {
		    E Tm, Tn, T10, T11;
		    Tm = ri[WS(ris, 9)];
		    Tn = ri[WS(ris, 7)];
		    To = Tm + Tn;
		    T1W = Tm - Tn;
		    T10 = ii[WS(iis, 9)];
		    T11 = ii[WS(iis, 7)];
		    T12 = T10 - T11;
		    T1J = T10 + T11;
	       }
	       Tp = Tl + To;
	       T2i = T1I + T1J;
	       T2m = T1X - T1W;
	       TN = Tl - To;
	       T13 = TZ - T12;
	       T1K = T1I - T1J;
	       T1Y = T1W + T1X;
	       T1k = T12 + TZ;
	  }
	  {
	       E Ts, T1L, TT, T1M, Tv, T1O, TQ, T1P;
	       {
		    E Tq, Tr, TR, TS;
		    Tq = ri[WS(ris, 5)];
		    Tr = ri[WS(ris, 11)];
		    Ts = Tq + Tr;
		    T1L = Tq - Tr;
		    TR = ii[WS(iis, 5)];
		    TS = ii[WS(iis, 11)];
		    TT = TR - TS;
		    T1M = TR + TS;
	       }
	       {
		    E Tt, Tu, TO, TP;
		    Tt = ri[WS(ris, 3)];
		    Tu = ri[WS(ris, 13)];
		    Tv = Tt + Tu;
		    T1O = Tt - Tu;
		    TO = ii[WS(iis, 13)];
		    TP = ii[WS(iis, 3)];
		    TQ = TO - TP;
		    T1P = TP + TO;
	       }
	       Tw = Ts + Tv;
	       TU = TQ - TT;
	       T1l = TT + TQ;
	       TW = Ts - Tv;
	       {
		    E T1T, T1U, T1N, T1Q;
		    T1T = T1L + T1M;
		    T1U = T1O + T1P;
		    T1V = KP707106781 * (T1T - T1U);
		    T2j = KP707106781 * (T1T + T1U);
		    T1N = T1L - T1M;
		    T1Q = T1O - T1P;
		    T1R = KP707106781 * (T1N + T1Q);
		    T2l = KP707106781 * (T1N - T1Q);
	       }
	  }
	  {
	       E Tx, T1r, Ti, T1q, Ta;
	       Tx = KP2_000000000 * (Tp + Tw);
	       T1r = KP2_000000000 * (T1l + T1k);
	       Ta = T6 + T9;
	       Ti = Ta + Th;
	       T1q = Ta - Th;
	       O[WS(os, 16)] = Ti - Tx;
	       O[WS(os, 24)] = T1q + T1r;
	       O[0] = Ti + Tx;
	       O[WS(os, 8)] = T1q - T1r;
	  }
	  {
	       E T1i, T1o, T1n, T1p, T1g, T1j, T1m;
	       T1g = T6 - T9;
	       T1i = T1g - T1h;
	       T1o = T1g + T1h;
	       T1j = Tp - Tw;
	       T1m = T1k - T1l;
	       T1n = KP1_414213562 * (T1j - T1m);
	       T1p = KP1_414213562 * (T1j + T1m);
	       O[WS(os, 20)] = T1i - T1n;
	       O[WS(os, 28)] = T1o + T1p;
	       O[WS(os, 4)] = T1i + T1n;
	       O[WS(os, 12)] = T1o - T1p;
	  }
	  {
	       E TM, T16, T15, T17;
	       {
		    E TC, TL, TV, T14;
		    TC = Ty - TB;
		    TL = KP1_414213562 * (TD - TK);
		    TM = TC + TL;
		    T16 = TC - TL;
		    TV = TN + TU;
		    T14 = TW + T13;
		    T15 = FNMS(KP765366864, T14, KP1_847759065 * TV);
		    T17 = FMA(KP765366864, TV, KP1_847759065 * T14);
	       }
	       O[WS(os, 18)] = TM - T15;
	       O[WS(os, 26)] = T16 + T17;
	       O[WS(os, 2)] = TM + T15;
	       O[WS(os, 10)] = T16 - T17;
	  }
	  {
	       E T2t, T2x, T2w, T2y;
	       {
		    E T2r, T2s, T2u, T2v;
		    T2r = T2b + T2c;
		    T2s = FMA(KP1_847759065, T2e, KP765366864 * T2f);
		    T2t = T2r - T2s;
		    T2x = T2r + T2s;
		    T2u = T2i + T2j;
		    T2v = T2m - T2l;
		    T2w = FNMS(KP1_961570560, T2v, KP390180644 * T2u);
		    T2y = FMA(KP1_961570560, T2u, KP390180644 * T2v);
	       }
	       O[WS(os, 23)] = T2t - T2w;
	       O[WS(os, 31)] = T2x + T2y;
	       O[WS(os, 7)] = T2t + T2w;
	       O[WS(os, 15)] = T2x - T2y;
	  }
	  {
	       E T1a, T1e, T1d, T1f;
	       {
		    E T18, T19, T1b, T1c;
		    T18 = Ty + TB;
		    T19 = KP1_414213562 * (TD + TK);
		    T1a = T18 - T19;
		    T1e = T18 + T19;
		    T1b = TN - TU;
		    T1c = T13 - TW;
		    T1d = FNMS(KP1_847759065, T1c, KP765366864 * T1b);
		    T1f = FMA(KP1_847759065, T1b, KP765366864 * T1c);
	       }
	       O[WS(os, 22)] = T1a - T1d;
	       O[WS(os, 30)] = T1e + T1f;
	       O[WS(os, 6)] = T1a + T1d;
	       O[WS(os, 14)] = T1e - T1f;
	  }
	  {
	       E T25, T29, T28, T2a;
	       {
		    E T23, T24, T26, T27;
		    T23 = T1v - T1y;
		    T24 = FMA(KP765366864, T1C, KP1_847759065 * T1F);
		    T25 = T23 - T24;
		    T29 = T23 + T24;
		    T26 = T1K - T1R;
		    T27 = T1Y - T1V;
		    T28 = FNMS(KP1_662939224, T27, KP1_111140466 * T26);
		    T2a = FMA(KP1_662939224, T26, KP1_111140466 * T27);
	       }
	       O[WS(os, 21)] = T25 - T28;
	       O[WS(os, 29)] = T29 + T2a;
	       O[WS(os, 5)] = T25 + T28;
	       O[WS(os, 13)] = T29 - T2a;
	  }
	  {
	       E T2h, T2p, T2o, T2q;
	       {
		    E T2d, T2g, T2k, T2n;
		    T2d = T2b - T2c;
		    T2g = FNMS(KP1_847759065, T2f, KP765366864 * T2e);
		    T2h = T2d + T2g;
		    T2p = T2d - T2g;
		    T2k = T2i - T2j;
		    T2n = T2l + T2m;
		    T2o = FNMS(KP1_111140466, T2n, KP1_662939224 * T2k);
		    T2q = FMA(KP1_111140466, T2k, KP1_662939224 * T2n);
	       }
	       O[WS(os, 19)] = T2h - T2o;
	       O[WS(os, 27)] = T2p + T2q;
	       O[WS(os, 3)] = T2h + T2o;
	       O[WS(os, 11)] = T2p - T2q;
	  }
	  {
	       E T1H, T21, T20, T22;
	       {
		    E T1z, T1G, T1S, T1Z;
		    T1z = T1v + T1y;
		    T1G = FNMS(KP765366864, T1F, KP1_847759065 * T1C);
		    T1H = T1z + T1G;
		    T21 = T1z - T1G;
		    T1S = T1K + T1R;
		    T1Z = T1V + T1Y;
		    T20 = FNMS(KP390180644, T1Z, KP1_961570560 * T1S);
		    T22 = FMA(KP390180644, T1S, KP1_961570560 * T1Z);
	       }
	       O[WS(os, 17)] = T1H - T20;
	       O[WS(os, 25)] = T21 + T22;
	       O[WS(os, 1)] = T1H + T20;
	       O[WS(os, 9)] = T21 - T22;
	  }
     }
}

static const khc2r_desc desc = { 32, "hc2r_32", {140, 34, 16, 0}, &GENUS, 0, 0, 0, 0, 0 };

void X(codelet_hc2r_32) (planner *p) {
     X(khc2r_register) (p, hc2r_32, &desc);
}
