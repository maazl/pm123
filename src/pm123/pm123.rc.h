/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
 *
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
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

#define ICO_MAIN              1
#define ICO_MP3               1700
#define ICO_MP3USED           1701
#define ICO_MP3ACTIVE         1702
#define ICO_MP3PLAY           1703
#define ICO_MP3INVLD          1708
#define ICO_MP3WAIT           1709
#define ICO_PLEMPTY           1710
#define ICO_PLCLOSE           1711
#define ICO_PLOPEN            1712
#define ICO_PLRECURSIVE       1713
#define ICO_PLACTIVE          1720
#define ICO_PLCLOSEACTIVE     1721
#define ICO_PLOPENACTIVE      1722
#define ICO_PLRECURSIVEACTIVE 1723
#define ICO_PLPLAY            1730
#define ICO_PLCLOSEPLAY       1731
#define ICO_PLOPENPLAY        1732
#define ICO_PLRECURSIVEPLAY   1733

#define WIN_MAIN              1
#define HLP_MAIN              1
#define ACL_MAIN              1

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
#define IDM_M_MANAGER       540
#define IDM_M_SAVE          542
#define IDM_M_BOOKMARKS     544
#define IDM_M_ADDBOOK       545
#define IDM_M_EDITBOOK      546
#define IDM_M_PLAYBACK      547
#define IDM_M_VOL_RAISE     548
#define IDM_M_VOL_LOWER     549
#define IDM_M_MENU          550

#define IDM_M_LOAD          600
#define IDM_M_LOADFILE      601
#define IDM_M_URL           602
#define IDM_M_LOADOTHER     603 /* reserve some ID's for several plug-ins.  */

#define IDM_M_LAST        10000 /* A lot of IDs after this need to be free. */
#define IDM_M_LAST_E      14999
//#define IDM_M_BOOKMARKS   11000 /* A lot of IDs after this need to be free. */
//#define IDM_M_BOOKMARKS_E 11999
#define IDM_M_PLUG        15000 /* A lot of IDs after this need to be free. */

#define MNU_SUBFOLDER      1700

#define DLG_URL            2014
#define ENT_URL             101

#define DLG_FILE           2100
#define CB_RECURSE          500
#define CB_RELATIV          501

#define DLG_EQUALIZER      2015

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

#define MSG_PLAY              1
#define MSG_STOP              2
#define MSG_PAUSE             3
#define MSG_FWD               4
#define MSG_REW               5
#define MSG_JUMP              6
#define MSG_SAVE              7

/* amp_load_playable options */
#define AMP_LOAD_NOT_PLAY    0x0001 // Load a playable object, but do not start playback automatically
#define AMP_LOAD_NOT_RECALL  0x0002 // Load a playable object, but do not add an entry into the list of recent files
#define AMP_LOAD_KEEP_PLAYLIST 0x0004 // Play a playable object. If A playlist containing this item is loaded, the item is activated only.

/* amp_add_* options */
#define URL_ADD_TO_PLAYER    0x0000
#define URL_ADD_TO_LIST      0x0001

/* amp_save_list_as options */
#define SAV_LST_PLAYLIST     0x0000
#define SAV_M3U_PLAYLIST     0x0001

/* amp_invalidate options */
#define UPD_TIMERS           0x0001
#define UPD_FILEINFO         0x0002
#define UPD_DELAYED          0x8000
#define UPD_ALL              0x7FFF

/* file dialog additional flags */
#define FDU_DIR_ENABLE       0x0001
#define FDU_RECURSEBTN       0x0002
#define FDU_RECURSE_ON       0x0004
#define FDU_RELATIVBTN       0x0008
#define FDU_RELATIV_ON       0x0010

/* file dialog standard types */
#define FDT_PLAYLIST         "Playlist files (*.LST;*.MPL;*.M3U;*.PLS)"
#define FDT_PLAYLIST_LST     "Playlist files (*.LST)"
#define FDT_PLAYLIST_M3U     "Playlist files (*.M3U)"
#define FDT_AUDIO            "All supported audio files ("
#define FDT_AUDIO_ALL        "All supported types (*.LST;*.MPL;*.M3U;*.PLS;"
#define FDT_SKIN             "Skin files (*.SKN)"
#define FDT_EQUALIZER        "Equalizer presets (*.EQ)"
#define FDT_PLUGIN           "Plug-in (*.DLL)"

#ifndef DC_PREPAREITEM
#define DC_PREPAREITEM  0x0040
#endif

/* Playlist */
#define DLG_PLAYLIST         42
#define CNR_PLAYLIST FID_CLIENT // TODO: remove
#define ACL_PLAYLIST       8001
#define MNU_PLAYLIST        900
#define MNU_RECORD          901
#define MNU_PLRECORD        902
/* Playlistmanager */
#define DLG_PM               48

#define PM_MAIN_MENU       1024
#define PM_FILE_MENU       2048
#define PM_LIST_MENU       3072

#define IDM_PL_USEALL       910
#define IDM_PL_USE          911
#define IDM_PL_NAVIGATE     912
#define IDM_PL_DETAILED     913
#define IDM_PL_DETAILEDALL  914
#define IDM_PL_TREEVIEW     915
#define IDM_PL_TREEVIEWALL  916
#define IDM_PL_REMOVE       917
#define IDM_PL_CLEAR        918
#define IDM_PL_CLEARALL     919
#define IDM_PL_RELOAD       920
#define IDM_PL_REFRESH      921
#define IDM_PL_EDIT         922
#define IDM_PL_SORT         930
#define IDM_PL_SORT_SIZE    931
#define IDM_PL_SORT_PLTIME  932
#define IDM_PL_SORT_NAME    933
#define IDM_PL_SORT_SONG    934
#define IDM_PL_SORT_ALIAS   935
#define IDM_PL_SORT_RANDOM  936
#define IDM_PL_APPEND      1100
#define IDM_PL_APPFILE     1101
#define IDM_PL_APPURL      1102
#define IDM_PL_APPOTHER    1103 /* Need some IDs for plugin extensions.     */
#define IDM_PL_OPEN         940
#define IDM_PL_OPENLAST    1000 /* A lot of IDs after this need to be free. */
#define IDM_PL_SAVE         960
#define IDM_PL_MENU         970 /* invoke main menu */
#define IDM_PL_MENUCONT     971 /* invoke record menu */ 

/* Bookmark */
#define DLG_BM_ADD           500
#define ST_BM_DESC           501
#define EF_BM_DESC           502

/* SKIN */
/* Special resources that control PM123 interface. */

#define UL_SHADE_BRIGHT       1 /* Bright 3D-shade color.                             */
#define UL_SHADE_DARK         2 /* Dark 3D-shade color.                               */
#define UL_SLIDER_BRIGHT      3 /* Bright color of seek slider (obsolete).            */
#define UL_SLIDER_COLOR       4 /* Color of seek slider border.                       */
#define UL_PL_COLOR           5 /* Playlist position indicator color.                 */
#define UL_SHADE_STAT         6 /* Disable 3D-shading of the statistics area.         */
#define UL_SHADE_VOLUME       7 /* Disable 3D-shading of the volume bar.              */
#define UL_DISPLAY_MSG        8 /* A string to be displayed on skin load (obsolete).  */
#define UL_SHADE_PLAYER       9 /* Disable 3D-shading of the player window.           */
#define UL_SHADE_SLIDER      10 /* Disable the seek slider border.                    */
#define UL_ONE_FONT          11 /* Disable the second font.                           */
#define UL_TIMER_SEPSPACE    12 /* Width of the main timer separator (obsolete).      */
#define UL_IN_PIXELS         13 /* Measure width of the filename display in pixels.   */
#define UL_R_MSG_HEIGHT      14 /* Height of the filename display for regular mode.   */
#define UL_S_MSG_HEIGHT      15 /* Height of the filename display for small mode.     */
#define UL_FG_MSG_COLOR      16 /* Foreground color of the filename display.          */
#define UL_R_MSG_LEN         20 /* Width of the filename display for regular mode.    */
#define UL_SLIDER_WIDTH      21 /* Width the seek slider area in pixels.              */
#define UL_S_MSG_LEN         22 /* Width of the filename display for small mode.      */
#define UL_FONT              23 /* Initial font, 0 or 1.                              */
#define UL_TIMER_SPACE       24 /* Space between the main timer digits (in pixels).   */
#define UL_TIMER_SEPARATE    25 /* Disable separator between the main timer groups.   */
#define UL_VOLUME_HRZ        26 /* Make volume bar horizontal.                        */
#define UL_VOLUME_SLIDER     27 /* Give volume bar a handle you can grab.             */
#define UL_BPS_DIGITS        28 /* Draw bitrates with digits from resource 1830-1839. */
#define UL_PL_INDEX          29 /* Draw playlist indicator with resources 1660-1669.  */
#define UL_BUNDLE            30 /* The bundle file for this skin.                     */

/* Bitmap identifiers for action buttons. */

#define BMP_PLAY           1300 /* Play button down for regular mode.                 */
#define BMP_PAUSE          1302 /* Pause button down for regular mode.                */
#define BMP_REW            1303 /* Rewind button down for regular mode.               */
#define BMP_FWD            1304 /* Fast forward button down for regular mode.         */
#define BMP_POWER          1305 /* Power button down for regular mode.                */
#define BMP_PREV           1306 /* Previous button down for regular mode.             */
#define BMP_NEXT           1307 /* Next button down for regular mode.                 */
#define BMP_SHUFFLE        1308 /* Shuffle button down for regular mode.              */
#define BMP_REPEAT         1309 /* Repeat button down for regular mode.               */
#define BMP_PL             1321 /* Playlist button down for regular mode.             */
#define BMP_STOP           1322 /* Stop button down for regular mode.                 */
#define BMP_FLOAD          1324 /* Load file button down for regular mode.            */

#define BMP_N_PLAY         1310 /* Play button up for regular mode.                   */
#define BMP_N_PAUSE        1312 /* Pause button up for regular mode.                  */
#define BMP_N_REW          1313 /* Rewind button up for regular mode.                 */
#define BMP_N_FWD          1314 /* Fast forward button up for regular mode.           */
#define BMP_N_POWER        1315 /* Power button up for regular mode.                  */
#define BMP_N_PREV         1316 /* Previous button up for regular mode.               */
#define BMP_N_NEXT         1317 /* Next button up for regular mode.                   */
#define BMP_N_SHUFFLE      1318 /* Shuffle button up for regular mode.                */
#define BMP_N_REPEAT       1319 /* Repeat button up for regular mode.                 */
#define BMP_N_PL           1320 /* Playlist button up for regular mode.               */
#define BMP_N_STOP         1323 /* Stop button up for regular mode.                   */
#define BMP_N_FLOAD        1325 /* Load file button up for regular mode.              */

#define BMP_S_PLAY         5500 /* Play button down for small and tiny modes.         */
#define BMP_S_PAUSE        5502 /* Pause button down for small and tiny modes.        */
#define BMP_S_REW          5503 /* Rewind button down for small and tiny modes.       */
#define BMP_S_FWD          5504 /* Fast forward button down for small and tiny modes. */
#define BMP_S_POWER        5505 /* Power button down for small and tiny modes.        */
#define BMP_S_PREV         5506 /* Previous button down for small and tiny modes.     */
#define BMP_S_NEXT         5507 /* Next button down for small and tiny modes.         */
#define BMP_S_SHUFFLE      5508 /* Shuffle button down for small and tiny modes.      */
#define BMP_S_REPEAT       5509 /* Repeat button down for small and tiny modes.       */
#define BMP_S_PL           5521 /* Playlist button down for small and tiny modes.     */
#define BMP_S_STOP         5523 /* Stop button down for small and tiny modes.         */
#define BMP_S_FLOAD        5524 /* Load file button down for small and tiny modes.    */

#define BMP_SN_PLAY        5510 /* Play button up for small and tiny modes.           */
#define BMP_SN_PAUSE       5512 /* Pause button up for small and tiny modes.          */
#define BMP_SN_REW         5513 /* Rewind button up for small and tiny modes.         */
#define BMP_SN_FWD         5514 /* Fast forward button up for small and tiny modes.   */
#define BMP_SN_POWER       5515 /* Power button up for small and tiny modes.          */
#define BMP_SN_PREV        5516 /* Previous button up for small and tiny modes.       */
#define BMP_SN_NEXT        5517 /* Next button up for small and tiny modes.           */
#define BMP_SN_SHUFFLE     5518 /* Shuffle button up for small and tiny modes.        */
#define BMP_SN_REPEAT      5519 /* Repeat button up for small and tiny modes.         */
#define BMP_SN_PL          5520 /* Playlist button up for small and tiny modes.       */
#define BMP_SN_STOP        5522 /* Stop button up for small and tiny modes.           */
#define BMP_SN_FLOAD       5525 /* Load file button down for small and tiny modes.    */

#define BMP_KILL           1600
#define BMP_LED            1820 /* Bright led (displayed when PM123 has focus).       */
#define BMP_N_LED          1821 /* Dark   led (displayed when PM123 is not focused).  */
#define BMP_FONT1          1400 /* Font 1.                                            */
#define BMP_FONT2          4400 /* Font 2.                                            */
#define BMP_R_BGROUND      1900 /* Background bitmap for regular mode.                */
#define BMP_S_BGROUND      1611 /* Background bitmap for small mode.                  */
#define BMP_T_BGROUND      1612 /* Background bitmap for tiny mode.                   */
#define BMP_SLIDER         1610 /* Seek slider handle.                                */
#define BMP_SLIDER_SHAFT   1906 /* Slider shaft (drawn before drawing slider handle). */
#define BMP_NO_CHANNELS    1608 /* No channels (unknown, no file loaded) mode.        */
#define BMP_STEREO         1601 /* Stereo mode.                                       */
#define BMP_MONO           1602 /* Mono mode.                                         */
#define BMP_VOLSLIDER      1620 /* Volume slider handle bitmap (only if enabled).     */
#define BMP_VOLBAR         1630 /* Volume bar.                                        */
#define BMP_SINGLEPLAY     1631 /* Indicator for single file play.                    */
#define BMP_LISTPLAY       1632 /* Indicator for playlist play.                       */
#define BMP_NOFILE         1633 /* Indicator for no file loaded.                      */
#define BMP_BPS            1800 /* Bitmaps for bitrates 0 (no bitrate), 32, 48, 56,   */
                                /* 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256 */
                                /* Not used if resource UL_BPS_DIGITS is enabled.     */
#define BMP_NOTL           1901 /* "Time left", dark.                                 */
#define BMP_TL             1902 /* "Time left", bright.                               */
#define BMP_NOPLIST        1903 /* "Playlist left", dark.                             */
#define BMP_PLIST          1904 /* "Playlist left", bright.                           */

#define DIG_SMALL          1100
#define DIG_BIG            1200 /* Digits 0-9 for the main ("big") timer.             */
#define DIG_TINY           1640 /* Digits 0-9 for time and playlist left timers.      */
#define DIG_PL_INDEX       1660 /* Digits 0-9 for playlist total/index displays.      */
#define DIG_BPS            1830 /* Digits 0-9 for bitrates.                           */

/* Resource (bitmap) position settings. */

#define POS_TIMER             1 /* Main timer.                                        */
#define POS_R_SIZE            2 /* Main window size (sx, sy) for regular mode.        */
#define POS_R_PLAY            3 /* Play button for regular mode.                      */
#define POS_R_PAUSE           4 /* Pause button for regular mode.                     */
#define POS_R_REW             5 /* Rewind button for regular mode.                    */
#define POS_R_FWD             6 /* Fast forward button for regular mode.              */
#define POS_R_PL              7 /* Playlist button for regular mode.                  */
#define POS_R_REPEAT          8 /* Repeat button for regular mode.                    */
#define POS_R_SHUFFLE         9 /* Shuffle button for regular mode.                   */
#define POS_R_PREV           10 /* Previous button for regular mode.                  */
#define POS_R_NEXT           11 /* Next button for regular mode.                      */
#define POS_R_POWER          12 /* Power button for regular mode.                     */
#define POS_R_TEXT           13 /* Text display for regular mode.                     */
#define POS_S_TEXT           14 /* Text display for small mode.                       */
#define POS_NOTL             15 /* "Time left", dark.                                 */
#define POS_TL               16 /* "Time left", bright.                               */
#define POS_NOPLIST          17 /* "Playlist left", dark.                             */
#define POS_PLIST            18 /* "Playlist left", bright.                           */
#define POS_TIME_LEFT        19 /* Time left timer.                                   */
#define POS_PL_LEFT          20 /* Playlist left timer.                               */
#define POS_PL_MODE          21 /* Playmode indicator (no file/single/playlist).      */
#define POS_LED              22 /* Bright led (displayed when PM123 has focus).       */
#define POS_N_LED            23 /* Dark led (displayed when PM123 is not focused).    */
#define POS_SLIDER           24 /* Seek slider.                                       */
#define POS_VOLBAR           25 /* Volume bar.                                        */
#define POS_NO_CHANNELS      26 /* No channels indicator.                             */
#define POS_MONO             27 /* Mono indicator.                                    */
#define POS_STEREO           28 /* Stereo indicator.                                  */
#define POS_BPS              29 /* Bitrate indicator.                                 */
#define POS_S_SIZE           30 /* Main window size (sx, sy) for small mode.          */
#define POS_T_SIZE           31 /* Main window size (sx, sy) for tiny mode.           */
#define POS_S_PLAY           33 /* Play button for small mode.                        */
#define POS_S_PAUSE          34 /* Pause button for small mode.                       */
#define POS_S_REW            35 /* Rewind button for small mode.                      */
#define POS_S_FWD            36 /* Fast forward button for small mode.                */
#define POS_S_PL             37 /* Playlist button for small mode.                    */
#define POS_S_REPEAT         38 /* Repeat button for small mode.                      */
#define POS_S_SHUFFLE        39 /* Shuffle button for small mode.                     */
#define POS_S_PREV           40 /* Previous button for small mode.                    */
#define POS_S_NEXT           41 /* Next button for small mode.                        */
#define POS_S_POWER          42 /* Power button for small mode.                       */
#define POS_T_PLAY           53 /* Play button for tiny mode.                         */
#define POS_T_PAUSE          54 /* Pause button for tiny mode.                        */
#define POS_T_REW            55 /* Rewind button for tiny mode.                       */
#define POS_T_FWD            56 /* Fast forward button for tiny mode.                 */
#define POS_T_PL             57 /* Playlist button for tiny mode.                     */
#define POS_T_REPEAT         58 /* Repeat button for tiny mode.                       */
#define POS_T_SHUFFLE        59 /* Shuffle button for tiny mode.                      */
#define POS_T_PREV           60 /* Previous button for tiny mode.                     */
#define POS_T_NEXT           61 /* Next button for tiny mode.                         */
#define POS_T_POWER          62 /* Power button for tiny mode.                        */
#define POS_PL_INDEX         63 /* Playlist index indicator (1 of 2)                  */
#define POS_PL_TOTAL         64 /* Playlist index indicator (1 of 2)                  */
#define POS_R_STOP           65 /* Stop button for regular mode.                      */
#define POS_R_FLOAD          66 /* Load file button for regular mode.                 */
#define POS_SLIDER_SHAFT     67 /* Location for the slider shaft (bitmap 1906).       */
#define POS_S_STOP           68 /* Stop button for small mode.                        */
#define POS_S_FLOAD          69 /* Load file button for small mode.                   */
#define POS_T_STOP           70 /* Stop button for tiny mode.                         */
#define POS_T_FLOAD          71 /* Load file button for tiny mode.                    */
#define POS_VOLSLIDER        72 /* Offset of the volume slider concerning a bar.      */

/* Properties */
#define DLG_CONFIG         2010
#define NB_CONFIG           100
#define PB_UNDO             101
#define PB_DEFAULT          102
#define PB_HELP             103

#define CFG_PAGE1          3000
#define GB_BEHAVIOUR       3001
#define CB_PLAYONLOAD      3010
#define CB_AUTOUSEPL       3020
#define CB_AUTOPLAYPL      3030
#define CB_SELECTPLAYED    3031
#define CB_TRASHONSEEK     3040
#define CB_DOCK            3150
#define EF_DOCK            3151
#define ST_PIXELS          3152
#define GB_STREAMING       3160
#define ST_PROXY_HOST      3170
#define EF_PROXY_HOST      3171
#define ST_PROXY_PORT      3172
#define EF_PROXY_PORT      3173
#define ST_PROXY_USER      3180
#define EF_PROXY_USER      3181
#define ST_PROXY_PASS      3182
#define EF_PROXY_PASS      3183
#define ST_BUFFERSIZE      3185
#define SB_BUFFERSIZE      3190
#define ST_KB              3195
#define CB_FILLBUFFER      3200

#define CFG_PAGE2          3500
#define GB_TITLE           3501
#define ST_SCROLL          3545
#define RB_SCROLL_INFINITE 3550
#define RB_SCROLL_ONCE     3551
#define RB_SCROLL_DISABLE  3552
#define ST_DISPLAY         3575
#define RB_DISP_FILENAME   3580
#define RB_DISP_ID3TAG     3581
#define RB_DISP_FILEINFO   3582
#define GB_FONT            3600
#define CB_USE_SKIN_FONT   3610
#define ST_FONT_SAMPLE     3630
#define PB_FONT_SELECT     3640

#define CFG_PAGE3          4000
#define GB_VISUALPLUGINS   4001
#define LB_VISPLUG         4010
#define PB_VIS_ENABLE      4020
#define PB_VIS_UNLOAD      4030
#define ST_VIS_AUTHOR      4040
#define ST_VIS_DESC        4050
#define PB_VIS_CONFIG      4060
#define PB_VIS_ADD         4070
#define GB_DECODERPLUGINS  4071
#define LB_DECPLUG         4080
#define PB_DEC_ENABLE      4090
#define PB_DEC_UNLOAD      4100
#define ST_DEC_AUTHOR      4110
#define ST_DEC_DESC        4120
#define PB_DEC_CONFIG      4130
#define PB_DEC_ADD         4140

#define CFG_PAGE4          5000
#define GB_FILPLUG         5005
#define LB_FILPLUG         5010
#define PB_FIL_ENABLE      5020
#define PB_FIL_UNLOAD      5030
#define ST_FIL_AUTHOR      5040
#define ST_FIL_DESC        5050
#define PB_FIL_CONFIG      5060
#define PB_FIL_ADD         5070
#define GB_OUTPLUG         5075
#define LB_OUTPLUG         5080
#define PB_OUT_ACTIVATE    5090
#define PB_OUT_UNLOAD      5100
#define ST_OUT_AUTHOR      5110
#define ST_OUT_DESC        5120
#define PB_OUT_CONFIG      5130
#define PB_OUT_ADD         5140

#define CFG_ABOUT          2011
#define ST_TITLE1          2020
#define ST_TITLE2          2030
#define ST_BUILT           2040
#define GB_AUTHORS         2050
#define ST_AUTHORS         2060
#define GB_CREDITS         2070
#define ST_CREDITS         2080


#endif
