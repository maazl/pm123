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
/* Generated on Sat Jul  5 21:56:56 EDT 2003 */

#include "codelet-rdft.h"

/* Generated by: /homee/stevenj/cvs/fftw3.0.1/genfft/gen_hc2hc -compact -variables 4 -n 7 -dit -name hf_7 -include hf.h */

/*
 * This function contains 72 FP additions, 60 FP multiplications,
 * (or, 36 additions, 24 multiplications, 36 fused multiply/add),
 * 29 stack variables, and 28 memory accesses
 */
/*
 * Generator Id's : 
 * $Id: hf_7.c,v 1.1 2005/07/26 17:37:08 glass Exp $
 * $Id: hf_7.c,v 1.1 2005/07/26 17:37:08 glass Exp $
 * $Id: hf_7.c,v 1.1 2005/07/26 17:37:08 glass Exp $
 */

#include "hf.h"

static const R *hf_7(R *rio, R *iio, const R *W, stride ios, int m, int dist)
{
     DK(KP222520933, +0.222520933956314404288902564496794759466355569);
     DK(KP900968867, +0.900968867902419126236102319507445051165919162);
     DK(KP623489801, +0.623489801858733530525004884004239810632274731);
     DK(KP433883739, +0.433883739117558120475768332848358754609990728);
     DK(KP781831482, +0.781831482468029808708444526674057750232334519);
     DK(KP974927912, +0.974927912181823607018131682993931217232785801);
     int i;
     for (i = m - 2; i > 0; i = i - 2, rio = rio + dist, iio = iio - dist, W = W + 12) {
	  E T1, Tc, TS, TC, TO, TR, Tn, TT, TI, TP, Ty, TU, TF, TQ;
	  T1 = rio[0];
	  {
	       E T6, TA, Tb, TB;
	       {
		    E T3, T5, T2, T4;
		    T3 = rio[WS(ios, 1)];
		    T5 = iio[-WS(ios, 5)];
		    T2 = W[0];
		    T4 = W[1];
		    T6 = FMA(T2, T3, T4 * T5);
		    TA = FNMS(T4, T3, T2 * T5);
	       }
	       {
		    E T8, Ta, T7, T9;
		    T8 = rio[WS(ios, 6)];
		    Ta = iio[0];
		    T7 = W[10];
		    T9 = W[11];
		    Tb = FMA(T7, T8, T9 * Ta);
		    TB = FNMS(T9, T8, T7 * Ta);
	       }
	       Tc = T6 + Tb;
	       TS = Tb - T6;
	       TC = TA - TB;
	       TO = TA + TB;
	  }
	  TR = iio[-WS(ios, 6)];
	  {
	       E Th, TG, Tm, TH;
	       {
		    E Te, Tg, Td, Tf;
		    Te = rio[WS(ios, 2)];
		    Tg = iio[-WS(ios, 4)];
		    Td = W[2];
		    Tf = W[3];
		    Th = FMA(Td, Te, Tf * Tg);
		    TG = FNMS(Tf, Te, Td * Tg);
	       }
	       {
		    E Tj, Tl, Ti, Tk;
		    Tj = rio[WS(ios, 5)];
		    Tl = iio[-WS(ios, 1)];
		    Ti = W[8];
		    Tk = W[9];
		    Tm = FMA(Ti, Tj, Tk * Tl);
		    TH = FNMS(Tk, Tj, Ti * Tl);
	       }
	       Tn = Th + Tm;
	       TT = Tm - Th;
	       TI = TG - TH;
	       TP = TG + TH;
	  }
	  {
	       E Ts, TD, Tx, TE;
	       {
		    E Tp, Tr, To, Tq;
		    Tp = rio[WS(ios, 3)];
		    Tr = iio[-WS(ios, 3)];
		    To = W[4];
		    Tq = W[5];
		    Ts = FMA(To, Tp, Tq * Tr);
		    TD = FNMS(Tq, Tp, To * Tr);
	       }
	       {
		    E Tu, Tw, Tt, Tv;
		    Tu = rio[WS(ios, 4)];
		    Tw = iio[-WS(ios, 2)];
		    Tt = W[6];
		    Tv = W[7];
		    Tx = FMA(Tt, Tu, Tv * Tw);
		    TE = FNMS(Tv, Tu, Tt * Tw);
	       }
	       Ty = Ts + Tx;
	       TU = Tx - Ts;
	       TF = TD - TE;
	       TQ = TD + TE;
	  }
	  rio[0] = T1 + Tc + Tn + Ty;
	  iio[0] = TO + TP + TQ + TR;
	  {
	       E TJ, Tz, TX, TY;
	       TJ = FNMS(KP781831482, TF, KP974927912 * TC) - (KP433883739 * TI);
	       Tz = FMA(KP623489801, Ty, T1) + FNMA(KP900968867, Tn, KP222520933 * Tc);
	       iio[-WS(ios, 5)] = Tz - TJ;
	       rio[WS(ios, 2)] = Tz + TJ;
	       TX = FNMS(KP781831482, TU, KP974927912 * TS) - (KP433883739 * TT);
	       TY = FMA(KP623489801, TQ, TR) + FNMA(KP900968867, TP, KP222520933 * TO);
	       rio[WS(ios, 5)] = TX - TY;
	       iio[-WS(ios, 2)] = TX + TY;
	  }
	  {
	       E TL, TK, TV, TW;
	       TL = FMA(KP781831482, TC, KP974927912 * TI) + (KP433883739 * TF);
	       TK = FMA(KP623489801, Tc, T1) + FNMA(KP900968867, Ty, KP222520933 * Tn);
	       iio[-WS(ios, 6)] = TK - TL;
	       rio[WS(ios, 1)] = TK + TL;
	       TV = FMA(KP781831482, TS, KP974927912 * TT) + (KP433883739 * TU);
	       TW = FMA(KP623489801, TO, TR) + FNMA(KP900968867, TQ, KP222520933 * TP);
	       rio[WS(ios, 6)] = TV - TW;
	       iio[-WS(ios, 1)] = TV + TW;
	  }
	  {
	       E TN, TM, TZ, T10;
	       TN = FMA(KP433883739, TC, KP974927912 * TF) - (KP781831482 * TI);
	       TM = FMA(KP623489801, Tn, T1) + FNMA(KP222520933, Ty, KP900968867 * Tc);
	       iio[-WS(ios, 4)] = TM - TN;
	       rio[WS(ios, 3)] = TM + TN;
	       TZ = FMA(KP433883739, TS, KP974927912 * TU) - (KP781831482 * TT);
	       T10 = FMA(KP623489801, TP, TR) + FNMA(KP222520933, TQ, KP900968867 * TO);
	       rio[WS(ios, 4)] = TZ - T10;
	       iio[-WS(ios, 3)] = TZ + T10;
	  }
     }
     return W;
}

static const tw_instr twinstr[] = {
     {TW_FULL, 0, 7},
     {TW_NEXT, 1, 0}
};

static const hc2hc_desc desc = { 7, "hf_7", twinstr, {36, 24, 36, 0}, &GENUS, 0, 0, 0 };

void X(codelet_hf_7) (planner *p) {
     X(khc2hc_dit_register) (p, hf_7, &desc);
}
