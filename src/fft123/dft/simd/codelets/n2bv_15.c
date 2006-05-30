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
/* Generated on Sat Jul  5 21:40:51 EDT 2003 */

#include "codelet-dft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_notw_c -simd -compact -variables 4 -sign 1 -n 15 -name n2bv_15 -with-ostride 2 -include n2b.h */

/*
 * This function contains 78 FP additions, 25 FP multiplications,
 * (or, 64 additions, 11 multiplications, 14 fused multiply/add),
 * 55 stack variables, and 30 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: n2bv_15.c,v 1.1 2005/07/26 17:36:59 glass Exp $
 * $Id: n2bv_15.c,v 1.1 2005/07/26 17:36:59 glass Exp $
 * $Id: n2bv_15.c,v 1.1 2005/07/26 17:36:59 glass Exp $
 */

#include "n2b.h"

static void n2bv_15(const R *ri, const R *ii, R *ro, R *io, stride is, stride os, int v, int ivs, int ovs)
{
     DVK(KP216506350, +0.216506350946109661690930792688234045867850657);
     DVK(KP509036960, +0.509036960455127183450980863393907648510733164);
     DVK(KP823639103, +0.823639103546331925877420039278190003029660514);
     DVK(KP951056516, +0.951056516295153572116439333379382143405698634);
     DVK(KP587785252, +0.587785252292473129168705954639072768597652438);
     DVK(KP250000000, +0.250000000000000000000000000000000000000000000);
     DVK(KP559016994, +0.559016994374947424102293417182819058860154590);
     DVK(KP866025403, +0.866025403784438646763723170752936183471402627);
     DVK(KP484122918, +0.484122918275927110647408174972799951354115213);
     DVK(KP500000000, +0.500000000000000000000000000000000000000000000);
     int i;
     const R *xi;
     R *xo;
     xi = ii;
     xo = io;
     BEGIN_SIMD();
     for (i = v; i > 0; i = i - VL, xi = xi + (VL * ivs), xo = xo + (VL * ovs)) {
	  V Ti, T11, TH, Ts, TL, TM, Tz, TC, TD, TI, T12, T13, T14, T15, T16;
	  V T17, Tf, Tj, TZ, T10;
	  {
	       V TF, Tg, Th, TG;
	       TF = LD(&(xi[0]), ivs, &(xi[0]));
	       Tg = LD(&(xi[WS(is, 5)]), ivs, &(xi[WS(is, 1)]));
	       Th = LD(&(xi[WS(is, 10)]), ivs, &(xi[0]));
	       TG = VADD(Tg, Th);
	       Ti = VSUB(Tg, Th);
	       T11 = VADD(TF, TG);
	       TH = VFNMS(LDK(KP500000000), TG, TF);
	  }
	  {
	       V Tm, Tn, T3, To, Tw, Tx, Td, Ty, Tp, Tq, T6, Tr, Tt, Tu, Ta;
	       V Tv, T7, Te;
	       {
		    V T1, T2, Tb, Tc;
		    Tm = LD(&(xi[WS(is, 3)]), ivs, &(xi[WS(is, 1)]));
		    T1 = LD(&(xi[WS(is, 8)]), ivs, &(xi[0]));
		    T2 = LD(&(xi[WS(is, 13)]), ivs, &(xi[WS(is, 1)]));
		    Tn = VADD(T1, T2);
		    T3 = VSUB(T1, T2);
		    To = VFNMS(LDK(KP500000000), Tn, Tm);
		    Tw = LD(&(xi[WS(is, 9)]), ivs, &(xi[WS(is, 1)]));
		    Tb = LD(&(xi[WS(is, 14)]), ivs, &(xi[0]));
		    Tc = LD(&(xi[WS(is, 4)]), ivs, &(xi[0]));
		    Tx = VADD(Tb, Tc);
		    Td = VSUB(Tb, Tc);
		    Ty = VFNMS(LDK(KP500000000), Tx, Tw);
	       }
	       {
		    V T4, T5, T8, T9;
		    Tp = LD(&(xi[WS(is, 12)]), ivs, &(xi[0]));
		    T4 = LD(&(xi[WS(is, 2)]), ivs, &(xi[0]));
		    T5 = LD(&(xi[WS(is, 7)]), ivs, &(xi[WS(is, 1)]));
		    Tq = VADD(T4, T5);
		    T6 = VSUB(T4, T5);
		    Tr = VFNMS(LDK(KP500000000), Tq, Tp);
		    Tt = LD(&(xi[WS(is, 6)]), ivs, &(xi[0]));
		    T8 = LD(&(xi[WS(is, 11)]), ivs, &(xi[WS(is, 1)]));
		    T9 = LD(&(xi[WS(is, 1)]), ivs, &(xi[WS(is, 1)]));
		    Tu = VADD(T8, T9);
		    Ta = VSUB(T8, T9);
		    Tv = VFNMS(LDK(KP500000000), Tu, Tt);
	       }
	       Ts = VSUB(To, Tr);
	       TL = VSUB(T3, T6);
	       TM = VSUB(Ta, Td);
	       Tz = VSUB(Tv, Ty);
	       TC = VADD(To, Tr);
	       TD = VADD(Tv, Ty);
	       TI = VADD(TC, TD);
	       T12 = VADD(Tm, Tn);
	       T13 = VADD(Tp, Tq);
	       T14 = VADD(T12, T13);
	       T15 = VADD(Tt, Tu);
	       T16 = VADD(Tw, Tx);
	       T17 = VADD(T15, T16);
	       T7 = VADD(T3, T6);
	       Te = VADD(Ta, Td);
	       Tf = VMUL(LDK(KP484122918), VSUB(T7, Te));
	       Tj = VADD(T7, Te);
	  }
	  TZ = VADD(TH, TI);
	  T10 = VBYI(VMUL(LDK(KP866025403), VADD(Ti, Tj)));
	  ST(&(xo[10]), VSUB(TZ, T10), ovs, &(xo[2]));
	  ST(&(xo[20]), VADD(T10, TZ), ovs, &(xo[0]));
	  {
	       V T1a, T18, T19, T1e, T1f, T1c, T1d, T1g, T1b;
	       T1a = VMUL(LDK(KP559016994), VSUB(T14, T17));
	       T18 = VADD(T14, T17);
	       T19 = VFNMS(LDK(KP250000000), T18, T11);
	       T1c = VSUB(T12, T13);
	       T1d = VSUB(T15, T16);
	       T1e = VBYI(VFNMS(LDK(KP951056516), T1d, VMUL(LDK(KP587785252), T1c)));
	       T1f = VBYI(VFMA(LDK(KP951056516), T1c, VMUL(LDK(KP587785252), T1d)));
	       ST(&(xo[0]), VADD(T11, T18), ovs, &(xo[0]));
	       T1g = VADD(T1a, T19);
	       ST(&(xo[12]), VADD(T1f, T1g), ovs, &(xo[0]));
	       ST(&(xo[18]), VSUB(T1g, T1f), ovs, &(xo[2]));
	       T1b = VSUB(T19, T1a);
	       ST(&(xo[6]), VSUB(T1b, T1e), ovs, &(xo[2]));
	       ST(&(xo[24]), VADD(T1e, T1b), ovs, &(xo[0]));
	  }
	  {
	       V TA, TN, TU, TS, Tl, TR, TK, TV, Tk, TE, TJ;
	       TA = VFMA(LDK(KP951056516), Ts, VMUL(LDK(KP587785252), Tz));
	       TN = VFMA(LDK(KP823639103), TL, VMUL(LDK(KP509036960), TM));
	       TU = VFNMS(LDK(KP823639103), TM, VMUL(LDK(KP509036960), TL));
	       TS = VFNMS(LDK(KP951056516), Tz, VMUL(LDK(KP587785252), Ts));
	       Tk = VFNMS(LDK(KP216506350), Tj, VMUL(LDK(KP866025403), Ti));
	       Tl = VADD(Tf, Tk);
	       TR = VSUB(Tf, Tk);
	       TE = VMUL(LDK(KP559016994), VSUB(TC, TD));
	       TJ = VFNMS(LDK(KP250000000), TI, TH);
	       TK = VADD(TE, TJ);
	       TV = VSUB(TJ, TE);
	       {
		    V TB, TO, TX, TY;
		    TB = VBYI(VADD(Tl, TA));
		    TO = VSUB(TK, TN);
		    ST(&(xo[2]), VADD(TB, TO), ovs, &(xo[2]));
		    ST(&(xo[28]), VSUB(TO, TB), ovs, &(xo[0]));
		    TX = VBYI(VSUB(TS, TR));
		    TY = VSUB(TV, TU);
		    ST(&(xo[14]), VADD(TX, TY), ovs, &(xo[2]));
		    ST(&(xo[16]), VSUB(TY, TX), ovs, &(xo[0]));
	       }
	       {
		    V TP, TQ, TT, TW;
		    TP = VBYI(VSUB(Tl, TA));
		    TQ = VADD(TN, TK);
		    ST(&(xo[8]), VADD(TP, TQ), ovs, &(xo[0]));
		    ST(&(xo[22]), VSUB(TQ, TP), ovs, &(xo[2]));
		    TT = VBYI(VADD(TR, TS));
		    TW = VADD(TU, TV);
		    ST(&(xo[4]), VADD(TT, TW), ovs, &(xo[0]));
		    ST(&(xo[26]), VSUB(TW, TT), ovs, &(xo[2]));
	       }
	  }
     }
     END_SIMD();
}

static const kdft_desc desc = { 15, "n2bv_15", {64, 11, 14, 0}, &GENUS, 0, 2, 0, 0 };
void X(codelet_n2bv_15) (planner *p) {
     X(kdft_register) (p, n2bv_15, &desc);
}
