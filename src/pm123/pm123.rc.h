/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2007-2008 M.Mueller
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

#ifndef  PM123_RC_H
#define  PM123_RC_H

/* ID ranges:
 *
 *     0- 1999   PM123 core
 **  500-  699   PM123 main menu
 **  800-  899   PM123 Plalist/Playlist Manager misc
 **  900-  999   PM123 Plalist/Playlist Manager menu
 ** 1000- 1099   PM123 Help table
 ** 1100- 1199   PM123 File, URL, EQ dialog
 ** 1200- 1799   PM123 properties dialog
 ** 1800- 1899   PM123 info dialog
 *  2000- 2999   PM123 Skin IDs
 ** 2000- 2099   PM123 Skinned buttons
 ** 2100- 2199   PM123 Skin misc
 ** 2200- 2399   PM123 Skin digits
 ** 2400- 2799   PM123 Skin fonts
 * 10000-19999   PM123 Bookmark browser
 */

#define WIN_MAIN              1
#define HLP_MAIN              1
#define ACL_MAIN              WIN_MAIN // neccessarily identical
#define ICO_MAIN              WIN_MAIN // neccessarily identical

#define MNU_MAIN            500
#define IDM_M_SHUFFLE       505
#define IDM_M_ABOUT         506
#define IDM_M_CFG           507
#define IDM_M_PLAYLIST      509
#define IDM_M_MINIMIZE      510
#define IDM_M_SMALL         511
#define IDM_M_TINY          512
#define IDM_M_NORMAL        513
#define IDM_M_FONT1         515
#define IDM_M_FONT2         516
#define IDM_M_FONT          517
#define IDM_M_SIZE          518
#define IDM_M_SKIN          519
#define IDM_M_SKINLOAD      520
#define IDM_M_FLOAT         521
#define IDM_M_EQUALIZE      522
#define IDM_M_TAG           523
#define IDM_M_INFO          524
#define IDM_M_PLINFO        525
#define IDM_M_MANAGER       540
#define IDM_M_SAVE          542
#define IDM_M_BOOKMARKS     544
#define IDM_M_ADDBOOK       545
#define IDM_M_EDITBOOK      546
#define IDM_M_PLAYBACK      547
#define IDM_M_VOL_RAISE     548
#define IDM_M_VOL_LOWER     549
#define IDM_M_MENU          550
#define IDM_M_CURRENT_SONG  551
#define IDM_M_DETAILED      552
#define IDM_M_TREEVIEW      553
#define IDM_M_ADDPLBOOK     554
#define IDM_M_ADDBOOK_TIME  555
#define IDM_M_PLSAVE        556
#define IDM_M_ADDPMBOOK     557
#define IDM_M_CURRENT_PL    558
#define IDM_M_ADDPLBOOK_TIME 559
#define IDM_M_LOAD          560
#define IDM_M_LOADFILE      600
#define IDM_M_URL           601
#define IDM_M_LOADOTHER     602 /* reserve some ID's for several plug-ins.  */
#define IDM_M_PLUG          650 /* reserve some ID's for several plug-ins.  */
#define IDM_M_PLUG_E        699

#define IDM_M_LAST        10000 /* A lot of IDs after this need to be free. */
#define IDM_M_LAST_E      19999

#define MNU_SUBFOLDER       800

/* Playlist */
#define DLG_PLAYLIST         42
/* Playlistmanager */
#define DLG_PM               48

#define MNU_PLAYLIST        810
#define MNU_RECORD          811
#define ACL_PLAYLIST        812
#define PM_MAIN_MENU        820
#define PM_REC_MENU         821
//#define ACL_PLAYLISTMAN     822

#define ICO_MP3             850
#define ICO_MP3USED         851
#define ICO_MP3ACTIVE       852
#define ICO_MP3PLAY         853
#define ICO_MP3INVLD        858
#define ICO_MP3WAIT         859
#define ICO_PLEMPTY         860
#define ICO_PLCLOSE         861
#define ICO_PLOPEN          862
#define ICO_PLRECURSIVE     863
#define ICO_PLACTIVE        870
#define ICO_PLCLOSEACTIVE   871
#define ICO_PLOPENACTIVE    872
#define ICO_PLRECURSIVEACTIVE 873
#define ICO_PLPLAY          880
#define ICO_PLCLOSEPLAY     881
#define ICO_PLOPENPLAY      882
#define ICO_PLRECURSIVEPLAY 883

#define IDM_PL_USE          900
#define IDM_PL_USEALL       901
#define IDM_PL_NAVIGATE     902
#define IDM_PL_DETAILED     903
#define IDM_PL_DETAILEDALL  904
#define IDM_PL_TREEVIEW     905
#define IDM_PL_TREEVIEWALL  906
#define IDM_PL_REMOVE       907
#define IDM_PL_CLEAR        908
#define IDM_PL_CLEARALL     909
#define IDM_PL_RELOAD       910
#define IDM_PL_REFRESH      911
#define IDM_PL_EDIT         912
#define IDM_PL_SAVE         913
#define IDM_PL_INFO         914
#define IDM_PL_INFOALL      915
#define IDM_PL_MENU         918 /* invoke main menu */
#define IDM_PL_MENUCONT     919 /* invoke record menu */ 
#define IDM_PL_SORT         920
#define IDM_PL_SORT_SIZE    921
#define IDM_PL_SORT_TIME    922
#define IDM_PL_SORT_URL     923
#define IDM_PL_SORT_SONG    924
#define IDM_PL_SORT_ALIAS   925
#define IDM_PL_SORT_ART     926
#define IDM_PL_SORT_ALBUM   927
#define IDM_PL_SORT_RAND    929
#define IDM_PL_SORT_SIZEALL 931
#define IDM_PL_SORT_TIMEALL 932
#define IDM_PL_SORT_URLALL  933
#define IDM_PL_SORT_SONGALL 934
#define IDM_PL_SORT_ALIASALL 935
#define IDM_PL_SORT_ARTALL  936
#define IDM_PL_SORT_ALBUMALL 937
#define IDM_PL_SORT_RANDALL 939
#define IDM_PL_APPEND       940
#define IDM_PL_APPFILE      941
#define IDM_PL_APPURL       942
#define IDM_PL_APPOTHER     943 /* Need some IDs for plugin extensions.     */
#define IDM_PL_APPENDALL    960
#define IDM_PL_APPFILEALL   961
#define IDM_PL_APPURLALL    962
#define IDM_PL_APPOTHERALL  963 /* Need some IDs for plugin extensions.     */
#define IDM_PL_OPEN         980
#define IDM_PL_OPENLAST     981 /* A lot of IDs after this need to be free. */


/* Helptable */
#define HLP_MAIN_TABLE      100
#define HLP_NULL_TABLE      101

#define IDH_MAIN           1000
#define IDH_ADVANTAGES     1001
#define IDH_ANALYZER       1002
#define IDH_SUPPORT        1003
#define IDH_COPYRIGHT      1004
//#define IDH_DRAG_AND_DROP  1005
#define IDH_EQUALIZER      1006
#define IDH_ID3_EDITOR     1007
#define IDH_INTERFACE      1008
#define IDH_MAIN_MENU      1009
#define IDH_MAIN_WINDOW    1010
#define IDH_PM             1011
#define IDH_NETSCAPE       1012
#define IDH_COMMANDLINE    1013
#define IDH_PL             1014
#define IDH_PROPERTIES     1015
#define IDH_REMOTE         1016
#define IDH_SKIN_GUIDE     1017
#define IDH_SKIN_UTILITY   1018
#define IDH_TROUBLES       1019
#define IDH_PLAYLISTFORMAT 1020
#define IDH_MIGRATE1_40    1021
#define IDH_LOCATION       1022

/* Dialogs */

/* File */
#define DLG_FILE           1100
#define CB_RECURSE         1101
#define CB_RELATIV         1102

/* URL */
#define DLG_URL            1110
#define ENT_URL            1111

/* Bookmark */
#define DLG_BM_ADD         1120
#define ST_BM_DESC         1121
#define EF_BM_DESC         1122

#define DLG_EQUALIZER      1150
#define ST_EQ_PREAMP       1151
#define ST_EQ_PREM12       1152
#define ST_EQ_PREP12       1153
#define SL_EQ_PREAMP       1154
#define ST_EQ_MUTE         1155
#define ST_EQ_M12DB        1156
#define ST_EQ_P12DB        1157
#define ST_EQ_0DB          1158
#define SL_EQ_0            1160 /* Slider 0..9 */
#define ST_EQ_0            1170 /* Text 0..9 */
#define CB_EQ_0            1180 /* Mute button 0..9 */
#define CB_EQ_ENABLED      1190
#define BT_EQ_DEFAULT      1191
#define BT_EQ_LOAD         1192
#define BT_EQ_SAVE         1193

/* Properties */
#define DLG_CONFIG         1200
#define NB_CONFIG          1201
#define PB_UNDO            1202
#define PB_DEFAULT         1203
#define PB_HELP            1204

#define CFG_ABOUT          1210
#define ST_TITLE1          1211
#define ST_TITLE2          1212
#define ST_BUILT           1213
#define GB_AUTHORS         1214
#define ST_AUTHORS         1215
#define GB_CREDITS         1216
#define ST_CREDITS         1217

#define CFG_SETTINGS1      1300
#define GB_BEHAVIOUR       1301
#define CB_PLAYONLOAD      1302
#define CB_TRASHONSCAN     1306
#define CB_AUTOUSEPL       1351
#define CB_AUTOPLAYPL      1352
#define CB_RECURSEDND      1353
#define CB_AUTOAPPENDDND   1354
#define CB_AUTOAPPENDCMD   1355
#define CB_QUEUEMODE       1356

#define CFG_SETTINGS2      1350
#define GB_GUIFEATURES     1351
#define CB_SELECTPLAYED    1355
#define CB_DOCK            1360
#define EF_DOCK            1361
#define ST_PIXELS          1362
#define GB_STREAMING       1370
#define ST_PROXY_HOST      1371
#define EF_PROXY_HOST      1372
#define ST_PROXY_PORT      1373
#define EF_PROXY_PORT      1374
#define ST_PROXY_USER      1375
#define EF_PROXY_USER      1376
#define ST_PROXY_PASS      1377
#define EF_PROXY_PASS      1378
#define ST_BUFFERSIZE      1379
#define SB_BUFFERSIZE      1380
#define ST_KB              1381
#define CB_FILLBUFFER      1382
#define SB_FILLBUFFER      1383
#define ST_FILLBUFFER      1384
#define GB_REMOTE          1390
#define ST_PIPE            1391
#define EF_PIPE            1392

#define CFG_DISPLAY1       1400
#define GB_TITLE           1401
#define ST_SCROLL          1410
#define RB_SCROLL_INFINITE 1411
#define RB_SCROLL_ONCE     1412
#define RB_SCROLL_DISABLE  1413
#define ST_DISPLAY         1420
#define RB_DISP_FILENAME   1421
#define RB_DISP_ID3TAG     1422
#define RB_DISP_FILEINFO   1423
#define GB_FONT            1430
#define CB_USE_SKIN_FONT   1431
#define ST_FONT_SAMPLE     1432
#define PB_FONT_SELECT     1433

#define CFG_CONFIG1        1500
#define GB_VISUALPLUGINS   1510
#define LB_VISPLUG         1511
#define PB_VIS_ENABLE      1512
#define PB_VIS_UNLOAD      1513
#define ST_VIS_AUTHOR      1514
#define ST_VIS_DESC        1515
#define PB_VIS_CONFIG      1516
#define PB_VIS_ADD         1517
#define GB_DECODERPLUGINS  1520
#define LB_DECPLUG         1521
#define PB_DEC_ENABLE      1522
#define PB_DEC_UNLOAD      1523
#define ST_DEC_AUTHOR      1524
#define ST_DEC_DESC        1525
#define PB_DEC_CONFIG      1526
#define PB_DEC_ADD         1527

#define CFG_CONFIG2        1550
#define GB_FILPLUG         1560
#define LB_FILPLUG         1561
#define PB_FIL_ENABLE      1562
#define PB_FIL_UNLOAD      1563
#define ST_FIL_AUTHOR      1564
#define ST_FIL_DESC        1565
#define PB_FIL_CONFIG      1566
#define PB_FIL_ADD         1567
#define GB_OUTPLUG         1570
#define LB_OUTPLUG         1571
#define PB_OUT_ACTIVATE    1572
#define PB_OUT_UNLOAD      1573
#define ST_OUT_AUTHOR      1574
#define ST_OUT_DESC        1575
#define PB_OUT_CONFIG      1576
#define PB_OUT_ADD         1577

/* Object Info */
#define DLG_INFO           1800
#define NB_INFO            1802

#define CFG_TECHINFO       1810
#define ST_FILESIZE        1811
#define EF_FILESIZE        1812
#define ST_TOTALTIME       1813
#define EF_TOTALTIME       1814
#define ST_BITRATE         1815
#define EF_BITRATE         1816
#define ST_NUMITEMS        1817
#define EF_NUMITEMS        1818
#define CB_ITEMSRECURSIVE  1819
#define ST_SAMPLERATE      1820
#define EF_SAMPLERATE      1821
#define ST_NUMCHANNELS     1822
#define EF_NUMCHANNELS     1823
#define ST_DECODER         1824
#define EF_DECODER         1825
#define ST_INFOSTRINGS     1826
#define EF_INFOSTRINGS     1827

#define CFG_METAINFO       1830
#define ST_METATITLE       1831
#define EF_METATITLE       1832 
#define ST_METAARTIST      1833
#define EF_METAARTIST      1834
#define ST_METAALBUM       1835
#define EF_METAALBUM       1836
#define ST_METATRACK       1837
#define EF_METATRACK       1838
#define ST_METADATE        1839
#define EF_METADATE        1840
#define ST_METAGENRE       1841
#define EF_METAGENRE       1842
#define ST_METACOMMENT     1843
#define EF_METACOMMENT     1844
#define ST_METARPGAIN      1845
#define EF_METARPGAINT     1846
#define ST_METARPGAINT     1847
#define EF_METARPGAINA     1848
#define ST_METARPGAINA     1849
#define ST_METARPPEAK      1850
#define EF_METARPPEAKT     1851
#define ST_METARPPEAKT     1852
#define EF_METARPPEAKA     1853
#define ST_METARPPEAKA     1854


/* SKINs ... */

/* Bitmap identifiers for action buttons. */
#define BMP_PLAY           2000 /* Play button down for regular mode.                 */
#define BMP_PAUSE          2001 /* Pause button down for regular mode.                */
#define BMP_REW            2002 /* Rewind button down for regular mode.               */
#define BMP_FWD            2003 /* Fast forward button down for regular mode.         */
#define BMP_POWER          2004 /* Power button down for regular mode.                */
#define BMP_PREV           2005 /* Previous button down for regular mode.             */
#define BMP_NEXT           2006 /* Next button down for regular mode.                 */
#define BMP_SHUFFLE        2007 /* Shuffle button down for regular mode.              */
#define BMP_REPEAT         2008 /* Repeat button down for regular mode.               */
#define BMP_PL             2009 /* Playlist button down for regular mode.             */
#define BMP_STOP           2010 /* Stop button down for regular mode.                 */
#define BMP_FLOAD          2011 /* Load file button down for regular mode.            */

#define BMP_N_PLAY         2020 /* Play button up for regular mode.                   */
#define BMP_N_PAUSE        2021 /* Pause button up for regular mode.                  */
#define BMP_N_REW          2022 /* Rewind button up for regular mode.                 */
#define BMP_N_FWD          2023 /* Fast forward button up for regular mode.           */
#define BMP_N_POWER        2024 /* Power button up for regular mode.                  */
#define BMP_N_PREV         2025 /* Previous button up for regular mode.               */
#define BMP_N_NEXT         2026 /* Next button up for regular mode.                   */
#define BMP_N_SHUFFLE      2027 /* Shuffle button up for regular mode.                */
#define BMP_N_REPEAT       2028 /* Repeat button up for regular mode.                 */
#define BMP_N_PL           2029 /* Playlist button up for regular mode.               */
#define BMP_N_STOP         2030 /* Stop button up for regular mode.                   */
#define BMP_N_FLOAD        2031 /* Load file button up for regular mode.              */

#define BMP_S_PLAY         2040 /* Play button down for small and tiny modes.         */
#define BMP_S_PAUSE        2041 /* Pause button down for small and tiny modes.        */
#define BMP_S_REW          2042 /* Rewind button down for small and tiny modes.       */
#define BMP_S_FWD          2043 /* Fast forward button down for small and tiny modes. */
#define BMP_S_POWER        2044 /* Power button down for small and tiny modes.        */
#define BMP_S_PREV         2045 /* Previous button down for small and tiny modes.     */
#define BMP_S_NEXT         2046 /* Next button down for small and tiny modes.         */
#define BMP_S_SHUFFLE      2047 /* Shuffle button down for small and tiny modes.      */
#define BMP_S_REPEAT       2048 /* Repeat button down for small and tiny modes.       */
#define BMP_S_PL           2049 /* Playlist button down for small and tiny modes.     */
#define BMP_S_STOP         2050 /* Stop button down for small and tiny modes.         */
#define BMP_S_FLOAD        2051 /* Load file button down for small and tiny modes.    */

#define BMP_SN_PLAY        2060 /* Play button up for small and tiny modes.           */
#define BMP_SN_PAUSE       2061 /* Pause button up for small and tiny modes.          */
#define BMP_SN_REW         2062 /* Rewind button up for small and tiny modes.         */
#define BMP_SN_FWD         2063 /* Fast forward button up for small and tiny modes.   */
#define BMP_SN_POWER       2064 /* Power button up for small and tiny modes.          */
#define BMP_SN_PREV        2065 /* Previous button up for small and tiny modes.       */
#define BMP_SN_NEXT        2066 /* Next button up for small and tiny modes.           */
#define BMP_SN_SHUFFLE     2067 /* Shuffle button up for small and tiny modes.        */
#define BMP_SN_REPEAT      2068 /* Repeat button up for small and tiny modes.         */
#define BMP_SN_PL          2069 /* Playlist button up for small and tiny modes.       */
#define BMP_SN_STOP        2070 /* Stop button up for small and tiny modes.           */
#define BMP_SN_FLOAD       2071 /* Load file button down for small and tiny modes.    */

#define BMP_LED            2100 /* Bright led (displayed when PM123 has focus).       */
#define BMP_N_LED          2101 /* Dark   led (displayed when PM123 is not focused).  */
#define BMP_R_BGROUND      2104 /* Background bitmap for regular mode.                */
#define BMP_S_BGROUND      2105 /* Background bitmap for small mode.                  */
#define BMP_T_BGROUND      2106 /* Background bitmap for tiny mode.                   */
#define BMP_SLIDER         2107 /* Seek slider handle.                                */
#define BMP_SLIDER_SHAFT   2108 /* Slider shaft (drawn before drawing slider handle). */
#define BMP_NO_CHANNELS    2109 /* No channels (unknown, no file loaded) mode.        */
#define BMP_STEREO         2110 /* Stereo mode.                                       */
#define BMP_MONO           2111 /* Mono mode.                                         */
#define BMP_VOLSLIDER      2112 /* Volume slider handle bitmap (only if enabled).     */
#define BMP_VOLBAR         2113 /* Volume bar.                                        */
#define BMP_SINGLEPLAY     2114 /* Indicator for single file play.                    */
#define BMP_LISTPLAY       2115 /* Indicator for playlist play.                       */
#define BMP_NOFILE         2116 /* Indicator for no file loaded.                      */
#define BMP_NOTL           2117 /* "Time left", dark.                                 */
#define BMP_TL             2118 /* "Time left", bright.                               */
#define BMP_NOPLIST        2119 /* "Playlist left", dark.                             */
#define BMP_PLIST          2120 /* "Playlist left", bright.                           */
#define BMP_BPS            2180 /* Bitmaps for bitrates 0 (no bitrate), 32, 48, 56,   */
                                /* 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256 */
                                /* Not used if resource UL_BPS_DIGITS is enabled.     */

#define DIG_SMALL          2200 /* Digits 0-9 for ???                                 */
#define DIG_BIG            2220 /* Digits 0-9 for the main ("big") timer.             */
#define DIG_TINY           2240 /* Digits 0-9 for time and playlist left timers.      */
#define DIG_PL_INDEX       2260 /* Digits 0-9 for playlist total/index displays.      */
#define DIG_BPS            2280 /* Digits 0-9 for bitrates.                           */

#define BMP_FONT1          2400 /* Font 1.                                            */
#define BMP_FONT2          2600 /* Font 2.                                            */


#endif
