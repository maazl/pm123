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

#define DLG_FRONTEND    110
#define DLG_CONFIG      120
#define DLG_DECONV      130
#define DLG_GENERATE    140
#define DLG_GENERATE_X  141
#define DLG_MEASURE     150
#define DLG_MEASURE_X   151
#define DLG_CALIBRATE   160
#define DLG_CALIBRATE_X 161

// Generic definitions
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
#define CB_FIRORDER     1022
#define CB_FFTSIZE      1023
#define EF_FREQ_BIN     1030
#define EF_FREQ_FACTOR  1031

#define RB_VIEWTARGET   1040
#define RB_VIEWGAIN     1041
#define RB_VIEWDELAY    1042
#define RB_VIEWTIME     1043

#define NB_FRONTEND     1090

// Configuration
#define EF_WORKDIR      1100
#define PB_BROWSE       1101
#define EF_RECURI       1103
#define PB_CURRENT      1104

// Deconvolution
#define CB_ENABLE       1160
#define LB_KERNEL       1162
#define RB_WIN_NONE     1165
#define RB_WIN_DIMMED_HAMMING 1166
#define RB_WIN_HAMMING  1167

// Generate
#define GB_RESULT       1205
#define GB_RESULT2      1206

#define EF_LIMITGAIN    1250
#define EF_LIMITGAINRATE 1251
#define CB_INVERTGAIN   1252
#define EF_LIMITDELAY   1260
#define EF_LIMITDELAYRATE 1261
#define CB_CLIPINVERT   1265
#define EF_NORM_LOW     1270
#define EF_NORM_HIGH    1271
#define RB_ENERGY       1275
#define RB_LOGARITHM    1276
#define CB_SUBSONIC     1280
#define CB_SUPERSONIC   1281

// Open loop
#define BX_LEFT         1300
#define BX_RIGHT        1301
#define PB_TEST         1304
#define BX_TEST         1305
#define SB_VOLUME       1308

// Open loop extended settings
#define EF_FREQ_LOW     1350
#define EF_FREQ_HIGH    1351
#define EF_GAIN_LOW     1352
#define EF_GAIN_HIGH    1353
#define EF_DELAY_LOW    1354
#define EF_DELAY_HIGH   1355
#define EF_GAIN2_LOW    1356
#define EF_GAIN2_HIGH   1357
#define EF_VU_LOW       1358
#define EF_VU_YELLOW    1359
#define EF_VU_RED       1360
#define EF_DISCARD      1361
#define EF_REFEXPONENT  1363
#define EF_REFFDIST     1364
#define RB_WHITE_N      1365
#define RB_PINK_N       1366
#define RB_BROWN_N      1367
#define CB_SKIPEVEN     1368

// Measure
#define RB_NOISE        1400
#define RB_SWEEP        1401
#define RB_CH_BOTH      1410
#define RB_CH_LEFT      1411
#define RB_CH_RIGHT     1412
#define CB_DIFFOUT      1415
#define CB_REFIN        1420

#define CB_CAL_FILE     1430
#define ST_CAL_DESC     1431
#define CB_MIC_FILE     1432
#define ST_MIC_DESC     1433

// Calibrate
#define RB_STEREO_LOOP  1500
#define RB_LEFT_LOOP    1501
#define RB_RIGHT_LOOP   1502
#define RB_BOTH_LOOP    1503


void save_config();

#endif
