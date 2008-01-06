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
/* Draws the current playlist mode. */
void bmp_draw_plmode( HPS hps, BOOL valid, Playable::Flags flags );
/* Draws the current position slider. */
void bmp_draw_slider( HPS hps, double played, double total );
/* Draws a current displayed text using the current selected font. */
void bmp_draw_text( HPS hps );
/* Draws the time left and playlist left labels. */
void bmp_draw_timeleft( HPS hps, Playable* root );

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
