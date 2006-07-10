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
/* Generated on Sat Jul  5 21:29:34 EDT 2003 */

#include "codelet_dft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_notw -compact -variables 4 -n 15 -name n1_15 -include n.h */

/*
 * This function contains 156 FP additions, 56 FP multiplications,
 * (or, 128 additions, 28 multiplications, 28 fused multiply/add),
 * 69 stack variables, and 60 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: n1_15.c,v 1.2 2006/06/14 10:56:04 glass Exp $
 * $Id: n1_15.c,v 1.2 2006/06/14 10:56:04 glass Exp $
 * $Id: n1_15.c,v 1.2 2006/06/14 10:56:04 glass Exp $
 */

#include "n.h"

static void n1_15(const R *ri, const R *ii, R *ro, R *io, stride is, stride os, int v, int ivs, int ovs)
{
     DK(KP587785252, +0.587785252292473129168705954639072768597652438);
     DK(KP951056516, +0.951056516295153572116439333379382143405698634);
     DK(KP250000000, +0.250000000000000000000000000000000000000000000);
     DK(KP559016994, +0.559016994374947424102293417182819058860154590);
     DK(KP500000000, +0.500000000000000000000000000000000000000000000);
     DK(KP866025403, +0.866025403784438646763723170752936183471402627);
     int i;
     for (i = v; i > 0; i = i - 1, ri = ri + ivs, ii = ii + ivs, ro = ro + ovs, io = io + ovs) {
	  E T5, T2l, Tx, TV, T1C, T20, Tl, Tq, Tr, TN, TS, TT, T2c, T2d, T2n;
	  E T1O, T1P, T22, T1l, T1q, T1w, TZ, T10, T11, Ta, Tf, Tg, TC, TH, TI;
	  E T2f, T2g, T2m, T1R, T1S, T21, T1a, T1f, T1v, TW, TX, TY;
	  {
	       E T1, T1z, T4, T1y, Tw, T1A, Tt, T1B;
	       T1 = ri[0];
	       T1z = ii[0];
	       {
		    E T2, T3, Tu, Tv;
		    T2 = ri[WS(is, 5)];
		    T3 = ri[WS(is, 10)];
		    T4 = T2 + T3;
		    T1y = KP866025403 * (T3 - T2);
		    Tu = ii[WS(is, 5)];
		    Tv = ii[WS(is, 10)];
		    Tw = KP866025403 * (Tu - Tv);
		    T1A = Tu + Tv;
	       }
	       T5 = T1 + T4;
	       T2l = T1z + T1A;
	       Tt = FNMS(KP500000000, T4, T1);
	       Tx = Tt - Tw;
	       TV = Tt + Tw;
	       T1B = FNMS(KP500000000, T1A, T1z);
	       T1C = T1y + T1B;
	       T20 = T1B - T1y;
	  }
	  {
	       E Th, Tk, TJ, T1h, T1i, T1j, TM, T1k, Tm, Tp, TO, T1m, T1n, T1o, TR;
	       E T1p;
	       {
		    E Ti, Tj, TK, TL;
		    Th = ri[WS(is, 6)];
		    Ti = ri[WS(is, 11)];
		    Tj = ri[WS(is, 1)];
		    Tk = Ti + Tj;
		    TJ = FNMS(KP500000000, Tk, Th);
		    T1h = KP866025403 * (Tj - Ti);
		    T1i = ii[WS(is, 6)];
		    TK = ii[WS(is, 11)];
		    TL = ii[WS(is, 1)];
		    T1j = TK + TL;
		    TM = KP866025403 * (TK - TL);
		    T1k = FNMS(KP500000000, T1j, T1i);
	       }
	       {
		    E Tn, To, TP, TQ;
		    Tm = ri[WS(is, 9)];
		    Tn = ri[WS(is, 14)];
		    To = ri[WS(is, 4)];
		    Tp = Tn + To;
		    TO = FNMS(KP500000000, Tp, Tm);
		    T1m = KP866025403 * (To - Tn);
		    T1n = ii[WS(is, 9)];
		    TP = ii[WS(is, 14)];
		    TQ = ii[WS(is, 4)];
		    T1o = TP + TQ;
		    TR = KP866025403 * (TP - TQ);
		    T1p = FNMS(KP500000000, T1o, T1n);
	       }
	       Tl = Th + Tk;
	       Tq = Tm + Tp;
	       Tr = Tl + Tq;
	       TN = TJ - TM;
	       TS = TO - TR;
	       TT = TN + TS;
	       T2c = T1i + T1j;
	       T2d = T1n + T1o;
	       T2n = T2c + T2d;
	       T1O = T1k - T1h;
	       T1P = T1p - T1m;
	       T22 = T1O + T1P;
	       T1l = T1h + T1k;
	       T1q = T1m + T1p;
	       T1w = T1l + T1q;
	       TZ = TJ + TM;
	       T10 = TO + TR;
	       T11 = TZ + T10;
	  }
	  {
	       E T6, T9, Ty, T16, T17, T18, TB, T19, Tb, Te, TD, T1b, T1c, T1d, TG;
	       E T1e;
	       {
		    E T7, T8, Tz, TA;
		    T6 = ri[WS(is, 3)];
		    T7 = ri[WS(is, 8)];
		    T8 = ri[WS(is, 13)];
		    T9 = T7 + T8;
		    Ty = FNMS(KP500000000, T9, T6);
		    T16 = KP866025403 * (T8 - T7);
		    T17 = ii[WS(is, 3)];
		    Tz = ii[WS(is, 8)];
		    TA = ii[WS(is, 13)];
		    T18 = Tz + TA;
		    TB = KP866025403 * (Tz - TA);
		    T19 = FNMS(KP500000000, T18, T17);
	       }
	       {
		    E Tc, Td, TE, TF;
		    Tb = ri[WS(is, 12)];
		    Tc = ri[WS(is, 2)];
		    Td = ri[WS(is, 7)];
		    Te = Tc + Td;
		    TD = FNMS(KP500000000, Te, Tb);
		    T1b = KP866025403 * (Td - Tc);
		    T1c = ii[WS(is, 12)];
		    TE = ii[WS(is, 2)];
		    TF = ii[WS(is, 7)];
		    T1d = TE + TF;
		    TG = KP866025403 * (TE - TF);
		    T1e = FNMS(KP500000000, T1d, T1c);
	       }
	       Ta = T6 + T9;
	       Tf = Tb + Te;
	       Tg = Ta + Tf;
	       TC = Ty - TB;
	       TH = TD - TG;
	       TI = TC + TH;
	       T2f = T17 + T18;
	       T2g = T1c + T1d;
	       T2m = T2f + T2g;
	       T1R = T19 - T16;
	       T1S = T1e - T1b;
	       T21 = T1R + T1S;
	       T1a = T16 + T19;
	       T1f = T1b + T1e;
	       T1v = T1a + T1f;
	       TW = Ty + TB;
	       TX = TD + TG;
	       TY = TW + TX;
	  }
	  {
	       E T2a, Ts, T29, T2i, T2k, T2e, T2h, T2j, T2b;
	       T2a = KP559016994 * (Tg - Tr);
	       Ts = Tg + Tr;
	       T29 = FNMS(KP250000000, Ts, T5);
	       T2e = T2c - T2d;
	       T2h = T2f - T2g;
	       T2i = FNMS(KP587785252, T2h, KP951056516 * T2e);
	       T2k = FMA(KP951056516, T2h, KP587785252 * T2e);
	       ro[0] = T5 + Ts;
	       T2j = T2a + T29;
	       ro[WS(os, 9)] = T2j - T2k;
	       ro[WS(os, 6)] = T2j + T2k;
	       T2b = T29 - T2a;
	       ro[WS(os, 12)] = T2b - T2i;
	       ro[WS(os, 3)] = T2b + T2i;
	  }
	  {
	       E T2q, T2o, T2p, T2u, T2w, T2s, T2t, T2v, T2r;
	       T2q = KP559016994 * (T2m - T2n);
	       T2o = T2m + T2n;
	       T2p = FNMS(KP250000000, T2o, T2l);
	       T2s = Tl - Tq;
	       T2t = Ta - Tf;
	       T2u = FNMS(KP587785252, T2t, KP951056516 * T2s);
	       T2w = FMA(KP951056516, T2t, KP587785252 * T2s);
	       io[0] = T2l + T2o;
	       T2v = T2q + T2p;
	       io[WS(os, 6)] = T2v - T2w;
	       io[WS(os, 9)] = T2w + T2v;
	       T2r = T2p - T2q;
	       io[WS(os, 3)] = T2r - T2u;
	       io[WS(os, 12)] = T2u + T2r;
	  }
	  {
	       E T1M, TU, T1L, T1U, T1W, T1Q, T1T, T1V, T1N;
	       T1M = KP559016994 * (TI - TT);
	       TU = TI + TT;
	       T1L = FNMS(KP250000000, TU, Tx);
	       T1Q = T1O - T1P;
	       T1T = T1R - T1S;
	       T1U = FNMS(KP587785252, T1T, KP951056516 * T1Q);
	       T1W = FMA(KP951056516, T1T, KP587785252 * T1Q);
	       ro[WS(os, 5)] = Tx + TU;
	       T1V = T1M + T1L;
	       ro[WS(os, 14)] = T1V - T1W;
	       ro[WS(os, 11)] = T1V + T1W;
	       T1N = T1L - T1M;
	       ro[WS(os, 2)] = T1N - T1U;
	       ro[WS(os, 8)] = T1N + T1U;
	  }
	  {
	       E T25, T23, T24, T1Z, T28, T1X, T1Y, T27, T26;
	       T25 = KP559016994 * (T21 - T22);
	       T23 = T21 + T22;
	       T24 = FNMS(KP250000000, T23, T20);
	       T1X = TN - TS;
	       T1Y = TC - TH;
	       T1Z = FNMS(KP587785252, T1Y, KP951056516 * T1X);
	       T28 = FMA(KP951056516, T1Y, KP587785252 * T1X);
	       io[WS(os, 5)] = T20 + T23;
	       T27 = T25 + T24;
	       io[WS(os, 11)] = T27 - T28;
	       io[WS(os, 14)] = T28 + T27;
	       T26 = T24 - T25;
	       io[WS(os, 2)] = T1Z + T26;
	       io[WS(os, 8)] = T26 - T1Z;
	  }
	  {
	       E T1x, T1D, T1E, T1I, T1J, T1G, T1H, T1K, T1F;
	       T1x = KP559016994 * (T1v - T1w);
	       T1D = T1v + T1w;
	       T1E = FNMS(KP250000000, T1D, T1C);
	       T1G = TW - TX;
	       T1H = TZ - T10;
	       T1I = FMA(KP951056516, T1G, KP587785252 * T1H);
	       T1J = FNMS(KP587785252, T1G, KP951056516 * T1H);
	       io[WS(os, 10)] = T1C + T1D;
	       T1K = T1E - T1x;
	       io[WS(os, 7)] = T1J + T1K;
	       io[WS(os, 13)] = T1K - T1J;
	       T1F = T1x + T1E;
	       io[WS(os, 1)] = T1F - T1I;
	       io[WS(os, 4)] = T1I + T1F;
	  }
	  {
	       E T13, T12, T14, T1s, T1u, T1g, T1r, T1t, T15;
	       T13 = KP559016994 * (TY - T11);
	       T12 = TY + T11;
	       T14 = FNMS(KP250000000, T12, TV);
	       T1g = T1a - T1f;
	       T1r = T1l - T1q;
	       T1s = FMA(KP951056516, T1g, KP587785252 * T1r);
	       T1u = FNMS(KP587785252, T1g, KP951056516 * T1r);
	       ro[WS(os, 10)] = TV + T12;
	       T1t = T14 - T13;
	       ro[WS(os, 7)] = T1t - T1u;
	       ro[WS(os, 13)] = T1t + T1u;
	       T15 = T13 + T14;
	       ro[WS(os, 4)] = T15 - T1s;
	       ro[WS(os, 1)] = T15 + T1s;
	  }
     }
}

static const kdft_desc desc = { 15, "n1_15", {128, 28, 28, 0}, &GENUS, 0, 0, 0, 0 };
void X(codelet_n1_15) (planner *p) {
     X(kdft_register) (p, n1_15, &desc);
}
