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

#define INCL_WIN
#include "playable.h"
#include <os2.h>

/* SKIN */

/* Resource (bitmap) position settings. */
enum
{ POS_TIMER = 1,       /* Main timer.                                        */
  POS_R_SIZE,          /* Main window size (sx, sy) for regular mode.        */
  POS_R_PLAY,          /* Play button for regular mode.                      */
  POS_R_PAUSE,         /* Pause button for regular mode.                     */
  POS_R_REW,           /* Rewind button for regular mode.                    */
  POS_R_FWD,           /* Fast forward button for regular mode.              */
  POS_R_PL,            /* Playlist button for regular mode.                  */
  POS_R_REPEAT,        /* Repeat button for regular mode.                    */
  POS_R_SHUFFLE,       /* Shuffle button for regular mode.                   */
  POS_R_PREV,          /* Previous button for regular mode.                  */
  POS_R_NEXT,          /* Next button for regular mode.                      */
  POS_R_POWER,         /* Power button for regular mode.                     */
  POS_R_TEXT,          /* Text display for regular mode.                     */
  POS_S_TEXT,          /* Text display for small mode.                       */
  POS_NOTL,            /* "Time left", dark.                                 */
  POS_TL,              /* "Time left", bright.                               */
  POS_NOPLIST,         /* "Playlist left", dark.                             */
  POS_PLIST,           /* "Playlist left", bright.                           */
  POS_TIME_LEFT,       /* Time left timer.                                   */
  POS_PL_LEFT,         /* Playlist left timer.                               */
  POS_PL_MODE,         /* Playmode indicator (no file/single/playlist).      */
  POS_LED,             /* Bright led (displayed when PM123 has focus).       */
  POS_N_LED,           /* Dark led (displayed when PM123 is not focused).    */
  POS_SLIDER,          /* Seek slider.                                       */
  POS_VOLBAR,          /* Volume bar.                                        */
  POS_NO_CHANNELS,     /* No channels indicator.                             */
  POS_MONO,            /* Mono indicator.                                    */
  POS_STEREO,          /* Stereo indicator.                                  */
  POS_BPS,             /* Bitrate indicator.                                 */
  POS_S_SIZE,          /* Main window size (sx, sy) for small mode.          */
  POS_T_SIZE,          /* Main window size (sx, sy) for tiny mode.           */
  POS_S_PLAY = 33,     /* Play button for small mode.                        */
  POS_S_PAUSE,         /* Pause button for small mode.                       */
  POS_S_REW,           /* Rewind button for small mode.                      */
  POS_S_FWD,           /* Fast forward button for small mode.                */
  POS_S_PL,            /* Playlist button for small mode.                    */
  POS_S_REPEAT,        /* Repeat button for small mode.                      */
  POS_S_SHUFFLE,       /* Shuffle button for small mode.                     */
  POS_S_PREV,          /* Previous button for small mode.                    */
  POS_S_NEXT,          /* Next button for small mode.                        */
  POS_S_POWER,         /* Power button for small mode.                       */
  POS_T_PLAY = 53,     /* Play button for tiny mode.                         */
  POS_T_PAUSE,         /* Pause button for tiny mode.                        */
  POS_T_REW,           /* Rewind button for tiny mode.                       */
  POS_T_FWD,           /* Fast forward button for tiny mode.                 */
  POS_T_PL,            /* Playlist button for tiny mode.                     */
  POS_T_REPEAT,        /* Repeat button for tiny mode.                       */
  POS_T_SHUFFLE,       /* Shuffle button for tiny mode.                      */
  POS_T_PREV,          /* Previous button for tiny mode.                     */
  POS_T_NEXT,          /* Next button for tiny mode.                         */
  POS_T_POWER,         /* Power button for tiny mode.                        */
  POS_PL_INDEX,        /* Playlist index indicator (1 of 2)                  */
  POS_PL_TOTAL,        /* Playlist index indicator (1 of 2)                  */
  POS_R_STOP,          /* Stop button for regular mode.                      */
  POS_R_FLOAD,         /* Load file button for regular mode.                 */
  POS_SLIDER_SHAFT,    /* Location for the slider shaft (bitmap 1906).       */
  POS_S_STOP,          /* Stop button for small mode.                        */
  POS_S_FLOAD,         /* Load file button for small mode.                   */
  POS_T_STOP,          /* Stop button for tiny mode.                         */
  POS_T_FLOAD,         /* Load file button for tiny mode.                    */
  POS_VOLSLIDER,       /* Offset of the volume slider concerning a bar.      */
  POS__MAX
};

/* Special resources that control PM123 interface. */
enum
{ UL_SHADE_BRIGHT = 1, /* Bright 3D-shade color.                             */
  UL_SHADE_DARK,       /* Dark 3D-shade color.                               */
  UL_SLIDER_BRIGHT,    /* Bright color of seek slider (obsolete).            */
  UL_SLIDER_COLOR,     /* Color of seek slider border.                       */
  UL_PL_COLOR,         /* Playlist position indicator color.                 */
  UL_SHADE_STAT,       /* Disable 3D-shading of the statistics area.         */
  UL_SHADE_VOLUME,     /* Disable 3D-shading of the volume bar.              */
  UL_DISPLAY_MSG,      /* A string to be displayed on skin load (obsolete).  */
  UL_SHADE_PLAYER,     /* Disable 3D-shading of the player window.           */
  UL_SHADE_SLIDER,     /* Disable the seek slider border.                    */
  UL_ONE_FONT,         /* Disable the second font.                           */
  UL_TIMER_SEPSPACE,   /* Width of the main timer separator (obsolete).      */
  UL_IN_PIXELS,        /* Measure width of the filename display in pixels.   */
  UL_R_MSG_HEIGHT,     /* Height of the filename display for regular mode.   */
  UL_S_MSG_HEIGHT,     /* Height of the filename display for small mode.     */
  UL_FG_MSG_COLOR,     /* Foreground color of the filename display.          */
  UL_R_MSG_LEN = 20,   /* Width of the filename display for regular mode.    */
  UL_SLIDER_WIDTH,     /* Width the seek slider area in pixels.              */
  UL_S_MSG_LEN,        /* Width of the filename display for small mode.      */
  UL_FONT,             /* Initial font, 0 or 1.                              */
  UL_TIMER_SPACE,      /* Space between the main timer digits (in pixels).   */
  UL_TIMER_SEPARATE,   /* Disable separator between the main timer groups.   */
  UL_VOLUME_HRZ,       /* Make volume bar horizontal.                        */
  UL_VOLUME_SLIDER,    /* Give volume bar a handle you can grab.             */
  UL_BPS_DIGITS,       /* Draw bitrates with digits from resource 1830-1839. */
  UL_PL_INDEX,         /* Draw playlist indicator with resources 1660-1669.  */
                       /* Number of digits to draw. (min. 3)                 */
  UL_BUNDLE,           /* The bundle file for this skin.                     */
  UL__MAX
};

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
//HBITMAP bmp_load_bitmap( const char* filename );
/* Draws a bit map using the current image colors and mixes. */
//void bmp_draw_bitmap( HPS hps, int x, int y, int res );
/* Draws a specified digit using the specified size. */
//void bmp_draw_digit( HPS hps, int x, int y, unsigned int digit, int size );
/* Draws a 3D shade of the specified area. */
//void bmp_draw_shade( HPS hps, int x, int y, int cx, int cy, long clr1, long clr2 );

/* Returns a width of the specified bitmap. */
//int  bmp_cx( int id );
/* Returns a height of the specified bitmap. */
//int  bmp_cy( int id );

/* Draws a activation led. */
void bmp_draw_led( HPS hps, int active );
/* Draws the player background. */
void bmp_draw_background( HPS hps, HWND hwnd );
/* Draws the specified part of the player background. */
//void bmp_draw_part_bg( HPS hps, int x1, int y1, int x2, int y2 );
/* Draws the main player timer. */
void bmp_draw_timer( HPS hps, double time );
/* Draws the tiny player timer. */
void bmp_draw_tiny_timer( HPS hps, int pos_id, double time );
/* Draws the channels indicator. */
void bmp_draw_channels( HPS hps, int channels );
/* Draws the volume bar and volume slider. */
void bmp_draw_volume( HPS hps, double volume );
/* Draws the file bitrate. */
void bmp_draw_rate( HPS hps, int rate );
/* Draws the current playlist index. */
void bmp_draw_plind( HPS hps, int index, int total );
/* Draws the current playlist mode and time left labels. */
void bmp_draw_plmode( HPS hps, BOOL valid, Playable::Flags flags );
/* Draws the current position slider. */
void bmp_draw_slider( HPS hps, double played, double total );
/* Draws a current displayed text using the current selected font. */
void bmp_draw_text( HPS hps );

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
double bmp_calc_volume( POINTL pos );
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
