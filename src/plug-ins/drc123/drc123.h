/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *    3. The name of the author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef DRC123_H
#define DRC123_H

#define DLG_FRONTEND    101
#define DLG_CONFIG      102
#define DLG_DECONV      103
#define DLG_GENERATE    104
#define DLG_MEASURE     105
#define DLG_CALIBRATE   106

#define ST_GENERIC      1000
#define GB_GENERIC      1001
#define ST_DESCR        1002
#define ML_DESCR        1003
#define EF_FILE         1004
#define PB_HELP         1010
#define PB_START        1011
#define PB_STOP         1012
#define PB_LOAD         1013
#define PB_CLEAR        1014
#define PB_SAVE         1015
#define PB_APPLY        1016
#define PB_UNDO         1017
#define PB_DEFAULT      1018
#define PB_RELOAD       1019
#define CC_RESULT       1020
#define CC_RESULT2      1021

#define NB_FRONTEND     1090

#define EF_WORKDIR      1100
#define PB_BROWSE       1101
#define EF_RECURI       1103
#define PB_CURRENT      1104
#define CB_ENABLE       1120
#define LB_KERNEL       1122
#define CB_FIRORDER     1125
#define RB_WIN_NONE     1130
#define RB_WIN_DIMMED_HAMMING 1131
#define RB_WIN_HAMMING  1132

#define RB_NOISE        1200
#define RB_SWEEP        1201
#define RB_WHITE_N      1205
#define RB_INTERMED     1206
#define RB_PINK_N       1207
#define RB_CH_BOTH      1210
#define RB_CH_LEFT      1211
#define RB_CH_RIGHT     1212
#define CB_DIFF         1215

#define CB_USE_CAL      1220
#define PB_CAL_SELECT   1221
#define ST_CAL_DESC     1222
#define CB_DYN_CAL      1223

#define BX_LEFT         1230
#define BX_RIGHT        1231
#define PB_TEST         1234
#define BX_TEST         1235
#define SB_VOLUME       1238

#define RB_STEREO_LOOP  1300
#define RB_MONO_LOOP    1301
#define RB_LEFT_LOOP    1302
#define RB_RIGHT_LOOP   1303


#endif

