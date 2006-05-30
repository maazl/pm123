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
/* Generated on Sat Jul  5 21:30:03 EDT 2003 */

#include "codelet-dft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_twiddle -compact -variables 4 -n 12 -name t1_12 -include t.h */

/*
 * This function contains 118 FP additions, 60 FP multiplications,
 * (or, 88 additions, 30 multiplications, 30 fused multiply/add),
 * 47 stack variables, and 48 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: t1_12.c,v 1.1 2005/07/26 17:36:54 glass Exp $
 * $Id: t1_12.c,v 1.1 2005/07/26 17:36:54 glass Exp $
 * $Id: t1_12.c,v 1.1 2005/07/26 17:36:54 glass Exp $
 */

#include "t.h"

static const R *t1_12(R *ri, R *ii, const R *W, stride ios, int m, int dist)
{
     DK(KP500000000, +0.500000000000000000000000000000000000000000000);
     DK(KP866025403, +0.866025403784438646763723170752936183471402627);
     int i;
     for (i = m; i > 0; i = i - 1, ri = ri + dist, ii = ii + dist, W = W + 22) {
	  E T1, T1W, T18, T21, Tc, T15, T1V, T22, TR, T1E, T1o, T1D, T12, T1l, T1F;
	  E T1G, Ti, T1S, T1d, T24, Tt, T1a, T1T, T25, TA, T1z, T1j, T1y, TL, T1g;
	  E T1A, T1B;
	  {
	       E T6, T16, Tb, T17;
	       T1 = ri[0];
	       T1W = ii[0];
	       {
		    E T3, T5, T2, T4;
		    T3 = ri[WS(ios, 4)];
		    T5 = ii[WS(ios, 4)];
		    T2 = W[6];
		    T4 = W[7];
		    T6 = FMA(T2, T3, T4 * T5);
		    T16 = FNMS(T4, T3, T2 * T5);
	       }
	       {
		    E T8, Ta, T7, T9;
		    T8 = ri[WS(ios, 8)];
		    Ta = ii[WS(ios, 8)];
		    T7 = W[14];
		    T9 = W[15];
		    Tb = FMA(T7, T8, T9 * Ta);
		    T17 = FNMS(T9, T8, T7 * Ta);
	       }
	       T18 = KP866025403 * (T16 - T17);
	       T21 = KP866025403 * (Tb - T6);
	       Tc = T6 + Tb;
	       T15 = FNMS(KP500000000, Tc, T1);
	       T1V = T16 + T17;
	       T22 = FNMS(KP500000000, T1V, T1W);
	  }
	  {
	       E T11, T1n, TW, T1m;
	       {
		    E TO, TQ, TN, TP;
		    TO = ri[WS(ios, 9)];
		    TQ = ii[WS(ios, 9)];
		    TN = W[16];
		    TP = W[17];
		    TR = FMA(TN, TO, TP * TQ);
		    T1E = FNMS(TP, TO, TN * TQ);
	       }
	       {
		    E TY, T10, TX, TZ;
		    TY = ri[WS(ios, 5)];
		    T10 = ii[WS(ios, 5)];
		    TX = W[8];
		    TZ = W[9];
		    T11 = FMA(TX, TY, TZ * T10);
		    T1n = FNMS(TZ, TY, TX * T10);
	       }
	       {
		    E TT, TV, TS, TU;
		    TT = ri[WS(ios, 1)];
		    TV = ii[WS(ios, 1)];
		    TS = W[0];
		    TU = W[1];
		    TW = FMA(TS, TT, TU * TV);
		    T1m = FNMS(TU, TT, TS * TV);
	       }
	       T1o = KP866025403 * (T1m - T1n);
	       T1D = KP866025403 * (T11 - TW);
	       T12 = TW + T11;
	       T1l = FNMS(KP500000000, T12, TR);
	       T1F = T1m + T1n;
	       T1G = FNMS(KP500000000, T1F, T1E);
	  }
	  {
	       E Ts, T1c, Tn, T1b;
	       {
		    E Tf, Th, Te, Tg;
		    Tf = ri[WS(ios, 6)];
		    Th = ii[WS(ios, 6)];
		    Te = W[10];
		    Tg = W[11];
		    Ti = FMA(Te, Tf, Tg * Th);
		    T1S = FNMS(Tg, Tf, Te * Th);
	       }
	       {
		    E Tp, Tr, To, Tq;
		    Tp = ri[WS(ios, 2)];
		    Tr = ii[WS(ios, 2)];
		    To = W[2];
		    Tq = W[3];
		    Ts = FMA(To, Tp, Tq * Tr);
		    T1c = FNMS(Tq, Tp, To * Tr);
	       }
	       {
		    E Tk, Tm, Tj, Tl;
		    Tk = ri[WS(ios, 10)];
		    Tm = ii[WS(ios, 10)];
		    Tj = W[18];
		    Tl = W[19];
		    Tn = FMA(Tj, Tk, Tl * Tm);
		    T1b = FNMS(Tl, Tk, Tj * Tm);
	       }
	       T1d = KP866025403 * (T1b - T1c);
	       T24 = KP866025403 * (Ts - Tn);
	       Tt = Tn + Ts;
	       T1a = FNMS(KP500000000, Tt, Ti);
	       T1T = T1b + T1c;
	       T25 = FNMS(KP500000000, T1T, T1S);
	  }
	  {
	       E TK, T1i, TF, T1h;
	       {
		    E Tx, Tz, Tw, Ty;
		    Tx = ri[WS(ios, 3)];
		    Tz = ii[WS(ios, 3)];
		    Tw = W[4];
		    Ty = W[5];
		    TA = FMA(Tw, Tx, Ty * Tz);
		    T1z = FNMS(Ty, Tx, Tw * Tz);
	       }
	       {
		    E TH, TJ, TG, TI;
		    TH = ri[WS(ios, 11)];
		    TJ = ii[WS(ios, 11)];
		    TG = W[20];
		    TI = W[21];
		    TK = FMA(TG, TH, TI * TJ);
		    T1i = FNMS(TI, TH, TG * TJ);
	       }
	       {
		    E TC, TE, TB, TD;
		    TC = ri[WS(ios, 7)];
		    TE = ii[WS(ios, 7)];
		    TB = W[12];
		    TD = W[13];
		    TF = FMA(TB, TC, TD * TE);
		    T1h = FNMS(TD, TC, TB * TE);
	       }
	       T1j = KP866025403 * (T1h - T1i);
	       T1y = KP866025403 * (TK - TF);
	       TL = TF + TK;
	       T1g = FNMS(KP500000000, TL, TA);
	       T1A = T1h + T1i;
	       T1B = FNMS(KP500000000, T1A, T1z);
	  }
	  {
	       E Tv, T1N, T1Y, T20, T14, T1Z, T1Q, T1R;
	       {
		    E Td, Tu, T1U, T1X;
		    Td = T1 + Tc;
		    Tu = Ti + Tt;
		    Tv = Td + Tu;
		    T1N = Td - Tu;
		    T1U = T1S + T1T;
		    T1X = T1V + T1W;
		    T1Y = T1U + T1X;
		    T20 = T1X - T1U;
	       }
	       {
		    E TM, T13, T1O, T1P;
		    TM = TA + TL;
		    T13 = TR + T12;
		    T14 = TM + T13;
		    T1Z = TM - T13;
		    T1O = T1z + T1A;
		    T1P = T1E + T1F;
		    T1Q = T1O - T1P;
		    T1R = T1O + T1P;
	       }
	       ri[WS(ios, 6)] = Tv - T14;
	       ii[WS(ios, 6)] = T1Y - T1R;
	       ri[0] = Tv + T14;
	       ii[0] = T1R + T1Y;
	       ri[WS(ios, 3)] = T1N - T1Q;
	       ii[WS(ios, 3)] = T1Z + T20;
	       ri[WS(ios, 9)] = T1N + T1Q;
	       ii[WS(ios, 9)] = T20 - T1Z;
	  }
	  {
	       E T1t, T1x, T27, T2a, T1w, T28, T1I, T29;
	       {
		    E T1r, T1s, T23, T26;
		    T1r = T15 + T18;
		    T1s = T1a + T1d;
		    T1t = T1r + T1s;
		    T1x = T1r - T1s;
		    T23 = T21 + T22;
		    T26 = T24 + T25;
		    T27 = T23 - T26;
		    T2a = T26 + T23;
	       }
	       {
		    E T1u, T1v, T1C, T1H;
		    T1u = T1g + T1j;
		    T1v = T1l + T1o;
		    T1w = T1u + T1v;
		    T28 = T1u - T1v;
		    T1C = T1y + T1B;
		    T1H = T1D + T1G;
		    T1I = T1C - T1H;
		    T29 = T1C + T1H;
	       }
	       ri[WS(ios, 10)] = T1t - T1w;
	       ii[WS(ios, 10)] = T2a - T29;
	       ri[WS(ios, 4)] = T1t + T1w;
	       ii[WS(ios, 4)] = T29 + T2a;
	       ri[WS(ios, 7)] = T1x - T1I;
	       ii[WS(ios, 7)] = T28 + T27;
	       ri[WS(ios, 1)] = T1x + T1I;
	       ii[WS(ios, 1)] = T27 - T28;
	  }
	  {
	       E T1f, T1J, T2d, T2f, T1q, T2g, T1M, T2e;
	       {
		    E T19, T1e, T2b, T2c;
		    T19 = T15 - T18;
		    T1e = T1a - T1d;
		    T1f = T19 + T1e;
		    T1J = T19 - T1e;
		    T2b = T25 - T24;
		    T2c = T22 - T21;
		    T2d = T2b + T2c;
		    T2f = T2c - T2b;
	       }
	       {
		    E T1k, T1p, T1K, T1L;
		    T1k = T1g - T1j;
		    T1p = T1l - T1o;
		    T1q = T1k + T1p;
		    T2g = T1k - T1p;
		    T1K = T1B - T1y;
		    T1L = T1G - T1D;
		    T1M = T1K - T1L;
		    T2e = T1K + T1L;
	       }
	       ri[WS(ios, 2)] = T1f - T1q;
	       ii[WS(ios, 2)] = T2d - T2e;
	       ri[WS(ios, 8)] = T1f + T1q;
	       ii[WS(ios, 8)] = T2e + T2d;
	       ri[WS(ios, 11)] = T1J - T1M;
	       ii[WS(ios, 11)] = T2g + T2f;
	       ri[WS(ios, 5)] = T1J + T1M;
	       ii[WS(ios, 5)] = T2f - T2g;
	  }
     }
     return W;
}

static const tw_instr twinstr[] = {
     {TW_FULL, 0, 12},
     {TW_NEXT, 1, 0}
};

static const ct_desc desc = { 12, "t1_12", twinstr, {88, 30, 30, 0}, &GENUS, 0, 0, 0 };

void X(codelet_t1_12) (planner *p) {
     X(kdft_dit_register) (p, t1_12, &desc);
}
