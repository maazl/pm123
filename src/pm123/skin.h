/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp„  <rosmo@sektori.com>
 *
 * Copyright 2004-2005 Dmitry A.Steklenev <glass@ptv.ru>
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

#ifndef PM123_SKIN_H
#define PM123_SKIN_H

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

typedef struct _BMPBUTTON
{
  HWND  handle;           /* Button window handle.                          */
  int   id_r_pressed;     /* Pressed state bitmap for regular mode.         */
  int   id_r_release;     /* Release state bitmap for regular mode.         */
  int   id_r_pos;         /* Button position for regular mode.              */
  int   id_s_pressed;     /* Pressed state bitmap for small and tiny modes. */
  int   id_s_release;     /* Release state bitmap for small and tiny modes. */
  int   id_s_pos;         /* Button position for small mode.                */
  int   id_t_pos;         /* Button position for tiny mode.                 */
  int   state;            /* Button state.                                  */
  BOOL  sticky;           /* Is this a sticky button.                       */
  char* help;             /* Button description.                            */

} BMPBUTTON, *PBMPBUTTON;

extern BMPBUTTON btn_play;    /* Play button      */
extern BMPBUTTON btn_stop;    /* Stop button      */
extern BMPBUTTON btn_pause;   /* Pause button     */
extern BMPBUTTON btn_rew;     /* Rewind button    */
extern BMPBUTTON btn_fwd;     /* Forward button   */
extern BMPBUTTON btn_power;   /* Power button     */
extern BMPBUTTON btn_prev;    /* Prev button      */
extern BMPBUTTON btn_next;    /* Next button      */
extern BMPBUTTON btn_shuffle; /* Shuffle button   */
extern BMPBUTTON btn_repeat;  /* Repeat button    */
extern BMPBUTTON btn_pl;      /* Playlist button  */
extern BMPBUTTON btn_fload;   /* Load a file      */

#ifdef __cplusplus
extern "C" {
#endif

/* Creates and loads a bitmap from a file, and returns the bitmap handle. */
HBITMAP bmp_load_bitmap( const char* filename );
/* Draws a bit map using the current image colors and mixes. */
void bmp_draw_bitmap( HPS hps, int x, int y, int res );
/* Draws a specified digit using the specified size. */
void bmp_draw_digit( HPS hps, int x, int y, int digit, int size );
/* Draws a 3D shade of the specified area. */
void bmp_draw_shade( HPS hps, int x, int y, int cx, int cy, long clr1, long clr2 );

/* Returns a width of the specified bitmap. */
int  bmp_cx( int id );
/* Returns a height of the specified bitmap. */
int  bmp_cy( int id );

/* Draws a activation led. */
void bmp_draw_led( HPS hps, int active );
/* Draws the player background. */
void bmp_draw_background( HPS hps, HWND hwnd );
/* Draws the specified part of the player background. */
void bmp_draw_part_bg( HPS hps, int x1, int y1, int x2, int y2 );
/* Draws the main player timer. */
void bmp_draw_timer( HPS hps, double time );
/* Draws the tiny player timer. */
void bmp_draw_tiny_timer( HPS hps, int pos_id, double time );
/* Draws the channels indicator. */
void bmp_draw_channels( HPS hps, int channels );
/* Draws the volume bar and volume slider. */
void bmp_draw_volume( HPS hps, int volume );
/* Draws the file bitrate. */
void bmp_draw_rate( HPS hps, int rate );
/* Draws the current playlist index. */
void bmp_draw_plind( HPS hps, int index, int total );
/* Draws the current playlist mode. */
void bmp_draw_plmode( HPS hps );
/* Draws the current position slider. */
void bmp_draw_slider( HPS hps, double played, double total );
/* Draws a current displayed text using the current selected font. */
void bmp_draw_text( HPS hps );
/* Draws the time left and playlist left labels. */
void bmp_draw_timeleft( HPS hps );

/* Sets the new displayed text. */
void bmp_set_text( const char* string );
/* Scrolls the current selected text. */
BOOL bmp_scroll_text( void );
/* Returns a pointer to the current selected text. */
const char* bmp_query_text( void );

/* Queries whether a point lies within a volume bar rectangle. */
BOOL bmp_pt_in_volume( POINTL pos );
/* Queries whether a point lies within a current displayed text rectangle. */
BOOL bmp_pt_in_text( POINTL pos );
/* Queries whether a point lies within a position slider rectangle. */
BOOL bmp_pt_in_slider( POINTL pos );

/* Calculates a volume level on the basis of position of the pointer. */
int  bmp_calc_volume( POINTL pos );
/* Calculates a current seeking time on the basis of position of the pointer. */
double bmp_calc_time( POINTL pos, double total );

/* Deallocates all resources used by current loaded skin. */
void bmp_clean_skin( void );
/* Loads specified skin. */
BOOL bmp_load_skin( const char *filename, HAB hab, HWND hplayer, HPS hps );
/* Adjusts current skin to the selected size of the player window. */
void bmp_reflow_and_resize( HWND hframe );

/* Returns TRUE if specified mode supported by current skin. */
BOOL bmp_is_mode_supported( int mode );
/* Returns TRUE if specified font supported by current skin. */
BOOL bmp_is_font_supported( int font );

#ifdef __cplusplus
}
#endif
#endif /* PM123_SKIN_H */
