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

#define  INCL_DOS
#define  INCL_WIN
#define  INCL_GPI
#define  INCL_DEV
#define  INCL_BITMAPFILEFORMAT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include <memory.h>
#include <io.h>
#include <fcntl.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <gbm.h>
#include <gbmerr.h>
#include <gbmht.h>
#include "plugman.h"
#include "skin.h"
#include "pm123.h"
#include "pm123.rc.h"
#include "button95.h"
#include <os2.h>

#include <debuglog.h>

static HBITMAP bmp_cache[3000];
static POINTL  bmp_pos  [POS__MAX];
static ULONG   bmp_ulong[UL__MAX];
static char    visual_param[256];

#define POS_UNDEF -1
#define LCID_FONT  1L

#define HAVE_BITBLT_BUG

/* New characters needed: "[],%" 128 - 255 */
static const char AMP_CHARSET[] = "abcdefghijklmnopqrstuvwxyz-_&.0123456789 ()„”:,+%[]";
static const int  LEN_CHARSET   = sizeof( AMP_CHARSET ) - 1;

/* The set of variables used during a text scroll. */
static char    s_display[512];
static size_t  s_len;
static size_t  s_offset;
static int     s_inc;
static int     s_pause;
static BOOL    s_done;
static HDC     s_dc     = NULLHANDLE;
static HPS     s_buffer = NULLHANDLE;
static HBITMAP s_bitmap = NULLHANDLE;

/* Buttons */
static BMPBUTTON btn_play = 
{ NULLHANDLE,     /* Button window handle.                          */
  BMP_PLAY,       /* Pressed state bitmap for regular mode.         */
  BMP_N_PLAY,     /* Release state bitmap for regular mode.         */
  POS_R_PLAY,     /* Button position for regular mode.              */
  BMP_S_PLAY,     /* Pressed state bitmap for small and tiny modes. */
  BMP_SN_PLAY,    /* Release state bitmap for small and tiny modes. */
  POS_S_PLAY,     /* Button position for small mode.                */
  POS_T_PLAY,     /* Button position for tiny mode.                 */
  0,              /* Button state.                                  */
  TRUE,           /* Is this a sticky button.                       */
  "Starts playing" };

static BMPBUTTON btn_stop =
{ NULLHANDLE,     /* Button window handle.                          */
  BMP_STOP,       /* Pressed state bitmap for regular mode.         */
  BMP_N_STOP,     /* Release state bitmap for regular mode.         */
  POS_R_STOP,     /* Button position for regular mode.              */
  BMP_S_STOP,     /* Pressed state bitmap for small and tiny modes. */
  BMP_SN_STOP,    /* Release state bitmap for small and tiny modes. */
  POS_S_STOP,     /* Button position for small mode.                */
  POS_T_STOP,     /* Button position for tiny mode.                 */
  0,              /* Button state.                                  */
  FALSE,          /* Is this a sticky button.                       */
  "Stops playing" };

static BMPBUTTON btn_pause =
{ NULLHANDLE,     /* Button window handle.                          */
  BMP_PAUSE,      /* Pressed state bitmap for regular mode.         */
  BMP_N_PAUSE,    /* Release state bitmap for regular mode.         */
  POS_R_PAUSE,    /* Button position for regular mode.              */
  BMP_S_PAUSE,    /* Pressed state bitmap for small and tiny modes. */
  BMP_SN_PAUSE,   /* Release state bitmap for small and tiny modes. */
  POS_S_PAUSE,    /* Button position for small mode.                */
  POS_T_PAUSE,    /* Button position for tiny mode.                 */
  0,              /* Button state.                                  */
  TRUE,           /* Is this a sticky button.                       */
  "Pauses the playback" };

static BMPBUTTON btn_rew =
{ NULLHANDLE,     /* Button window handle.                          */
  BMP_REW,        /* Pressed state bitmap for regular mode.         */
  BMP_N_REW,      /* Release state bitmap for regular mode.         */
  POS_R_REW,      /* Button position for regular mode.              */
  BMP_S_REW,      /* Pressed state bitmap for small and tiny modes. */
  BMP_SN_REW,     /* Release state bitmap for small and tiny modes. */
  POS_S_REW,      /* Button position for small mode.                */
  POS_T_REW,      /* Button position for tiny mode.                 */
  0,              /* Button state.                                  */
  TRUE,           /* Is this a sticky button.                       */
  "Seeks current song backward" };

static BMPBUTTON btn_fwd =
{ NULLHANDLE,     /* Button window handle.                          */
  BMP_FWD,        /* Pressed state bitmap for regular mode.         */
  BMP_N_FWD,      /* Release state bitmap for regular mode.         */
  POS_R_FWD,      /* Button position for regular mode.              */
  BMP_S_FWD,      /* Pressed state bitmap for small and tiny modes. */
  BMP_SN_FWD,     /* Release state bitmap for small and tiny modes. */
  POS_S_FWD,      /* Button position for small mode.                */
  POS_T_FWD,      /* Button position for tiny mode.                 */
  0,              /* Button state.                                  */
  TRUE,           /* Is this a sticky button.                       */
  "Seeks current song forward" };

static BMPBUTTON btn_power =
{ NULLHANDLE,     /* Button window handle.                          */
  BMP_POWER,      /* Pressed state bitmap for regular mode.         */
  BMP_N_POWER,    /* Release state bitmap for regular mode.         */
  POS_R_POWER,    /* Button position for regular mode.              */
  BMP_S_POWER,    /* Pressed state bitmap for small and tiny modes. */
  BMP_SN_POWER,   /* Release state bitmap for small and tiny modes. */
  POS_S_POWER,    /* Button position for small mode.                */
  POS_T_POWER,    /* Button position for tiny mode.                 */
  0,              /* Button state.                                  */
  FALSE,          /* Is this a sticky button.                       */
  "Quits PM123" };

static BMPBUTTON btn_prev =
{ NULLHANDLE,     /* Button window handle.                          */
  BMP_PREV,       /* Pressed state bitmap for regular mode.         */
  BMP_N_PREV,     /* Release state bitmap for regular mode.         */
  POS_R_PREV,     /* Button position for regular mode.              */
  BMP_S_PREV,     /* Pressed state bitmap for small and tiny modes. */
  BMP_SN_PREV,    /* Release state bitmap for small and tiny modes. */
  POS_S_PREV,     /* Button position for small mode.                */
  POS_T_PREV,     /* Button position for tiny mode.                 */
  0,              /* Button state.                                  */
  FALSE,          /* Is this a sticky button.                       */
  "Selects previous song in playlist" };

static BMPBUTTON btn_next =
{ NULLHANDLE,     /* Button window handle.                          */
  BMP_NEXT,       /* Pressed state bitmap for regular mode.         */
  BMP_N_NEXT,     /* Release state bitmap for regular mode.         */
  POS_R_NEXT,     /* Button position for regular mode.              */
  BMP_S_NEXT,     /* Pressed state bitmap for small and tiny modes. */
  BMP_SN_NEXT,    /* Release state bitmap for small and tiny modes. */
  POS_S_NEXT,     /* Button position for small mode.                */
  POS_T_NEXT,     /* Button position for tiny mode.                 */
  0,              /* Button state.                                  */
  FALSE,          /* Is this a sticky button.                       */
  "Selects next song in playlist" };

static BMPBUTTON btn_shuffle =
{ NULLHANDLE,     /* Button window handle.                          */
  BMP_SHUFFLE,    /* Pressed state bitmap for regular mode.         */
  BMP_N_SHUFFLE,  /* Release state bitmap for regular mode.         */
  POS_R_SHUFFLE,  /* Button position for regular mode.              */
  BMP_S_SHUFFLE,  /* Pressed state bitmap for small and tiny modes. */
  BMP_SN_SHUFFLE, /* Release state bitmap for small and tiny modes. */
  POS_S_SHUFFLE,  /* Button position for small mode.                */
  POS_T_SHUFFLE,  /* Button position for tiny mode.                 */
  0,              /* Button state.                                  */
  TRUE,           /* Is this a sticky button.                       */
  "Plays randomly from the playlist" };

static BMPBUTTON btn_repeat =
{ NULLHANDLE,     /* Button window handle.                          */
  BMP_REPEAT,     /* Pressed state bitmap for regular mode.         */
  BMP_N_REPEAT,   /* Release state bitmap for regular mode.         */
  POS_R_REPEAT,   /* Button position for regular mode.              */
  BMP_S_REPEAT,   /* Pressed state bitmap for small and tiny modes. */
  BMP_SN_REPEAT,  /* Release state bitmap for small and tiny modes. */
  POS_S_REPEAT,   /* Button position for small mode.                */
  POS_T_REPEAT,   /* Button position for tiny mode.                 */
  0,              /* Button state.                                  */
  TRUE,           /* Is this a sticky button.                       */
  "Toggles playlist/song repeat" };

static BMPBUTTON btn_pl =
{ NULLHANDLE,     /* Button window handle.                          */
  BMP_PL,         /* Pressed state bitmap for regular mode.         */
  BMP_N_PL,       /* Release state bitmap for regular mode.         */
  POS_R_PL,       /* Button position for regular mode.              */
  BMP_S_PL,       /* Pressed state bitmap for small and tiny modes. */
  BMP_SN_PL,      /* Release state bitmap for small and tiny modes. */
  POS_S_PL,       /* Button position for small mode.                */
  POS_T_PL,       /* Button position for tiny mode.                 */
  0,              /* Button state.                                  */
  FALSE,          /* Is this a sticky button.                       */
  "Opens/hides playlist window" };

static BMPBUTTON btn_fload =
{ NULLHANDLE,     /* Button window handle.                          */
  BMP_FLOAD,      /* Pressed state bitmap for regular mode.         */
  BMP_N_FLOAD,    /* Release state bitmap for regular mode.         */
  POS_R_FLOAD,    /* Button position for regular mode.              */
  BMP_S_FLOAD,    /* Pressed state bitmap for small and tiny modes. */
  BMP_SN_FLOAD,   /* Release state bitmap for small and tiny modes. */
  POS_S_FLOAD,    /* Button position for small mode.                */
  POS_T_FLOAD,    /* Button position for tiny mode.                 */
  0,              /* Button state.                                  */
  FALSE,          /* Is this a sticky button.                       */
  "Load a file" };

typedef struct _BUNDLEHDR
{
  int   resource;
  char  filename[128];
  ULONG length;

} BUNDLEHDR;


/* Returns a width of the specified bitmap. */
static int
bmp_cx( int id )
{
  BITMAPINFOHEADER2 hdr;

  hdr.cbFix = sizeof( BITMAPINFOHEADER2 );
  GpiQueryBitmapInfoHeader( bmp_cache[id], &hdr );
  return hdr.cx;
}

/* Returns a height of the specified bitmap. */
static int
bmp_cy( int id )
{
  BITMAPINFOHEADER2 hdr;

  hdr.cbFix = sizeof( BITMAPINFOHEADER2 );
  GpiQueryBitmapInfoHeader( bmp_cache[id], &hdr );
  return hdr.cy;
}

/* Reads a bitmap file data into memory. */
static BOOL
bmp_read_bitmap( const char* filename, GBM* gbm, GBMRGB* gbmrgb, BYTE** ppbData )
{
  int   file;
  int   filetype;
  int   cb;
  char* opt = "";

  if( gbm_guess_filetype( filename, &filetype ) != GBM_ERR_OK ) {
    amp_player_error( "Unable deduce bitmap format from file extension:\n%s\n", filename );
    return FALSE;
  }

  file = gbm_io_open( filename, GBM_O_RDONLY );

  if( file == -1 ) {
    amp_player_error( "Unable open bitmap file:\n%s\n", filename );
    return FALSE;
  }

  if( gbm_read_header( filename, file, filetype, gbm, opt ) != GBM_ERR_OK ) {
    gbm_io_close( file );
    amp_player_error( "Unable read bitmap file header:\n%s\n", filename );
    return FALSE;
  }

  if( gbm_read_palette( file, filetype, gbm, gbmrgb ) != GBM_ERR_OK ) {
    gbm_io_close( file );
    amp_player_error( "Unable read bitmap file palette:\n%s\n", filename );
    return FALSE;
  }

  cb = (( gbm->w * gbm->bpp + 31 ) / 32 ) * 4 * gbm->h;

  if(( *ppbData = (BYTE*)malloc( cb )) == NULL ) {
    gbm_io_close( file );
    amp_player_error( "Out of memory" );
    return FALSE;
  }

  if( gbm_read_data( file, filetype, gbm, (byte*)*ppbData ) != GBM_ERR_OK )
  {
    free( *ppbData );
    gbm_io_close( file );
    amp_player_error( "Unable read bitmap file data:\n%s\n", filename );
    return FALSE;
  }

  gbm_io_close( file );
  return TRUE;
}

/* Creates a native OS/2 bitmap from specified data. */
static BOOL
bmp_make_bitmap( HWND hwnd, GBM* gbm, GBMRGB* gbmrgb, BYTE* pbData, HBITMAP* phbm )
{
  HAB hab = WinQueryAnchorBlock( hwnd );

  USHORT cRGB, usCol;
  SIZEL  sizl;
  HDC    hdc;
  HPS    hps;

  struct {
    BITMAPINFOHEADER2 bmp2;
    RGB2 argb2Color[0x100];
  } bm;

  // Got the data in memory, now make bitmap.
  memset( &bm, 0, sizeof( bm ));

  bm.bmp2.cbFix     = sizeof( BITMAPINFOHEADER2 );
  bm.bmp2.cx        = gbm->w;
  bm.bmp2.cy        = gbm->h;
  bm.bmp2.cBitCount = gbm->bpp;
  bm.bmp2.cPlanes   = 1;

  cRGB = (( 1 << gbm->bpp ) & 0x1FF );
  // 1 -> 2, 4 -> 16, 8 -> 256, 24 -> 0

  for( usCol = 0; usCol < cRGB; usCol++ )
  {
    bm.argb2Color[ usCol ].bRed   = gbmrgb[ usCol ].r;
    bm.argb2Color[ usCol ].bGreen = gbmrgb[ usCol ].g;
    bm.argb2Color[ usCol ].bBlue  = gbmrgb[ usCol ].b;
  }

  if(( hdc = DevOpenDC( hab, OD_MEMORY, "*", 0L, (PDEVOPENDATA)NULL, (HDC)NULL )) == (HDC)NULL )
  {
    return FALSE;
  }

  sizl.cx = bm.bmp2.cx;
  sizl.cy = bm.bmp2.cy;

  if(( hps = GpiCreatePS( hab, hdc, &sizl,
                          PU_PELS | GPIF_DEFAULT | GPIT_MICRO | GPIA_ASSOC )) == (HPS)NULL )
  {
    DevCloseDC( hdc );
    return FALSE;
  }

  if( cRGB == 2 )
  {
    static RGB2 argb2Black = { 0x00, 0x00, 0x00 };
    static RGB2 argb2White = { 0xff, 0xff, 0xff };

    bm.argb2Color[0] = argb2Black; /* Contrast */
    bm.argb2Color[1] = argb2White; /* Reset    */
  }

  if(( *phbm = GpiCreateBitmap( hps, &(bm.bmp2), CBM_INIT,
                               (BYTE*)pbData, (BITMAPINFO2*)&bm.bmp2 )) == (HBITMAP)NULL )
  {
    GpiDestroyPS( hps );
    DevCloseDC  ( hdc );
    return FALSE;
  }

  GpiSetBitmap( hps, (HBITMAP)NULL );
  GpiDestroyPS( hps );
  DevCloseDC  ( hdc );
  return TRUE;
}

/* Creates and loads a bitmap from a file, and returns the bitmap handle. */
static HBITMAP
bmp_load_bitmap( const char* filename )
{
  GBM     gbm;
  GBMRGB  gbmrgb[0x100];
  BYTE*   pbData;
  HBITMAP hbmBmp = NULLHANDLE;

  gbm_init();

  if( bmp_read_bitmap( filename, &gbm, gbmrgb, &pbData )) {
    bmp_make_bitmap( HWND_DESKTOP, &gbm, gbmrgb, pbData, &hbmBmp );
    free( pbData );
  }

  gbm_deinit();
  return hbmBmp;
}

/* Draws a bitmap using the current image colors and mixes. */
static void
bmp_draw_bitmap( HPS hps, int x, int y, int res )
{ DEBUGLOG(("bmp_draw_bitmap(%p, %i,%i, %i) - %p\n", hps, x, y, res, bmp_cache[res]));
  POINTL pos[3];

  pos[0].x = x;
  pos[0].y = y;
  pos[1].x = x + bmp_cx(res);
  pos[1].y = y + bmp_cy(res);
  pos[2].x = 0;
  pos[2].y = 0;

  PMXASSERT(GpiWCBitBlt( hps, bmp_cache[res], 3, pos, ROP_SRCCOPY, BBO_AND ), != GPI_ERROR);
}

static void
bmp_draw_box3d( HPS hps, int x, int y, int cx, int cy, long clr1, long clr2 )
{
  POINTL pos;

  pos.x = x;
  pos.y = y;
  GpiMove( hps, &pos );
  GpiSetColor( hps, clr2 );
  pos.x = x + cx;
  GpiLine( hps, &pos );
  GpiMove( hps, &pos );
  pos.y = y + cy;
  GpiLine( hps, &pos );
  pos.x = x;
  GpiSetColor( hps, clr1 );
  GpiLine( hps, &pos );
  GpiMove( hps, &pos );
  pos.y = y;
  GpiLine( hps, &pos );
}

/* Draws a 3D shade of the specified area. */
static void
bmp_draw_shade( HPS hps, int x, int y, int cx, int cy, long clr1, long clr2 )
{
  bmp_draw_box3d( hps, x, y, cx, cy, clr1, clr2 );
  bmp_draw_box3d( hps, x + 1, y + 1, cx - 2, cy - 2, clr1, clr2 );
}

/* Draws the player background. */
void
bmp_draw_background( HPS hps, HWND hwnd )
{
  DEBUGLOG(("bmp_draw_background(%p, %p)\n", hps, hwnd));

  RECTL  rcl;
  POINTL pos;

  WinQueryWindowRect( hwnd, &rcl );

  if( cfg.mode == CFG_MODE_SMALL && bmp_cache[BMP_S_BGROUND] ) {
    bmp_draw_bitmap( hps, 0, 0, BMP_S_BGROUND );
  } else if( cfg.mode == CFG_MODE_TINY && bmp_cache[BMP_T_BGROUND] ) {
    bmp_draw_bitmap( hps, 0, 0, BMP_T_BGROUND );
  } else {
    bmp_draw_bitmap( hps, 0, 0, BMP_R_BGROUND );
  }

  GpiCreateLogColorTable( hps, 0, LCOLF_RGB, 0, 0, 0 );

  if( bmp_ulong[ UL_SHADE_PLAYER ] )
  {
    bmp_draw_shade( hps, rcl.xLeft,
                         rcl.yBottom,
                         rcl.xRight - rcl.xLeft   - 1,
                         rcl.yTop   - rcl.yBottom - 1,
                         bmp_ulong[ UL_SHADE_BRIGHT ],
                         bmp_ulong[ UL_SHADE_DARK   ]  );
  }

  if( cfg.mode == CFG_MODE_REGULAR && bmp_ulong[ UL_SHADE_VOLUME ] )
  {
    bmp_draw_shade( hps, 7, 36, 13, 50,
                         bmp_ulong[ UL_SHADE_DARK   ],
                         bmp_ulong[ UL_SHADE_BRIGHT ]  );
  }

  if( cfg.mode == CFG_MODE_REGULAR && bmp_ulong[ UL_SHADE_STAT ] )
  {
    rcl.yBottom += 36;
    rcl.xLeft   += 26;
    rcl.xRight  -=  6;
    rcl.yTop    -=  6;

    bmp_draw_shade( hps, rcl.xLeft,
                         rcl.yBottom,
                         rcl.xRight - rcl.xLeft   - 1,
                         rcl.yTop   - rcl.yBottom - 1,
                         bmp_ulong[ UL_SHADE_DARK   ],
                         bmp_ulong[ UL_SHADE_BRIGHT ]  );
  }

  if( cfg.mode == CFG_MODE_SMALL && bmp_ulong[ UL_SHADE_STAT ] )
  {
    rcl.yBottom += 36;
    rcl.xLeft   +=  6;
    rcl.xRight  -=  6;
    rcl.yTop    -=  6;

    bmp_draw_shade( hps, rcl.xLeft,
                         rcl.yBottom,
                         rcl.xRight - rcl.xLeft   - 1,
                         rcl.yTop   - rcl.yBottom - 1,
                         bmp_ulong[ UL_SHADE_DARK   ],
                         bmp_ulong[ UL_SHADE_BRIGHT ]  );
  }

  if( cfg.mode == CFG_MODE_REGULAR && bmp_ulong[ UL_SHADE_SLIDER ] )
  {
    pos.x  = bmp_pos[ POS_SLIDER ].x - 1;
    pos.y  = bmp_pos[ POS_SLIDER ].y - 1;
    GpiMove( hps, &pos );
    pos.x += bmp_ulong[ UL_SLIDER_WIDTH ] + bmp_cx( BMP_SLIDER ) + 1;
    pos.y += bmp_cy( BMP_SLIDER ) + 1;
    GpiSetColor( hps, bmp_ulong[ UL_SLIDER_COLOR ]);
    GpiBox( hps, DRO_OUTLINE, &pos, 0, 0 );
  }
}

/* Draws the specified part of the player background. */
static void
bmp_draw_part_bg_to( HPS hps, int x1_1, int y1_1, int x1_2, int y1_2,
                              int x2_1, int y2_1 )
{

  #if defined( HAVE_BITBLT_BUG )

    POINTL aptlPoints[3];
    int    bg_id = BMP_R_BGROUND;
    RECTL  save_field;
    RECTL  disp_field;

    if( x1_1 > x1_2 || y1_1 > y1_2 ) {
      return;
    }

    if( cfg.mode == CFG_MODE_SMALL && bmp_cache[ BMP_S_BGROUND ] ) {
      bg_id = BMP_S_BGROUND;
    } else if( cfg.mode == CFG_MODE_TINY && bmp_cache[ BMP_T_BGROUND ] ) {
      bg_id = BMP_T_BGROUND;
    }

    GpiQueryGraphicsField( hps, &save_field );

    disp_field.xLeft   = x1_1;
    disp_field.yBottom = y1_1;
    disp_field.xRight  = x1_2;
    disp_field.yTop    = y1_2;

    GpiSetGraphicsField( hps, &disp_field );

    aptlPoints[0].x = x1_1 - x2_1;
    aptlPoints[0].y = y1_1 - y2_1;
    aptlPoints[1].x = x1_2;
    aptlPoints[1].y = y1_2;
    aptlPoints[2].x = 0;
    aptlPoints[2].y = 0;

    GpiWCBitBlt( hps, bmp_cache[bg_id], 3, (PPOINTL)&aptlPoints, ROP_SRCCOPY, 0 );
    GpiSetGraphicsField( hps, &save_field );
  #else
    POINTL aptlPoints[4];
    int    bg_id = BMP_R_BGROUND;

    if( cfg.mode == CFG_MODE_SMALL && bmp_cache[ BMP_S_BGROUND ] ) {
      bg_id = BMP_S_BGROUND;
    } else if( cfg.mode == CFG_MODE_TINY && bmp_cache[ BMP_T_BGROUND ] ) {
      bg_id = BMP_T_BGROUND;
    }

    aptlPoints[0].x = x1_1;
    aptlPoints[0].y = y1_1;
    aptlPoints[1].x = x1_2;
    aptlPoints[1].y = y1_2;
    aptlPoints[2].x = x2_1;
    aptlPoints[2].y = y2_1;
    aptlPoints[3].x = x1_2 + 1;
    aptlPoints[3].y = y1_2 + 1;

    GpiWCBitBlt( hps, bmp_cache[bg_id], 4, (PPOINTL)&aptlPoints, ROP_SRCCOPY, BBO_IGNORE );
  #endif
}

/* Draws the specified part of the player background. */
static void
bmp_draw_part_bg( HPS hps, int x1, int y1, int x2, int y2 )
{
  bmp_draw_part_bg_to( hps, x1, y1, x2, y2, x1, y1 );
}

/* Draws a activation led. */
void
bmp_draw_led( HPS hps, int active )
{
  DEBUGLOG(("bmp_draw_led(%p, %i) %i,%i\n", hps, active, bmp_pos[ POS_N_LED ].x, bmp_pos[ POS_N_LED ].x));

  if( cfg.mode != CFG_MODE_TINY )
  {
    if( active ) {
      if( bmp_pos[ POS_LED ].x != POS_UNDEF &&
          bmp_pos[ POS_LED ].y != POS_UNDEF )
      {
        bmp_draw_bitmap( hps, bmp_pos[ POS_LED ].x,
                              bmp_pos[ POS_LED ].y, BMP_LED );
      }
    } else {
      if( bmp_pos[ POS_N_LED ].x != POS_UNDEF &&
          bmp_pos[ POS_N_LED ].y != POS_UNDEF )
      {
        bmp_draw_bitmap( hps, bmp_pos[ POS_N_LED ].x,
                              bmp_pos[ POS_N_LED ].y, BMP_N_LED );
      }
    }
  }
}

/* Returns a resource id of the first symbol of current the selected font. */
static int
bmp_font( void )
{
  if( cfg.font == 0 ) {
    return BMP_FONT1;
  } else {
    return BMP_FONT2;
  }
}

/* Returns a width of the first symbol of the current selected font. */
static int
bmp_char_cx( void )
{
  return bmp_cx( bmp_font());
}

/* Returns a height of the first symbol of the current selected font. */
static int
bmp_char_cy( void )
{
  return bmp_cy( bmp_font());
}

/* Returns a bitmap identifier of the specified character
   of the current selected font. */
static int
bmp_char_id( char ch )
{
  int id;
  ch = tolower( ch );
  id = strchr ( AMP_CHARSET, ch ) - AMP_CHARSET;

  // Support for 127..255.
  if( id < 0 && ch > 126 ) {
      id = LEN_CHARSET + ch - 126 + 1;
  }

  if( id < 0 || bmp_cache[ bmp_font() + id ] == 0 ) {
      id = 40;
  }

  return bmp_font() + id;
}

/* Calculates a length of the current displayed text
   using the current selected font. */
static size_t
bmp_text_length( const char* string )
{
  int len = 0;

  if( cfg.font_skinned ) {
    while( *string ) {
      len += bmp_cx( bmp_char_id( *string++ ));
    }
  } else {
    RECTL rect = { 0, 0, 32000, 32000 };
    WinDrawText( s_buffer, -1, s_display, &rect, 0, 0, DT_LEFT | DT_VCENTER | DT_QUERYEXTENT );
    len = rect.xRight - rect.xLeft + 1;
  }

  return len;
}

/* Calculates a rectangle of the current displayed text
   using the current selected font. */
static RECTL
bmp_text_rect( void )
{
  RECTL rect;

  switch( cfg.mode ) {
    case CFG_MODE_REGULAR:

      rect.xLeft   = bmp_pos[ POS_R_TEXT ].x;
      rect.yBottom = bmp_pos[ POS_R_TEXT ].y;

      if( bmp_ulong[ UL_IN_PIXELS ] ) {
        rect.xRight = rect.xLeft   + bmp_ulong[ UL_R_MSG_LEN    ] - 1;
        rect.yTop   = rect.yBottom + bmp_ulong[ UL_R_MSG_HEIGHT ] - 1;
      } else {
        rect.xRight = rect.xLeft   + bmp_ulong[ UL_R_MSG_LEN ] * bmp_char_cx() - 1;
        rect.yTop   = rect.yBottom + bmp_char_cy() - 1;
      }
      break;

   case CFG_MODE_SMALL:

      rect.xLeft   = bmp_pos[ POS_S_TEXT ].x;
      rect.yBottom = bmp_pos[ POS_S_TEXT ].y;

      if( bmp_ulong[ UL_IN_PIXELS ] ) {
        rect.xRight = rect.xLeft   + bmp_ulong[ UL_S_MSG_LEN    ] - 1;
        rect.yTop   = rect.yBottom + bmp_ulong[ UL_S_MSG_HEIGHT ] - 1;
      } else {
        rect.xRight = rect.xLeft   + bmp_ulong[ UL_S_MSG_LEN ] * bmp_char_cx() - 1;
        rect.yTop   = rect.yBottom + bmp_char_cy() - 1;
      }
      break;

   default:
      rect.xLeft  = POS_UNDEF;
      rect.xRight = POS_UNDEF;
      break;
  }

  return rect;
}

/* Draws a current displayed text using the current selected font. */
void
bmp_draw_text( HPS hps )
{
  SIZEL  size;
  POINTL pos[3];
  RECTL  rect = bmp_text_rect();

  pos[0].x = rect.xLeft;
  pos[0].y = rect.yBottom;
  pos[1].x = rect.xRight + 1;
  pos[1].y = rect.yTop   + 1;
  pos[2].x = 0;
  pos[2].y = 0;

  // Someone might want to use their own scroller.
  if( rect.xLeft == POS_UNDEF && rect.yBottom == POS_UNDEF ) {
    return;
  }

  if (!GpiQueryBitmapDimension( s_bitmap, &size ))
    return;
  bmp_draw_part_bg_to( s_buffer, 0, 0, size.cx, size.cy,
                       rect.xLeft, rect.yBottom );

  if( cfg.font_skinned )
  {
    char* p =  s_display;
    int   x =  s_offset;
    int   y = (rect.yTop - rect.yBottom - bmp_char_cy()) / 2;
    int   i;

    while( *p ) {
      i = bmp_char_id( *p++ );
      bmp_draw_bitmap( s_buffer, x, y, i );
      x += bmp_cx( i );
    }

    i = bmp_char_id( ' ' );

    while( x < size.cx ) {
      bmp_draw_bitmap( s_buffer, x, y, i );
      x += bmp_cx( i );
    }
  }
  else
  {
    RECTL rect;

    rect.xLeft   = s_offset;
    rect.yBottom = 0;
    rect.xRight  = size.cx;
    rect.yTop    = size.cy;

    WinDrawText( s_buffer, -1, s_display, &rect, 0, 0,
                               DT_TEXTATTRS | DT_LEFT | DT_VCENTER );
  }

  GpiBitBlt( hps, s_buffer, 3, pos, ROP_SRCCOPY, BBO_IGNORE );
}

/* Deletes a memory buffer used for drawing of the displayed text. */
static void
bmp_delete_text_buffer( void )
{
  if( s_buffer != NULLHANDLE ) {
    if( GpiQueryCharSet( s_buffer ) > 0 )
    {
      GpiSetCharSet ( s_buffer, LCID_DEFAULT );
      GpiDeleteSetId( s_buffer, LCID_FONT );
    }
  }
  if( s_bitmap != NULLHANDLE ) {
    GpiSetBitmap( s_buffer, NULLHANDLE );
    GpiDeleteBitmap( s_bitmap );
  }
  if( s_buffer != NULLHANDLE ) {
    GpiDestroyPS( s_buffer );
  }
  if( s_dc != NULLHANDLE ) {
    DevCloseDC( s_dc );
  }

  s_bitmap = NULLHANDLE;
  s_buffer = NULLHANDLE;
  s_dc     = NULLHANDLE;
}

/* Creates a memory buffer used for drawing of the displayed text. */
static void
bmp_create_text_buffer( void )
{
  RECTL rect = bmp_text_rect();
  SIZEL size;

  BITMAPINFOHEADER2 bmp_header;

  bmp_delete_text_buffer();

  size.cx = rect.xRight - rect.xLeft + 1;
  size.cy = rect.yTop - rect.yBottom + 1;

  s_dc = DevOpenDC( amp_player_hab(), OD_MEMORY, "*", 0, NULL, NULLHANDLE );
  s_buffer = GpiCreatePS( amp_player_hab(), s_dc, &size,
                          PU_PELS | GPIT_MICRO | GPIA_ASSOC );

  memset( &bmp_header, 0, sizeof( bmp_header ));
  bmp_header.cbFix      = sizeof( bmp_header );
  bmp_header.cx         = size.cx;
  bmp_header.cy         = size.cy;
  bmp_header.cPlanes    = 1;
  bmp_header.cBitCount  = 8;

  s_bitmap = GpiCreateBitmap( s_buffer, &bmp_header, 0, NULL, NULL );
  GpiSetBitmapDimension( s_bitmap, &size );
  GpiSetBitmap( s_buffer, s_bitmap );
  GpiCreateLogColorTable( s_buffer, 0, LCOLF_RGB, 0, 0, 0 );
}

/* Sets the new displayed text.
   The NULL can be used as argument for reset of a scroll status. */
void
bmp_set_text( const char* string )
{
  DEBUGLOG(("bmp_set_text(%p)\n", string));
  if( string && string != s_display ) {
    strlcpy( s_display, string, sizeof( s_display ));
  }

  s_offset =  0;
  s_inc    = -1;
  s_pause  = 20;
  s_done   = FALSE;

  if( cfg.mode != CFG_MODE_TINY ) {
    if( !s_buffer ) {
      bmp_create_text_buffer();
    }
    if( GpiQueryCharSet( s_buffer ) > 0 )
    {
      GpiSetCharSet ( s_buffer, LCID_DEFAULT );
      GpiDeleteSetId( s_buffer, LCID_FONT );
    }
    if( !cfg.font_skinned )
    {
      GpiSetColor     ( s_buffer, bmp_ulong[ UL_FG_MSG_COLOR ]);
      GpiSetBackMix   ( s_buffer, BM_LEAVEALONE );
      GpiCreateLogFont( s_buffer, NULL, LCID_FONT, &cfg.font_attrs );
      GpiSetCharSet   ( s_buffer, LCID_FONT );

      if( cfg.font_attrs.fsFontUse & FATTR_FONTUSE_OUTLINE )
      {
        HPS   ps   = WinGetPS( HWND_DESKTOP );
        HDC   dc   = GpiQueryDevice( ps );
        LONG  hres = 0;
        LONG  vres = 0;
        SIZEF size;

        DevQueryCaps( dc, CAPS_HORIZONTAL_FONT_RES, 1L, &hres );
        DevQueryCaps( dc, CAPS_VERTICAL_FONT_RES  , 1L, &vres );

        size.cx = ( MAKEFIXED( cfg.font_size, 0 ) / 72 ) * hres;
        size.cy = ( MAKEFIXED( cfg.font_size, 0 ) / 72 ) * vres;

        GpiSetCharBox( s_buffer, &size );
        WinReleasePS ( ps );
      }
    }
    s_len = bmp_text_length( s_display );
  } else {
    s_len = 0;
  }

  vis_broadcast( WM_PLUGIN_CONTROL, MPFROMLONG( PN_TEXTCHANGED ), 0 );
}

/* Returns a pointer to the current selected text. */
const char*
bmp_query_text( void )
{
  return s_display;
}

/* Scrolls the current selected text. */
BOOL
bmp_scroll_text( void )
{
  RECTL  s_rec = bmp_text_rect();
  size_t s_max = s_rec.xRight - s_rec.xLeft + 1;

  if( cfg.scroll == CFG_SCROLL_NONE ||
      cfg.mode   == CFG_MODE_TINY   || s_len < s_max || s_done )
  {
    return FALSE;
  }

  if( s_pause > 0 ) {
    --s_pause;
    return FALSE;
  }

  if( s_inc < 0 && s_len + s_offset <= s_max ) {
    s_inc   = 1;
    s_pause = 16;
    return FALSE;
  }

  if( s_inc > 0 && s_offset == 0 ) {
    if( cfg.scroll == CFG_SCROLL_ONCE ) {
      s_inc   = 0;
      s_done  = TRUE;
    } else {
      s_inc   = -1;
      s_pause = 16;
    }
    return FALSE;
  }

  s_offset += s_inc;
  return TRUE;
}

/* Queries whether a point lies within a current displayed text rectangle. */
BOOL
bmp_pt_in_text( POINTL pos )
{
  RECTL rect = bmp_text_rect();

  if( pos.y >= rect.yBottom && pos.y <= rect.yTop &&
      pos.x >= rect.xLeft   && pos.x <= rect.xRight )
  {
    return TRUE;
  } else {
    return FALSE;
  }
}

/* Converts time to two integer suitable for display by the timer. */
static void
sec2num( double seconds, unsigned int* major, unsigned int* minor )
{ unsigned int val = (unsigned int)(seconds / 3600);
  unsigned int frac = 24;
  if (val*60 < 100*24)
  { val = (unsigned int)(seconds / 60);
    frac = 60;
    if (val < 100)
      val = (unsigned int)seconds;
  }
  *major = val / frac;
  *minor = val % frac;
}

/* Draws the main player timer. */
void
bmp_draw_timer( HPS hps, double time )
{ DEBUGLOG(("bmp_draw_timer(%p, %g)\n", hps, time));

  if( cfg.mode != CFG_MODE_REGULAR ) {
    return;
  }

  int x = bmp_pos[ POS_TIMER ].x;
  int y = bmp_pos[ POS_TIMER ].y;

  if ( x != POS_UNDEF && y != POS_UNDEF && time >= 0 )
  {
    unsigned int major;
    unsigned int minor;
    sec2num( time, &major, &minor );

    div_t d = div(major, 10);
    d.quot %= 10; // limit
    bmp_draw_bitmap( hps, x, y, d.quot + DIG_BIG );
    x += bmp_cx( d.quot + DIG_BIG ) + bmp_ulong[ UL_TIMER_SPACE ];
    bmp_draw_bitmap( hps, x, y, d.rem  + DIG_BIG );
    x += bmp_cx( d.rem  + DIG_BIG ) + bmp_ulong[ UL_TIMER_SPACE ];

    if( bmp_ulong[ UL_TIMER_SEPARATE ] )
    { ULONG idx = DIG_BIG + 10 + ((unsigned int)(2*time) & 1);
      bmp_draw_bitmap( hps, x, y, idx );
      x += bmp_cx( idx ) + bmp_ulong[ UL_TIMER_SPACE ];
    } else {
      // TODO: probably wrong
      x += max( bmp_ulong[ UL_TIMER_SPACE ], bmp_ulong[ UL_TIMER_SPACE ] * 2 );
    }

    d = div(minor, 10);
    bmp_draw_bitmap( hps, x, y, d.quot + DIG_BIG );
    x += bmp_cx( d.quot + DIG_BIG ) + bmp_ulong[ UL_TIMER_SPACE ];
    bmp_draw_bitmap( hps, x, y, d.rem  + DIG_BIG );
  }
}

/* Draws the tiny player timer. */
void
bmp_draw_tiny_timer( HPS hps, int pos_id, double time )
{ DEBUGLOG(("bmp_draw_tiny_timer(%p, %i, %g)\n", hps, pos_id, time));

  if( cfg.mode != CFG_MODE_REGULAR ) {
    return;
  }

  unsigned int major, minor;
  int x = bmp_pos[pos_id].x;
  int y = bmp_pos[pos_id].y;

  sec2num( time, &major, &minor );

  if( x != POS_UNDEF && y != POS_UNDEF )
  {
    if( time >= 0 )
    { div_t d = div(major, 10);
      d.quot %= 10;
      bmp_draw_bitmap( hps, x, y, d.quot ? d.quot + DIG_TINY : DIG_TINY + 11 );
      x += bmp_cx( d.quot + DIG_TINY );
      bmp_draw_bitmap( hps, x, y, d.rem  + DIG_TINY );
      x += bmp_cx( d.rem  + DIG_TINY );
      ULONG idx = DIG_TINY + 10 + (((unsigned int)(2*time) & 1) << 1);
      bmp_draw_bitmap( hps, x, y, idx );
      x += bmp_cx( idx );
      d = div(minor, 10);
      bmp_draw_bitmap( hps, x, y, d.quot + DIG_TINY );
      x += bmp_cx( d.quot + DIG_TINY );
      bmp_draw_bitmap( hps, x, y, d.rem  + DIG_TINY );
    } else
      bmp_draw_part_bg( hps, x, y, x + bmp_cx( DIG_TINY ) * 4 + bmp_cx( DIG_TINY + 10 ),
                                   y + bmp_cy( DIG_TINY ));
  }
}

/* Draws the channels indicator. */
void
bmp_draw_channels( HPS hps, int channels )
{
  DEBUGLOG(("bmp_draw_channels(%p, %i)\n", hps, channels));

  int     id;
  POINTL* pos;

  if( cfg.mode != CFG_MODE_REGULAR ) {
    return;
  }

  if( bmp_pos[ POS_NO_CHANNELS ].x != POS_UNDEF &&
      bmp_pos[ POS_NO_CHANNELS ].y != POS_UNDEF )
  {
    bmp_draw_part_bg( hps, bmp_pos[ POS_NO_CHANNELS ].x,
                           bmp_pos[ POS_NO_CHANNELS ].y,
                           bmp_pos[ POS_NO_CHANNELS ].x + bmp_cx( BMP_NO_CHANNELS ),
                           bmp_pos[ POS_NO_CHANNELS ].y + bmp_cy( BMP_NO_CHANNELS ));
  }
  if( bmp_pos[ POS_MONO ].x != POS_UNDEF &&
      bmp_pos[ POS_MONO ].y != POS_UNDEF )
  {
    bmp_draw_part_bg( hps, bmp_pos[ POS_MONO ].x,
                           bmp_pos[ POS_MONO ].y,
                           bmp_pos[ POS_MONO ].x + bmp_cx( BMP_MONO ),
                           bmp_pos[ POS_MONO ].y + bmp_cy( BMP_MONO ));
  }
  if( bmp_pos[ POS_STEREO ].x != POS_UNDEF &&
      bmp_pos[ POS_STEREO ].y != POS_UNDEF )
  {
    bmp_draw_part_bg( hps, bmp_pos[ POS_STEREO ].x,
                           bmp_pos[ POS_STEREO ].y,
                           bmp_pos[ POS_STEREO ].x + bmp_cx( BMP_STEREO ),
                           bmp_pos[ POS_STEREO ].y + bmp_cy( BMP_STEREO ));
  }

  switch( channels )
  {
    default:
      id  = BMP_NO_CHANNELS;
      pos = bmp_pos + POS_NO_CHANNELS;
      break;

    case 1:
      id  = BMP_MONO;
      pos = bmp_pos + POS_MONO;
      break;

    case 2:
      id  = BMP_STEREO;
      pos = bmp_pos + POS_STEREO;
      break;
  }

  if( pos->x != POS_UNDEF && pos->y != POS_UNDEF )
  {
    bmp_draw_bitmap( hps, pos->x, pos->y, id );
  }
}

/* Draws the volume bar and volume slider. */
void
bmp_draw_volume( HPS hps, double volume )
{
  DEBUGLOG(("bmp_draw_volume(%p, %g)\n", hps, volume));

  int x = bmp_pos[ POS_VOLBAR ].x;
  int y = bmp_pos[ POS_VOLBAR ].y;
  int xo, yo;

  if( cfg.mode != CFG_MODE_REGULAR ) {
    return;
  }

  if( volume < 0 ) {
    volume = 0;
  } else if( volume > 1 ) {
    volume = 1;
  }

  if( !bmp_ulong[ UL_VOLUME_SLIDER ])
  {
    xo = bmp_ulong[ UL_VOLUME_HRZ ] ? (int)(volume * bmp_cx( BMP_VOLBAR ) +.5) : 0;
    yo = bmp_ulong[ UL_VOLUME_HRZ ] ? 0 : (int)(volume * bmp_cy( BMP_VOLBAR ) +.5);

    bmp_draw_bitmap ( hps, x, y, BMP_VOLBAR );
    bmp_draw_part_bg( hps, x + xo, y + yo, x + bmp_cx( BMP_VOLBAR ) - 1,
                                           y + bmp_cy( BMP_VOLBAR ) - 1 );
  }
  else
  {
    if( !bmp_ulong[ UL_VOLUME_HRZ ] )
    {
      yo = (int)(volume * ( bmp_cy( BMP_VOLBAR ) - bmp_cy( BMP_VOLSLIDER )) +.5);

      bmp_draw_bitmap( hps, x, y, BMP_VOLBAR );
      bmp_draw_bitmap( hps, x + bmp_pos[ POS_VOLSLIDER ].x,
                            y + bmp_pos[ POS_VOLSLIDER ].y + yo, BMP_VOLSLIDER );
    }
    else
    {
      xo = (int)(volume * ( bmp_cx( BMP_VOLBAR ) - bmp_cx( BMP_VOLSLIDER )) +.5);

      bmp_draw_bitmap( hps, x, y, BMP_VOLBAR );
      bmp_draw_bitmap( hps, x + bmp_pos[ POS_VOLSLIDER ].x + xo,
                            y + bmp_pos[ POS_VOLSLIDER ].y, BMP_VOLSLIDER );
    }
  }
}

/* Queries whether a point lies within a volume bar rectangle. */
BOOL
bmp_pt_in_volume( POINTL pos )
{
  if( cfg.mode != CFG_MODE_REGULAR ) {
    return FALSE;
  }

  if( pos.x >= bmp_pos[ POS_VOLBAR ].x &&
      pos.x <= bmp_pos[ POS_VOLBAR ].x + bmp_cx( BMP_VOLBAR ) &&
      pos.y >= bmp_pos[ POS_VOLBAR ].y &&
      pos.y <= bmp_pos[ POS_VOLBAR ].y + bmp_cy( BMP_VOLBAR ))
  {
    return TRUE;
  } else {
    return FALSE;
  }
}

/* Calculates a volume level on the basis of position of the
   mouse pointer concerning the volume bar. */
double
bmp_calc_volume( POINTL pos )
{
  double volume;

  if( bmp_ulong[ UL_VOLUME_HRZ ]) {
    volume = ( pos.x - bmp_pos[ POS_VOLBAR ].x + 1. ) / bmp_cx( BMP_VOLBAR );
  } else {
    volume = ( pos.y - bmp_pos[ POS_VOLBAR ].y + 1. ) / bmp_cy( BMP_VOLBAR );
  }

  if( volume < 0) {
    volume = 0;
  } else if( volume > 1 ) {
    volume = 1;
  }

  return volume;
}

/* Draws the file bitrate. */
void
bmp_draw_rate( HPS hps, int rate )
{
  static const int  rate_index[] = { 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256, 320 };
  int  i, index = 0;
  char buf[32];
  int  x = bmp_pos[ POS_BPS ].x;
  int  y = bmp_pos[ POS_BPS ].y;

  DEBUGLOG(("bmp_draw_rate(%p, %d)\n", hps, rate));

  if( cfg.mode != CFG_MODE_REGULAR ) {
    return;
  }

  for( i = 0; i < sizeof( rate_index ) / sizeof( int ); i++ ) {
    if( rate_index[i] == rate ) {
      index = i;
      break;
    }
  }

  if( !bmp_ulong[ UL_BPS_DIGITS ]) {
    bmp_draw_bitmap ( hps, x, y, BMP_BPS + index );
  }
  else
  {
    if( bmp_cache[ DIG_BPS + 10 ] ) {
      bmp_draw_part_bg( hps, x, y,
                             x + bmp_cx( DIG_BPS ) * 3,
                             y + bmp_cy( DIG_BPS ));
    } else {
      bmp_draw_part_bg( hps, x - bmp_cx( DIG_BPS ),
                             y,
                             x + bmp_cx( DIG_BPS ) * 4,
                             y + bmp_cy( DIG_BPS ));
    }

    if( rate > 0 && rate <= 9999 )
    {
      x += bmp_cx( DIG_BPS ) * 2;

      if( rate > 999 && bmp_cache[ DIG_BPS + 10 ] ) {
        sprintf( buf, "%u", rate / 100 );
        bmp_draw_bitmap( hps, x, y, 10 + DIG_BPS );
        x -= bmp_cx( DIG_BPS );
      } else {
        sprintf( buf, "%u", rate );
      }

      for( i = strlen( buf ) - 1; i >= 0; i-- )
      {
        if( buf[i] != ' ' ) {
          bmp_draw_bitmap( hps, x, y, buf[i] - 48 + DIG_BPS );
          x -= bmp_cx( DIG_BPS );
        }
      }
    }
  }
}

/* Draws the current playlist mode and time left labels. */
void
bmp_draw_plmode( HPS hps, BOOL valid, Playable::Flags flags )
{
  DEBUGLOG(("bmp_draw_plmode(%p, %u, %x)\n", hps, valid, flags));

  if( cfg.mode != CFG_MODE_REGULAR )
    return;
  
  if( bmp_pos[ POS_PL_MODE ].x != POS_UNDEF && bmp_pos[ POS_PL_MODE ].y != POS_UNDEF )
  {
    if (!valid)
      bmp_draw_bitmap( hps, bmp_pos[ POS_PL_MODE ].x, bmp_pos[ POS_PL_MODE ].y, BMP_NOFILE );
    else if (flags & Playable::Enumerable)
      bmp_draw_bitmap( hps, bmp_pos[ POS_PL_MODE ].x, bmp_pos[ POS_PL_MODE ].y, BMP_LISTPLAY );
    else
      bmp_draw_bitmap( hps, bmp_pos[ POS_PL_MODE ].x, bmp_pos[ POS_PL_MODE ].y, BMP_SINGLEPLAY );
  }
  
  if( !valid ) {
    if( bmp_pos[ POS_NOTL ].x != POS_UNDEF && bmp_pos[ POS_NOTL ].y != POS_UNDEF )
      bmp_draw_bitmap( hps, bmp_pos[ POS_NOTL ].x, bmp_pos[ POS_NOTL ].y, BMP_NOTL );
  } else {
    if( bmp_pos[ POS_TL ].x != POS_UNDEF && bmp_pos[ POS_TL ].y != POS_UNDEF )
      bmp_draw_bitmap( hps, bmp_pos[ POS_TL   ].x, bmp_pos[ POS_TL   ].y, BMP_TL );
  }

  if( valid && (flags & Playable::Enumerable) ) {
    if( bmp_pos[ POS_PLIST   ].x != POS_UNDEF && bmp_pos[ POS_PLIST   ].y != POS_UNDEF )
      bmp_draw_bitmap( hps, bmp_pos[ POS_PLIST   ].x, bmp_pos[ POS_PLIST   ].y, BMP_PLIST );
  } else {
    if( bmp_pos[ POS_NOPLIST ].x != POS_UNDEF && bmp_pos[ POS_NOPLIST ].y != POS_UNDEF )
      bmp_draw_bitmap( hps, bmp_pos[ POS_NOPLIST ].x, bmp_pos[ POS_NOPLIST ].y, BMP_NOPLIST );
  }
}

/* Draws the current playlist index. */
void
bmp_draw_plind( HPS hps, int index, int total )
{
  DEBUGLOG(("bmp_draw_plind(%p, %i, %i)\n", hps, index, total));

  if( cfg.mode != CFG_MODE_REGULAR ) {
    return;
  }

  int digits = bmp_ulong[ UL_PL_INDEX ];
  if( digits )
  {
    int i;

    if ( digits < 3 )
      digits = 3;
      
    if( bmp_pos[ POS_PL_INDEX ].x != POS_UNDEF &&
        bmp_pos[ POS_PL_INDEX ].y != POS_UNDEF )
    {
      POINTL p = bmp_pos[ POS_PL_INDEX ];
      bmp_draw_part_bg( hps, p.x, p.y, p.x + bmp_cx( DIG_PL_INDEX ) * digits,
                                       p.y + bmp_cy( DIG_PL_INDEX ));

      if( index > 0 )
      { for( i = digits; --i >= 0; )
        { div_t d = div( index, 10 );
          bmp_draw_bitmap( hps, p.x + bmp_cx( DIG_PL_INDEX ) * i, p.y, index ? DIG_PL_INDEX + d.rem : DIG_PL_INDEX + 10 );
          index = d.quot;
        }
      }
    }

    if( bmp_pos[ POS_PL_TOTAL ].x != POS_UNDEF &&
        bmp_pos[ POS_PL_TOTAL ].y != POS_UNDEF )
    {
      POINTL p = bmp_pos[ POS_PL_TOTAL ];
      bmp_draw_part_bg( hps, p.x, p.y, p.x + bmp_cx( DIG_PL_INDEX ) * digits,
                                       p.y + bmp_cy( DIG_PL_INDEX ));

      if( total > 0 )
      { for( i = digits; --i >= 0; )
        { div_t d = div( total, 10 );
          bmp_draw_bitmap( hps, p.x + bmp_cx( DIG_PL_INDEX ) * i, p.y, total ? DIG_PL_INDEX + d.rem : DIG_PL_INDEX + 10 );
          total = d.quot;
        }
      }
    }
  }
  /* probably crap
  else
  {
    POINTL pos = { 158, 50 };

    bmp_draw_part_bg( hps, pos.x, pos.y, pos.x + 50, pos.y + 20 );

    if( total > 0 )
    {
      sprintf( buf, "%02u of %02u", index, total );
      GpiCreateLogColorTable( hps, 0, LCOLF_RGB, 0, 0, 0 );
      GpiSetColor( hps, bmp_ulong[ UL_PL_COLOR ]);
      GpiSetBackColor( hps, 0x00000000UL );
      GpiMove( hps, &pos );
      GpiCharString( hps, strlen( buf ), buf );
    }
  }*/
}

/* Draws the current position slider. */
void
bmp_draw_slider( HPS hps, double played, double total )
{ DEBUGLOG(("bmp_draw_slider(%p, %g, %g)\n", hps, played, total));
  
  if( cfg.mode == CFG_MODE_REGULAR )
  {
    if( bmp_cache[ BMP_SLIDER_SHAFT ]   != 0 &&
        bmp_pos  [ POS_SLIDER_SHAFT ].x != POS_UNDEF &&
        bmp_pos  [ POS_SLIDER_SHAFT ].y != POS_UNDEF )
    {
      bmp_draw_bitmap( hps, bmp_pos[ POS_SLIDER_SHAFT ].x,
                            bmp_pos[ POS_SLIDER_SHAFT ].y, BMP_SLIDER_SHAFT );
    }
    else
    {
      bmp_draw_part_bg( hps, bmp_pos[ POS_SLIDER ].x,
                             bmp_pos[ POS_SLIDER ].y,
                             bmp_pos[ POS_SLIDER ].x + bmp_ulong[ UL_SLIDER_WIDTH ] + bmp_cx( BMP_SLIDER ) - 1,
                             bmp_pos[ POS_SLIDER ].y + bmp_cy( BMP_SLIDER ) - 1 );
    }

    if( total > 0 && played >= 0 )
    { ULONG pos = played >= total
        ? bmp_ulong[UL_SLIDER_WIDTH]
        : (ULONG)(played/total * bmp_ulong[UL_SLIDER_WIDTH]);

      bmp_draw_bitmap( hps, bmp_pos[ POS_SLIDER ].x + pos,
                            bmp_pos[ POS_SLIDER ].y, BMP_SLIDER );
    }
  }
}

/* Calculates a current seeking time on the basis of position of the
   mouse pointer concerning the position slider. */
double
bmp_calc_time( POINTL pos, double total )
{
  double time = 0;

  if( bmp_ulong[ UL_SLIDER_WIDTH ] && pos.x >= bmp_pos[ POS_SLIDER ].x )
  {
    time = total * ( pos.x - bmp_pos[ POS_SLIDER ].x ) / bmp_ulong[ UL_SLIDER_WIDTH ];

    if( time < 0 ) {
      time = 0;
    }
    if( time > total ) {
      time = total;
    }
  }

  return time;
}

/* Queries whether a point lies within a position slider rectangle. */
BOOL
bmp_pt_in_slider( POINTL pos )
{
  if( cfg.mode != CFG_MODE_REGULAR ) {
    return FALSE;
  }

  if( pos.x >= bmp_pos[ POS_SLIDER ].x &&
      pos.x <= bmp_pos[ POS_SLIDER ].x + bmp_ulong[ UL_SLIDER_WIDTH ] + bmp_cx( BMP_SLIDER ) &&
      pos.y >= bmp_pos[ POS_SLIDER ].y - ( bmp_ulong[ UL_SHADE_SLIDER ] ? 1 : 0 ) &&
      pos.y <= bmp_pos[ POS_SLIDER ].y + ( bmp_ulong[ UL_SHADE_SLIDER ] ? 1 : 0 ) + bmp_cy( BMP_SLIDER ))
  {
    return TRUE;
  } else {
    return FALSE;
  }
}

/* Loads specified bitmap to the bitmap cache if it is
   not loaded before. */
static void
bmp_load_default( HPS hps, int id, int defid )
{ DEBUGLOG(("bmp_load_default(%p, %i, %i)\n", hps, id, defid));
  if( bmp_cache[ id ] == 0 && ( id == defid || bmp_cache[ defid ] != 0 ) )
  {
    HBITMAP hbitmap = GpiLoadBitmap( hps, NULLHANDLE, defid, 0, 0 );

    if( hbitmap != GPI_ERROR ) {
      bmp_cache[ id ] = hbitmap;
    }
    #ifdef DEBUG
     else
    { char buf[1024];
      os2pm_strerror(buf, sizeof(buf));
      if (*buf)
        DEBUGLOG(("bmp_load_default: GpiLoadBitmap failed: %s\n", buf));
    }
    #endif
  }
}
static inline void
bmp_load_default( HPS hps, int id )
{ bmp_load_default( hps, id, id );
}

/* Loads default bitmaps to the bitmap cache. */
static void
bmp_init_skins_bitmaps( HPS hps )
{
  int i;

  for( i = DIG_SMALL; i < DIG_SMALL + 10; i++ )
    bmp_load_default( hps, i );
    
  bmp_load_default( hps, DIG_BIG + 11, DIG_BIG + 10 );
  for( i = DIG_BIG; i < DIG_BIG + 12; i++ )
    bmp_load_default( hps, i );
  
  bmp_load_default( hps, DIG_TINY + 11, DIG_TINY );
  bmp_load_default( hps, DIG_TINY + 12, DIG_TINY + 10 );
  for( i = DIG_TINY; i < DIG_TINY + 13; i++ )
    bmp_load_default( hps, i );
  
  bmp_load_default( hps, DIG_TINY + 10, DIG_TINY );
  for( i = DIG_PL_INDEX; i < DIG_PL_INDEX + 11; i++ )
    bmp_load_default( hps, i );
  
  for( i = DIG_BPS; i < DIG_BPS + 10; i++ ) {
    bmp_load_default( hps, i );
  }

  bmp_load_default( hps, BMP_PLAY       );
  bmp_load_default( hps, BMP_PAUSE      );
  bmp_load_default( hps, BMP_REW        );
  bmp_load_default( hps, BMP_FWD        );
  bmp_load_default( hps, BMP_POWER      );
  bmp_load_default( hps, BMP_PREV       );
  bmp_load_default( hps, BMP_NEXT       );
  bmp_load_default( hps, BMP_SHUFFLE    );
  bmp_load_default( hps, BMP_REPEAT     );
  bmp_load_default( hps, BMP_PL         );
  bmp_load_default( hps, BMP_FLOAD      );

  bmp_load_default( hps, BMP_N_PLAY     );
  bmp_load_default( hps, BMP_N_PAUSE    );
  bmp_load_default( hps, BMP_N_REW      );
  bmp_load_default( hps, BMP_N_FWD      );
  bmp_load_default( hps, BMP_N_POWER    );
  bmp_load_default( hps, BMP_N_PREV     );
  bmp_load_default( hps, BMP_N_NEXT     );
  bmp_load_default( hps, BMP_N_SHUFFLE  );
  bmp_load_default( hps, BMP_N_REPEAT   );
  bmp_load_default( hps, BMP_N_PL       );
  bmp_load_default( hps, BMP_N_FLOAD    );

  for( i = BMP_FONT1; i <= BMP_FONT1 + 101; i++ ) {
    bmp_load_default( hps, i );
  }

  for( i = BMP_FONT2; i <= BMP_FONT2 + 50; i++ ) {
    bmp_load_default( hps, i );
  }

  bmp_load_default( hps, BMP_STEREO       );
  bmp_load_default( hps, BMP_MONO         );
  bmp_load_default( hps, BMP_NO_CHANNELS  );
  bmp_load_default( hps, BMP_SLIDER       );
  bmp_load_default( hps, BMP_S_BGROUND    );
  bmp_load_default( hps, BMP_T_BGROUND    );
  bmp_load_default( hps, BMP_VOLSLIDER    );
  bmp_load_default( hps, BMP_VOLBAR       );
  bmp_load_default( hps, BMP_SINGLEPLAY   );
  bmp_load_default( hps, BMP_LISTPLAY     );
  bmp_load_default( hps, BMP_NOFILE       );
  bmp_load_default( hps, BMP_LED          );
  bmp_load_default( hps, BMP_N_LED        );

  for( i = BMP_BPS; i < BMP_BPS + 16; i++ ) {
    bmp_load_default( hps, i );
  }

  bmp_load_default( hps, BMP_R_BGROUND    );
  bmp_load_default( hps, BMP_NOTL         );
  bmp_load_default( hps, BMP_TL           );
  bmp_load_default( hps, BMP_NOPLIST      );
  bmp_load_default( hps, BMP_PLIST        );
}

/* Initializes default bitmap positions. */
static void
bmp_init_skin_positions( void )
{
  bmp_pos[ POS_TIMER       ].x =       228; bmp_pos[ POS_TIMER       ].y =        48;
  bmp_pos[ POS_R_SIZE      ].x =       300; bmp_pos[ POS_R_SIZE      ].y =       110;
  bmp_pos[ POS_R_PLAY      ].x =         6; bmp_pos[ POS_R_PLAY      ].y =         8;
  bmp_pos[ POS_R_PAUSE     ].x =        29; bmp_pos[ POS_R_PAUSE     ].y =         8;
  bmp_pos[ POS_R_REW       ].x =        52; bmp_pos[ POS_R_REW       ].y =         8;
  bmp_pos[ POS_R_FWD       ].x =        75; bmp_pos[ POS_R_FWD       ].y =         8;
  bmp_pos[ POS_R_PL        ].x =       108; bmp_pos[ POS_R_PL        ].y =         8;
  bmp_pos[ POS_R_REPEAT    ].x =       165; bmp_pos[ POS_R_REPEAT    ].y =         8;
  bmp_pos[ POS_R_SHUFFLE   ].x =       188; bmp_pos[ POS_R_SHUFFLE   ].y =         8;
  bmp_pos[ POS_R_PREV      ].x =       211; bmp_pos[ POS_R_PREV      ].y =         8;
  bmp_pos[ POS_R_NEXT      ].x =       234; bmp_pos[ POS_R_NEXT      ].y =         8;
  bmp_pos[ POS_R_POWER     ].x =       270; bmp_pos[ POS_R_POWER     ].y =         8;
  bmp_pos[ POS_R_STOP      ].x = POS_UNDEF; bmp_pos[ POS_R_STOP      ].y = POS_UNDEF;
  bmp_pos[ POS_R_FLOAD     ].x = POS_UNDEF; bmp_pos[ POS_R_FLOAD     ].y = POS_UNDEF;
  bmp_pos[ POS_R_TEXT      ].x =        32; bmp_pos[ POS_R_TEXT      ].y =        84;
  bmp_pos[ POS_S_TEXT      ].x =        10; bmp_pos[ POS_S_TEXT      ].y =        41;
  bmp_pos[ POS_NOTL        ].x =       178; bmp_pos[ POS_NOTL        ].y =        62;
  bmp_pos[ POS_TL          ].x =       178; bmp_pos[ POS_TL          ].y =        62;
  bmp_pos[ POS_NOPLIST     ].x =       178; bmp_pos[ POS_NOPLIST     ].y =        47;
  bmp_pos[ POS_PLIST       ].x =       178; bmp_pos[ POS_PLIST       ].y =        47;
  bmp_pos[ POS_TIME_LEFT   ].x =       188; bmp_pos[ POS_TIME_LEFT   ].y =        62;
  bmp_pos[ POS_PL_LEFT     ].x =       188; bmp_pos[ POS_PL_LEFT     ].y =        47;
  bmp_pos[ POS_PL_MODE     ].x =       132; bmp_pos[ POS_PL_MODE     ].y =        47;
  bmp_pos[ POS_LED         ].x =         9; bmp_pos[ POS_LED         ].y =        92;
  bmp_pos[ POS_N_LED       ].x =         9; bmp_pos[ POS_N_LED       ].y =        92;
  bmp_pos[ POS_SLIDER      ].x =        32; bmp_pos[ POS_SLIDER      ].y =        40;
  bmp_pos[ POS_VOLBAR      ].x =         9; bmp_pos[ POS_VOLBAR      ].y =        38;
  bmp_pos[ POS_NO_CHANNELS ].x =       240; bmp_pos[ POS_NO_CHANNELS ].y =        72;
  bmp_pos[ POS_MONO        ].x =       240; bmp_pos[ POS_MONO        ].y =        72;
  bmp_pos[ POS_STEREO      ].x =       240; bmp_pos[ POS_STEREO      ].y =        72;
  bmp_pos[ POS_BPS         ].x =       259; bmp_pos[ POS_BPS         ].y =        72;
  bmp_pos[ POS_S_SIZE      ].x = POS_UNDEF; bmp_pos[ POS_S_SIZE      ].y = POS_UNDEF;
  bmp_pos[ POS_T_SIZE      ].x = POS_UNDEF; bmp_pos[ POS_T_SIZE      ].y = POS_UNDEF;
  bmp_pos[ POS_S_PLAY      ].x =         6; bmp_pos[ POS_S_PLAY      ].y =         8;
  bmp_pos[ POS_S_PAUSE     ].x =        29; bmp_pos[ POS_S_PAUSE     ].y =         8;
  bmp_pos[ POS_S_REW       ].x =        52; bmp_pos[ POS_S_REW       ].y =         8;
  bmp_pos[ POS_S_FWD       ].x =        75; bmp_pos[ POS_S_FWD       ].y =         8;
  bmp_pos[ POS_S_PL        ].x =       108; bmp_pos[ POS_S_PL        ].y =         8;
  bmp_pos[ POS_S_REPEAT    ].x =       165; bmp_pos[ POS_S_REPEAT    ].y =         8;
  bmp_pos[ POS_S_SHUFFLE   ].x =       188; bmp_pos[ POS_S_SHUFFLE   ].y =         8;
  bmp_pos[ POS_S_PREV      ].x =       211; bmp_pos[ POS_S_PREV      ].y =         8;
  bmp_pos[ POS_S_NEXT      ].x =       234; bmp_pos[ POS_S_NEXT      ].y =         8;
  bmp_pos[ POS_S_POWER     ].x =       270; bmp_pos[ POS_S_POWER     ].y =         8;
  bmp_pos[ POS_S_STOP      ].x = POS_UNDEF; bmp_pos[ POS_S_STOP      ].y = POS_UNDEF;
  bmp_pos[ POS_S_FLOAD     ].x = POS_UNDEF; bmp_pos[ POS_S_FLOAD     ].y = POS_UNDEF;
  bmp_pos[ POS_T_PLAY      ].x =         6; bmp_pos[ POS_T_PLAY      ].y =         8;
  bmp_pos[ POS_T_PAUSE     ].x =        29; bmp_pos[ POS_T_PAUSE     ].y =         8;
  bmp_pos[ POS_T_REW       ].x =        52; bmp_pos[ POS_T_REW       ].y =         8;
  bmp_pos[ POS_T_FWD       ].x =        75; bmp_pos[ POS_T_FWD       ].y =         8;
  bmp_pos[ POS_T_PL        ].x =       108; bmp_pos[ POS_T_PL        ].y =         8;
  bmp_pos[ POS_T_REPEAT    ].x =       165; bmp_pos[ POS_T_REPEAT    ].y =         8;
  bmp_pos[ POS_T_SHUFFLE   ].x =       188; bmp_pos[ POS_T_SHUFFLE   ].y =         8;
  bmp_pos[ POS_T_PREV      ].x =       211; bmp_pos[ POS_T_PREV      ].y =         8;
  bmp_pos[ POS_T_NEXT      ].x =       234; bmp_pos[ POS_T_NEXT      ].y =         8;
  bmp_pos[ POS_T_POWER     ].x =       270; bmp_pos[ POS_T_POWER     ].y =         8;
  bmp_pos[ POS_T_STOP      ].x = POS_UNDEF; bmp_pos[ POS_T_STOP      ].y = POS_UNDEF;
  bmp_pos[ POS_T_FLOAD     ].x = POS_UNDEF; bmp_pos[ POS_T_FLOAD     ].y = POS_UNDEF;
  bmp_pos[ POS_PL_INDEX    ].x =       143; bmp_pos[ POS_PL_INDEX    ].y =        62;
  bmp_pos[ POS_PL_TOTAL    ].x =       143; bmp_pos[ POS_PL_TOTAL    ].y =        47;
  bmp_pos[ POS_SLIDER_SHAFT].x = POS_UNDEF; bmp_pos[ POS_SLIDER_SHAFT].y = POS_UNDEF;
}

/* Loads default PM123 skin. */
static void
bmp_init_default_skin( HPS hps )
{
  VISUAL_PROPERTIES visual;
  char module_name[_MAX_PATH];
  int    i;

  bmp_pos[ POS_S_SIZE      ].x = 300; bmp_pos[ POS_S_SIZE      ].y = 70;
  bmp_pos[ POS_T_SIZE      ].x = 300; bmp_pos[ POS_T_SIZE      ].y = 37;
  bmp_pos[ POS_R_FLOAD     ].x = 131; bmp_pos[ POS_R_FLOAD     ].y =  8;
  bmp_pos[ POS_S_FLOAD     ].x = 131; bmp_pos[ POS_S_FLOAD     ].y =  8;
  bmp_pos[ POS_T_FLOAD     ].x = 131; bmp_pos[ POS_T_FLOAD     ].y =  8;
  bmp_pos[ POS_NO_CHANNELS ].x = 235; bmp_pos[ POS_NO_CHANNELS ].y = 72;
  bmp_pos[ POS_MONO        ].x = 235; bmp_pos[ POS_MONO        ].y = 72;
  bmp_pos[ POS_STEREO      ].x = 235; bmp_pos[ POS_STEREO      ].y = 72;

  for( i = BMP_FONT1 + 45; i < BMP_FONT1 + 51 + 127; i++ ) {
    bmp_load_default( hps, i );
  }

  for( i = BMP_FONT2 + 45; i < BMP_FONT2 + 51 + 127; i++ ) {
    bmp_load_default( hps, i );
  }

  bmp_ulong[ UL_PL_INDEX     ] = 4;
  bmp_ulong[ UL_IN_PIXELS    ] = TRUE;
  bmp_ulong[ UL_R_MSG_LEN    ] = 256;
  bmp_ulong[ UL_R_MSG_HEIGHT ] = 16;
  bmp_ulong[ UL_S_MSG_LEN    ] = 276;
  bmp_ulong[ UL_S_MSG_HEIGHT ] = 16;
  bmp_ulong[ UL_FG_MSG_COLOR ] = DEF_FG_MSG_COLOR;
  bmp_ulong[ UL_FG_COLOR     ] = DEF_FG_COLOR;    
  bmp_ulong[ UL_BG_COLOR     ] = DEF_BG_COLOR;    
  bmp_ulong[ UL_HI_FG_COLOR  ] = DEF_HI_FG_COLOR; 
  bmp_ulong[ UL_HI_BG_COLOR  ] = DEF_HI_BG_COLOR;;
  bmp_ulong[ UL_BPS_DIGITS   ] = TRUE;

  strcpy( visual.param, "" );

  visual.skin = TRUE;
  visual.x    = 32;
  visual.y    = 49;
  visual.cx   = 95;
  visual.cy   = 30;
  strlcpy( visual.param, visual_param, sizeof visual.param );

  strlcpy( module_name, startpath, sizeof module_name );
  strlcat( module_name, "visplug\\analyzer.dll", sizeof module_name );
  add_plugin( module_name, &visual );
}

static void                                                                  
bmp_init_colors( void )                                                      
{                                                                            
/* TODO: currently unsupported
  pl_set_colors( bmp_ulong[ UL_FG_COLOR    ], bmp_ulong[ UL_BG_COLOR    ],   
                 bmp_ulong[ UL_HI_FG_COLOR ], bmp_ulong[ UL_HI_BG_COLOR ] ); 
  pm_set_colors( bmp_ulong[ UL_FG_COLOR    ], bmp_ulong[ UL_BG_COLOR    ],   
                 bmp_ulong[ UL_HI_FG_COLOR ], bmp_ulong[ UL_HI_BG_COLOR ] ); 
  bm_set_colors( bmp_ulong[ UL_FG_COLOR    ], bmp_ulong[ UL_BG_COLOR    ],   
                 bmp_ulong[ UL_HI_FG_COLOR ], bmp_ulong[ UL_HI_BG_COLOR ] );
*/ 
}

static int bmp_map_bitmap_id( int i )
{
  static const int button_map[] = {
    BMP_PLAY, 0, BMP_PAUSE, BMP_REW, BMP_FWD, BMP_POWER, BMP_PREV, BMP_NEXT, BMP_SHUFFLE, BMP_REPEAT,
    BMP_N_PLAY, 0, BMP_N_PAUSE, BMP_N_REW, BMP_N_FWD, BMP_N_POWER, BMP_N_PREV, BMP_N_NEXT, BMP_N_SHUFFLE, BMP_N_REPEAT,
    BMP_N_PL, BMP_PL, BMP_N_STOP, BMP_STOP, BMP_N_FLOAD, BMP_FLOAD };
  static const int misc1600_map[] = {
    0, BMP_STEREO, BMP_MONO, 0, 0, 0, 0, 0, BMP_NO_CHANNELS, 0,
    0, BMP_S_BGROUND, BMP_T_BGROUND, 0, 0, 0, 0, 0, 0, 0,
    BMP_VOLSLIDER, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    BMP_VOLBAR, BMP_SINGLEPLAY, BMP_LISTPLAY, BMP_NOFILE };
  static const int misc1900_map[] = {
    BMP_R_BGROUND, BMP_NOTL, BMP_TL, BMP_NOPLIST, BMP_PLIST, 0, BMP_SLIDER_SHAFT };

  switch ( i )
  { case 1820: return BMP_LED;
    case 1821: return BMP_N_LED;
  }
  if (i >= 1100 && i <= 1119)
    return i + DIG_SMALL-1100;
  if (i >= 1200 && i <= 1219)
    return i += DIG_BIG-1200;
  if (i >= 1300 && i < 1300 + sizeof button_map / sizeof *button_map)
    return button_map[i-1300]; 
  if (i >= 1600 && i < 1600 + sizeof misc1600_map / sizeof *misc1600_map)
    return misc1600_map[i-1600];
  if (i >= 1640 && i <= 1659)
    return DIG_TINY-1640;
  if (i >= 1660 && i <= 1679)
    return i + DIG_PL_INDEX-1640;
  if (i >= 1400 && i <= 1499)
    return i + BMP_FONT1-1400;
  if (i >= 1800 && i <= 1819)
    return i + DIG_BPS-1800;
  if (i >= 1830 && i <= 1839)
    return i + DIG_BPS-1830;
  if (i >= 1900 && i < 1900 + sizeof misc1900_map / sizeof *misc1900_map)
    return misc1900_map[i-1600];
  if (i >= 4400 && i <= 4499)
    return i + BMP_FONT2-4400;
  if (i >= 5500 && i < 5000 + sizeof button_map / sizeof *button_map)
    return button_map[i-5500] + BMP_S_PLAY - BMP_PLAY;
  return 0;
}                                                                            

/* Returns TRUE if specified mode supported by current skin. */
BOOL
bmp_is_mode_supported( int mode )
{
  switch( mode )
  {
    case CFG_MODE_REGULAR:
      return bmp_pos[ POS_R_SIZE ].x != POS_UNDEF && bmp_pos[ POS_R_SIZE ].y != POS_UNDEF;
    case CFG_MODE_SMALL:
      return bmp_pos[ POS_S_SIZE ].x != POS_UNDEF && bmp_pos[ POS_S_SIZE ].y != POS_UNDEF;
    case CFG_MODE_TINY:
      return bmp_pos[ POS_T_SIZE ].x != POS_UNDEF && bmp_pos[ POS_T_SIZE ].y != POS_UNDEF;
  }

  return FALSE;
}

/* Returns TRUE if specified font supported by current skin. */
BOOL
bmp_is_font_supported( int font )
{
  if( font != 0 && bmp_ulong[ UL_ONE_FONT ] ) {
    return FALSE;
  } else {
    return TRUE;
  }
}

/* Loads specified bitmaps bundle. */
static BOOL
bmp_load_packfile( char *filename )
{
  FILE*     pack;
  FILE*     temp;
  char*     fbuf;
  char      tempname[_MAX_PATH];
  char      tempexts[_MAX_EXT ];
  int       cb;
  BUNDLEHDR hdr;

  pack = fopen( filename, "rb" );
  if( pack == NULL ) {
    amp_player_error( "The bitmap bundle file, %s, for this skin was not found. "
                      "Skin display as incomplete.", filename );
    return FALSE;
  }

  while( !feof( pack ))
  {
    if( fread( &hdr, 1, sizeof( BUNDLEHDR ), pack ) == sizeof( BUNDLEHDR ) &&
        hdr.length > 0 )
    { if(( fbuf = (char*)malloc( hdr.length )) != NULL )
      {
        cb = fread( fbuf, 1, hdr.length, pack );
        
        // decouple from internal IDs
        hdr.resource = bmp_map_bitmap_id( hdr.resource );
        if ( hdr.resource == 0 )
          continue;

        sprintf( tempname, "%spm123%s", startpath,
                           sfext( tempexts, hdr.filename, sizeof( tempexts )));

        if(( temp = fopen( tempname, "wb" )) != NULL )
        {
          fwrite( fbuf, 1, cb, temp );
          fclose( temp );


          bmp_cache[ hdr.resource ] = bmp_load_bitmap( tempname );
          remove( tempname );
        }
        else
        {
          amp_player_error( "Unable create temporary bitmap file %s\n", tempname );
          free  ( fbuf );
          fclose( pack );
          return FALSE;
        }
        free( fbuf );
      }
      else
      {
        fclose( pack );
        amp_player_error( "Out of memory" );
        return FALSE;
      }
    }
  }

  fclose( pack );
  return TRUE;
}

/* Deallocates all resources used by current loaded skin. */
void
bmp_clean_skin( void )
{
  unsigned int i;

  for( i = 0; i < sizeof( bmp_cache ) / sizeof( HBITMAP ); i++ ) {
    if( bmp_cache[i] != NULLHANDLE )
    {
      GpiDeleteBitmap( bmp_cache[i] );
      bmp_cache[i] = NULLHANDLE;
    }
  }

  memset( &bmp_pos,   0, sizeof( bmp_pos   ));
  memset( &bmp_ulong, 0, sizeof( bmp_ulong ));

  bmp_delete_text_buffer();
}

/* Loads specified skin. */
BOOL
bmp_load_skin( const char *filename, HAB hab, HWND hplayer, HPS hps )
{
  unsigned int   i;
  FILE* file;
  char  line[256];
  char  path[_MAX_PATH];
  int   errors = 0;
  BOOL  empty  = TRUE;

  sdrivedir( path, filename, sizeof( path ));

  file = fopen( filename, "r" );
  if( !file && strlen( filename ) > 0 ) {
    amp_player_error( "Unable open skin %s, %s", filename, strerror( errno ));
  }

  // Free loaded visual plugins.
  remove_visual_plugins( TRUE );

  bmp_clean_skin();
  bmp_init_skin_positions();
  *visual_param = 0;

  bmp_ulong[ UL_SHADE_BRIGHT   ] = 0x00FFFFFFUL;
  bmp_ulong[ UL_SHADE_DARK     ] = 0x00404040UL;
  bmp_ulong[ UL_SLIDER_COLOR   ] = 0x00007F00UL;
  bmp_ulong[ UL_SHADE_STAT     ] = TRUE;
  bmp_ulong[ UL_SHADE_SLIDER   ] = TRUE;
  bmp_ulong[ UL_SHADE_PLAYER   ] = TRUE;
  bmp_ulong[ UL_SHADE_VOLUME   ] = TRUE;
  bmp_ulong[ UL_SLIDER_WIDTH   ] = 242;
  bmp_ulong[ UL_TIMER_SPACE    ] = 1;
  bmp_ulong[ UL_TIMER_SEPARATE ] = TRUE;
  bmp_ulong[ UL_VOLUME_SLIDER  ] = FALSE;
  bmp_ulong[ UL_VOLUME_HRZ     ] = FALSE;
  bmp_ulong[ UL_BPS_DIGITS     ] = FALSE;
  bmp_ulong[ UL_PL_COLOR       ] = 0x0000FF00UL;
  bmp_ulong[ UL_R_MSG_LEN      ] = 25;
  bmp_ulong[ UL_R_MSG_HEIGHT   ] = 0;
  bmp_ulong[ UL_S_MSG_LEN      ] = 25;
  bmp_ulong[ UL_S_MSG_HEIGHT   ] = 0;
  bmp_ulong[ UL_FG_MSG_COLOR   ] = 0x00FFFFFFUL;
  bmp_ulong[ UL_FG_COLOR       ] = 0xFFFFFFFFUL;
  bmp_ulong[ UL_BG_COLOR       ] = 0xFFFFFFFFUL;
  bmp_ulong[ UL_HI_FG_COLOR    ] = 0xFFFFFFFFUL;
  bmp_ulong[ UL_HI_BG_COLOR    ] = 0xFFFFFFFFUL;
  bmp_ulong[ UL_PL_INDEX       ] = 0;
  bmp_ulong[ UL_FONT           ] = 0;
  bmp_ulong[ UL_ONE_FONT       ] = FALSE;
  bmp_ulong[ UL_IN_PIXELS      ] = FALSE;

  if( !file )
  {
    bmp_init_skins_bitmaps( hps );
    bmp_init_default_skin ( hps );
    bmp_init_colors();
    bmp_reflow_and_resize ( WinQueryWindow( hplayer, QW_PARENT ));

    vis_init_all( TRUE );

    return FALSE;
  }

  while( !feof( file ) &&  fgets( line, 256, file ))
  {
    blank_strip( line );

    if( *line == '#' || *line == ';' ) {
      continue;
    }

    for( i = 0; line[i]; i++ ) {
      if( strchr( ",:=", line[i] )) {
        break;
      }
    }

    switch( line[i] ) {
      case '=': // plug-in
      {
        VISUAL_PROPERTIES visual;
        char module_name[_MAX_PATH];
        char*  p = strtok( line + i + 1, "," );
        char   param[_MAX_PATH];
        struct stat fi;

        if( p == NULL ) {
          break;
        }

        rel2abs( startpath, p,
                 module_name, sizeof( module_name ));

        if(( p = strtok( NULL, "," )) != NULL ) {
          visual.x  = atoi(p);
        }
        if(( p = strtok( NULL, "," )) != NULL ) {
          visual.y  = atoi(p);
        }
        if(( p = strtok( NULL, "," )) != NULL ) {
          visual.cx = atoi(p);
        }
        if(( p = strtok( NULL, "," )) != NULL ) {
          visual.cy = atoi(p);
        }
        if(( p = strtok( NULL, "," )) != NULL ) {
          rel2abs( path, p, param, sizeof( param ));
          DEBUGLOG(("bmp_load_skin: %s => %s\n", p, param));
          if( stat( param, &fi ) == 0 ) {
            strlcpy( visual_param, param, sizeof visual_param );
          } else {
            strlcpy( visual_param, p, sizeof visual_param );
          }
        }

        visual.skin = TRUE;
        strlcpy( visual.param, visual_param, sizeof visual.param );
        add_plugin( module_name, &visual );
        break;
      }

      case ':': // position
      {
        int x = POS_UNDEF;
        int y = POS_UNDEF;
        int i =  0;

        sscanf( line, "%d:%d,%d", &i, &x, &y );
        if (i < POS__MAX)
        { bmp_pos[ i ].x = x;
          bmp_pos[ i ].y = y;
        }
        break;
      }

      case ',': // bitmap
      {
        char* p = line + i + 1;
        int   i = atoi( line );
        int   r, g, b;

        if( i == UL_SHADE_BRIGHT  ||
            i == UL_SHADE_DARK    ||
            i == UL_SLIDER_BRIGHT ||
            i == UL_SLIDER_COLOR  ||
            i == UL_PL_COLOR      ||
            i == UL_FG_COLOR      ||
            i == UL_BG_COLOR      ||
            i == UL_HI_FG_COLOR   ||
            i == UL_HI_BG_COLOR   ||
            i == UL_FG_MSG_COLOR   )
        {
          sscanf( p, "%d/%d/%d", &r, &g, &b );
          bmp_ulong[ i ] = r << 16 | g << 8 | b;
          break;
        }
        switch( i ) {
          case UL_SHADE_STAT:    bmp_ulong[ UL_SHADE_STAT    ] = FALSE;   break;
          case UL_SHADE_VOLUME:  bmp_ulong[ UL_SHADE_VOLUME  ] = FALSE;   break;

          case UL_DISPLAY_MSG:
            // Not supported since 1.32
            // if( amp_playmode == AMP_NOFILE ) {
            //  bmp_set_text( p );
            // }
            break;

          case UL_TIMER_SEPSPACE:
            // Not supported since 1.32
            // bmp_ulong[ UL_TIMER_SEPSPACE ] = atoi(p);
            break;

          case UL_SHADE_PLAYER:   bmp_ulong[ UL_SHADE_PLAYER   ] = FALSE;   break;
          case UL_SHADE_SLIDER:   bmp_ulong[ UL_SHADE_SLIDER   ] = FALSE;   break;
          case UL_R_MSG_LEN:      bmp_ulong[ UL_R_MSG_LEN      ] = atoi(p); break;
          case UL_SLIDER_WIDTH:   bmp_ulong[ UL_SLIDER_WIDTH   ] = atoi(p); break;
          case UL_S_MSG_LEN:      bmp_ulong[ UL_S_MSG_LEN      ] = atoi(p); break;
          case UL_FONT:           cfg.font = atoi(p); break;
          case UL_TIMER_SPACE:    bmp_ulong[ UL_TIMER_SPACE    ] = atoi(p); break;
          case UL_TIMER_SEPARATE: bmp_ulong[ UL_TIMER_SEPARATE ] = FALSE;   break;
          case UL_VOLUME_HRZ:     bmp_ulong[ UL_VOLUME_HRZ     ] = TRUE;    break;
          case UL_VOLUME_SLIDER:  bmp_ulong[ UL_VOLUME_SLIDER  ] = TRUE;    break;
          case UL_BPS_DIGITS:     bmp_ulong[ UL_BPS_DIGITS     ] = TRUE;    break;
          case UL_PL_INDEX:       bmp_ulong[ UL_PL_INDEX       ] = atoi(p); break;
          case UL_ONE_FONT:       bmp_ulong[ UL_ONE_FONT       ] = TRUE;    break;       
          case UL_IN_PIXELS:      bmp_ulong[ UL_IN_PIXELS      ] = TRUE;    break;
          case UL_R_MSG_HEIGHT:   bmp_ulong[ UL_R_MSG_HEIGHT   ] = atoi(p); break;
          case UL_S_MSG_HEIGHT:   bmp_ulong[ UL_S_MSG_HEIGHT   ] = atoi(p); break;

          case UL_BUNDLE:
          {
            char bundle[_MAX_PATH];

            rel2abs( path, p, bundle, sizeof( bundle ));
            bmp_load_packfile( bundle );
            break;
          }

          default:
          { char image[_MAX_PATH];
            // decouple skin IDs from internal IDs
            i = bmp_map_bitmap_id(i);
            if (i)
            { rel2abs( path, p, image, sizeof( image ));
              bmp_cache[ i ] = bmp_load_bitmap( image );
              if( bmp_cache[ i ] == NULLHANDLE ) {
                ++errors;
              }
            }
            break;
          }
        }
      }
    }
  }

  fclose( file );

  DEBUGLOG(("bmp_load_skin: plugin loaded\n"));

  for( i = 0; i < sizeof( bmp_cache ) / sizeof( HBITMAP ); i++ ) {
    if( bmp_cache[ i ] ) {
      empty = FALSE;
      break;
    }
  }

  if( empty ) {
    bmp_init_default_skin( hps );
  }

  if( cfg.mode != CFG_MODE_REGULAR && !bmp_is_mode_supported( cfg.mode )) {
    cfg.mode = CFG_MODE_REGULAR;
  }
  if( bmp_ulong[ UL_ONE_FONT ] ) {
    cfg.font = 0;
  }

  bmp_init_skins_bitmaps( hps );
  bmp_init_colors();
  bmp_reflow_and_resize ( WinQueryWindow( hplayer, QW_PARENT ));

  if( errors > 0 ) {
    if( !amp_query( hplayer, "Some bitmaps of this skin was not found. "
                             "Would you like to continue the loading of this skin? "
                             "(if you select No, default skin will be used)" ))
    {
      strcpy( cfg.defskin, "" );
      return bmp_load_skin( "", hab, hplayer, hps );
    }
  }

  return TRUE;
}

/* Initializes specified skin button. */
static void
bmp_init_button( HWND hwnd, BMPBUTTON* button )
{ DEBUGLOG(("bmp_init_button(%p, %p{%p, %i,%i, %i, %i,%i, %i, %i, %i, %i, %s})\n",
    hwnd, button, button->handle, button->id_r_pressed, button->id_r_release, button->id_r_pos,
    button->id_s_pressed, button->id_s_release, button->id_s_pos, button->id_t_pos, button->state, button->sticky, button->help));
  DATA95 btn_data;
  int    x, y, cx, cy;
  int    pressed;
  int    release;

  switch( cfg.mode )
  {
    case CFG_MODE_REGULAR:

      x       = bmp_pos[ button->id_r_pos ].x;
      y       = bmp_pos[ button->id_r_pos ].y;
      pressed = button->id_r_pressed;
      release = button->id_r_release;
      cx      = bmp_cx( pressed );
      cy      = bmp_cy( pressed );
      break;

    case CFG_MODE_SMALL:

      x       = bmp_pos[ button->id_s_pos ].x;
      y       = bmp_pos[ button->id_s_pos ].y;
      pressed = bmp_cache[ button->id_s_pressed ] ? button->id_s_pressed : button->id_r_pressed;
      release = bmp_cache[ button->id_s_release ] ? button->id_s_release : button->id_r_release;
      cx      = bmp_cx( pressed );
      cy      = bmp_cy( pressed );
      break;

    default:

      x       = bmp_pos  [ button->id_t_pos ].x;
      y       = bmp_pos  [ button->id_t_pos ].y;
      pressed = bmp_cache[ button->id_s_pressed ] ? button->id_s_pressed : button->id_r_pressed;
      release = bmp_cache[ button->id_s_release ] ? button->id_s_release : button->id_r_release;
      cx      = bmp_cx( pressed );
      cy      = bmp_cy( pressed );
      break;
  }

  if( x != POS_UNDEF && y != POS_UNDEF ) {
    if( button->handle == NULLHANDLE )
    {
      btn_data.cb        =  sizeof( DATA95 );
      btn_data.pressed   =  0;
      btn_data.bmp_release_id = bmp_cache + release;
      btn_data.bmp_pressed_id = bmp_cache + pressed;
      btn_data.stick     =  button->sticky;
      btn_data.stickvar  = &button->state;
      btn_data.hwnd_owner =  hwnd;

      strcpy( btn_data.help, button->help );
      button->handle = WinCreateWindow( hwnd, CLASSNAME, "", WS_VISIBLE, x, y, cx, cy,
                                        hwnd, HWND_TOP, button->id_r_pressed, &btn_data, NULL );
    }
    else
    {
      WinSetWindowPos( button->handle, HWND_TOP, x, y, cx, cy, SWP_MOVE | SWP_SIZE );
      WinSendMsg( button->handle, WM_CHANGEBMP, MPFROMP( bmp_cache + release ),
                                                MPFROMP( bmp_cache + pressed ));
    }
    //DEBUGLOG(("bmp_init_button: after set bitmap %p %p\n", bmp_cache[release], bmp_cache[pressed]));
  }
  else
  {
    if( button->handle != NULLHANDLE ) {
      WinDestroyWindow( button->handle );
      button->handle = NULLHANDLE;
    }
  }
}

/* Adjusts current skin to the selected size of the player window. */
void
bmp_reflow_and_resize( HWND hframe )
{
  DEBUGLOG(("bmp_reflow_and_resize(%p)\n", hframe));

  HWND hplayer = WinWindowFromID( hframe, FID_CLIENT );

  switch( cfg.mode )
  {
    case CFG_MODE_SMALL:
    {
      vis_deinit_all(TRUE);

      WinSetWindowPos( hframe, HWND_TOP, 0, 0,
                       bmp_pos[POS_S_SIZE].x, bmp_pos[POS_S_SIZE].y, SWP_SIZE );
      break;
    }

    case CFG_MODE_TINY:
    {
      vis_deinit_all(TRUE);

      WinSetWindowPos( hframe, HWND_TOP, 0, 0,
                       bmp_pos[POS_T_SIZE].x, bmp_pos[POS_T_SIZE].y, SWP_SIZE );
      break;
    }

    default:
    {
      WinSetWindowPos( hframe, HWND_TOP, 0, 0,
                       bmp_pos[POS_R_SIZE].x, bmp_pos[POS_R_SIZE].y, SWP_SIZE );

      vis_init_all(TRUE);

      break;
    }
  }

  bmp_init_button( hplayer, &btn_play    );
  bmp_init_button( hplayer, &btn_pause   );
  bmp_init_button( hplayer, &btn_rew     );
  bmp_init_button( hplayer, &btn_fwd     );
  bmp_init_button( hplayer, &btn_pl      );
  bmp_init_button( hplayer, &btn_repeat  );
  bmp_init_button( hplayer, &btn_shuffle );
  bmp_init_button( hplayer, &btn_prev    );
  bmp_init_button( hplayer, &btn_next    );
  bmp_init_button( hplayer, &btn_power   );
  bmp_init_button( hplayer, &btn_stop    );
  bmp_init_button( hplayer, &btn_fload   );

  bmp_delete_text_buffer();
  amp_invalidate( UPD_WINDOW | UPD_FILENAME );
}

