/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
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

/* PM123 Skin Utility */

#define  INCL_PM
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <gbm.h>
#include <gbmrect.h>
#include <utilfct.h>
#include <gui/skin.h>
#include <copyright.h>

static char suberror[256];

/* Prototypes */
void dtlf( char* );
int  cropimage( char*, char*, int, int, int, int );
int  infoimage( char*, GBM* );
int  skin_bundle( char*, char* );
int  skin_convert( char* );

typedef struct
{
  int           resource;
  char          filename[128];
  unsigned long length;

} bundle_hdr;

#define CHECK(x) if (!(x)) { printf("\n\x1b[1mError\x1b[0m: %s\n", suberror); return 0; }
#define DOT      printf( "." )
#define TSAR(x)  putchar( x )

int
fexists( char *filename )
{
  struct stat z;
  if( stat( filename, &z ) == -1 ) {
    return 0;
  } else {
    return 1;
  }
}

char*
concat( char *st, char *nd )
{
  static char concatbuf[_MAX_PATH];

  strcpy( concatbuf, st );
  strcat( concatbuf, nd );

  return concatbuf;
}

int
skin_convert( char *src )
{
  char   dir [_MAX_PATH];
  char   file[_MAX_PATH];
  FILE*  skin;
  FILE*  f01;
  FILE*  f02;
  int    have_specana = 0;
  POINTL pos[1024];
  ULONG  flg[1024];
  GBM    gbm;

  memset( &pos, 0, sizeof( pos ));
  memset( &flg, 0, sizeof( flg ));
  strcpy( dir, src );

  if( dir[ strlen(dir) - 1 ] == '\\' ) {
    dir[ strlen(dir) - 1 ] = '\\';
  }
  if( dir[ strlen(dir) - 1 ] != '\\' && dir[ strlen(dir) - 1 ] != '/' ) {
    strcat( dir, "\\" );
  }

  sprintf( file, "%smain.bmp", dir );
  if( !fexists( file )) {
    printf( "%s: File doesn't exists. The directory doesn't contain a WinAmp skin.\n", file );
    return 0;
  }

  pos[ POS_R_SIZE ].x = 275; pos[ POS_R_SIZE ].y = 116;

  sprintf( file, "%scbuttons.bmp", dir );
  printf ( "Processing %s.", file );

  DOT;
  CHECK( cropimage( file, concat( dir, "prevup.bmp"  ),   0, 18, 23, 18 ));
  CHECK( cropimage( file, concat( dir, "prevdn.bmp"  ),   0,  0, 23, 18 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "playup.bmp"  ),  23, 18, 23, 18 ));
  CHECK( cropimage( file, concat( dir, "playdn.bmp"  ),  23,  0, 23, 18 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "pauseup.bmp" ),  46, 18, 23, 18 ));
  CHECK( cropimage( file, concat( dir, "pausedn.bmp" ),  46,  0, 23, 18 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "nextup.bmp"  ),  92, 18, 22, 18 ));
  CHECK( cropimage( file, concat( dir, "nextdn.bmp"  ),  92,  0, 22, 18 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "stopup.bmp"  ),  69, 18, 23, 18 ));
  CHECK( cropimage( file, concat( dir, "stopdn.bmp"  ),  69,  0, 23, 18 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "loadup.bmp"  ), 114, 20, 22, 16 ));
  CHECK( cropimage( file, concat( dir, "loaddn.bmp"  ), 114,  4, 22, 16 ));

  pos[ POS_R_PREV  ].x =  16; pos[ POS_R_PREV  ].y = 10;
  pos[ POS_R_PLAY  ].x =  39; pos[ POS_R_PLAY  ].y = 10;
  pos[ POS_R_PAUSE ].x =  62; pos[ POS_R_PAUSE ].y = 10;
  pos[ POS_R_REW   ].x =  -1; pos[ POS_R_REW   ].y = -1;
  pos[ POS_R_FWD   ].x =  -1; pos[ POS_R_FWD   ].y = -1;
  pos[ POS_R_NEXT  ].x = 108; pos[ POS_R_NEXT  ].y = 10;
  pos[ POS_R_STOP  ].x =  85; pos[ POS_R_STOP  ].y = 10;
  pos[ POS_R_FLOAD ].x = 136; pos[ POS_R_FLOAD ].y = 11;

  printf ( "\n" );
  sprintf( file, "%stitlebar.bmp", dir );
  printf ( "Processing %s.", file );

  DOT;
  CHECK( cropimage( file, concat( dir, "tbd.bmp"     ),  27, 58, 275, 14 ));
  CHECK( cropimage( file, concat( dir, "tb.bmp"      ),  27, 73, 275, 14 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "stb.bmp"     ),  27, 44, 275, 14 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "sprevup.bmp" ), 195, 46,   8, 10 ));
  CHECK( cropimage( file, concat( dir, "sprevdn.bmp" ), 195, 46,   8, 10 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "splayup.bmp" ), 203, 46,   9, 10 ));
  CHECK( cropimage( file, concat( dir, "splaydn.bmp" ), 203, 46,   9, 10 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "spauseup.bmp"), 213, 46,   9, 10 ));
  CHECK( cropimage( file, concat( dir, "spausedn.bmp"), 213, 46,   9, 10 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "sstopup.bmp" ), 222, 46,   9, 10 ));
  CHECK( cropimage( file, concat( dir, "sstopdn.bmp" ), 222, 46,   9, 10 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "snextup.bmp" ), 231, 46,  11, 10 ));
  CHECK( cropimage( file, concat( dir, "snextdn.bmp" ), 231, 46,  11, 10 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "sloadup.bmp" ), 242, 46,  10, 10 ));
  CHECK( cropimage( file, concat( dir, "sloaddn.bmp" ), 242, 46,  10, 10 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "pwrup.bmp"   ),  18, 78,   1,  1 ));
  CHECK( cropimage( file, concat( dir, "pwrdn.bmp"   ),  18, 69,   9,  9 ));

  pos[ POS_LED     ].x =   0; pos[ POS_LED     ].y = 102;
  pos[ POS_N_LED   ].x =   0; pos[ POS_N_LED   ].y = 102;
  pos[ POS_R_POWER ].x = 264; pos[ POS_R_POWER ].y = 104;
  pos[ POS_T_SIZE  ].x =  -1; pos[ POS_T_SIZE  ].y =  -1;
  pos[ POS_T_PREV  ].x =  -1; pos[ POS_T_PREV  ].y =  -1;
  pos[ POS_T_PLAY  ].x =  -1; pos[ POS_T_PLAY  ].y =  -1;
  pos[ POS_T_PAUSE ].x =  -1; pos[ POS_T_PAUSE ].y =  -1;
  pos[ POS_T_STOP  ].x =  -1; pos[ POS_T_STOP  ].y =  -1;
  pos[ POS_T_REW   ].x =  -1; pos[ POS_T_REW   ].y =  -1;
  pos[ POS_T_FWD   ].x =  -1; pos[ POS_T_FWD   ].y =  -1;
  pos[ POS_T_NEXT  ].x =  -1; pos[ POS_T_NEXT  ].y =  -1;
  pos[ POS_T_FLOAD ].x =  -1; pos[ POS_T_FLOAD ].y =  -1;
  pos[ POS_T_POWER ].x =  -1; pos[ POS_T_POWER ].y =  -1;
  pos[ POS_S_SIZE  ].x = 275; pos[ POS_S_SIZE  ].y =  14;
  pos[ POS_S_PREV  ].x = 169; pos[ POS_S_PREV  ].y =   2;
  pos[ POS_S_PLAY  ].x = 177; pos[ POS_S_PLAY  ].y =   2;
  pos[ POS_S_PAUSE ].x = 187; pos[ POS_S_PAUSE ].y =   2;
  pos[ POS_S_STOP  ].x = 196; pos[ POS_S_STOP  ].y =   2;
  pos[ POS_S_REW   ].x =  -1; pos[ POS_S_REW   ].y =  -1;
  pos[ POS_S_FWD   ].x =  -1; pos[ POS_S_FWD   ].y =  -1;
  pos[ POS_S_NEXT  ].x = 205; pos[ POS_S_NEXT  ].y =   2;
  pos[ POS_S_FLOAD ].x = 216; pos[ POS_S_FLOAD ].y =   2;
  pos[ POS_S_POWER ].x = 264; pos[ POS_S_POWER ].y =   2;

  printf ( "\n" );
  sprintf( file, "%stext.bmp", dir );
  printf ( "Processing %s.", file );

  CHECK( infoimage( file, &gbm ));
  TSAR ('a');
  CHECK( cropimage( file, concat( dir, "a.bmp"      ),   0, gbm.h -  6, 5, 6 ));
  TSAR ('b');
  CHECK( cropimage( file, concat( dir, "b.bmp"      ),   5, gbm.h -  6, 5, 6 ));
  TSAR ('c');
  CHECK( cropimage( file, concat( dir, "c.bmp"      ),  10, gbm.h -  6, 5, 6 ));
  TSAR ('d');
  CHECK( cropimage( file, concat( dir, "d.bmp"      ),  15, gbm.h -  6, 5, 6 ));
  TSAR ('e');
  CHECK( cropimage( file, concat( dir, "e.bmp"      ),  20, gbm.h -  6, 5, 6 ));
  TSAR ('f');
  CHECK( cropimage( file, concat( dir, "f.bmp"      ),  25, gbm.h -  6, 5, 6 ));
  TSAR ('g');
  CHECK( cropimage( file, concat( dir, "g.bmp"      ),  30, gbm.h -  6, 5, 6 ));
  TSAR ('h');
  CHECK( cropimage( file, concat( dir, "h.bmp"      ),  35, gbm.h -  6, 5, 6 ));
  TSAR ('i');
  CHECK( cropimage( file, concat( dir, "i.bmp"      ),  40, gbm.h -  6, 5, 6 ));
  TSAR ('j');
  CHECK( cropimage( file, concat( dir, "j.bmp"      ),  45, gbm.h -  6, 5, 6 ));
  TSAR ('k');
  CHECK( cropimage( file, concat( dir, "k.bmp"      ),  50, gbm.h -  6, 5, 6 ));
  TSAR ('l');
  CHECK( cropimage( file, concat( dir, "l.bmp"      ),  55, gbm.h -  6, 5, 6 ));
  TSAR ('m');
  CHECK( cropimage( file, concat( dir, "m.bmp"      ),  60, gbm.h -  6, 5, 6 ));
  TSAR ('n');
  CHECK( cropimage( file, concat( dir, "n.bmp"      ),  65, gbm.h -  6, 5, 6 ));
  TSAR ('o');
  CHECK( cropimage( file, concat( dir, "o.bmp"      ),  70, gbm.h -  6, 5, 6 ));
  TSAR ('p');
  CHECK( cropimage( file, concat( dir, "p.bmp"      ),  75, gbm.h -  6, 5, 6 ));
  TSAR ('q');
  CHECK( cropimage( file, concat( dir, "q.bmp"      ),  80, gbm.h -  6, 5, 6 ));
  TSAR ('r');
  CHECK( cropimage( file, concat( dir, "r.bmp"      ),  85, gbm.h -  6, 5, 6 ));
  TSAR ('s');
  CHECK( cropimage( file, concat( dir, "s.bmp"      ),  90, gbm.h -  6, 5, 6 ));
  TSAR ('t');
  CHECK( cropimage( file, concat( dir, "t.bmp"      ),  95, gbm.h -  6, 5, 6 ));
  TSAR ('u');
  CHECK( cropimage( file, concat( dir, "u.bmp"      ), 100, gbm.h -  6, 5, 6 ));
  TSAR ('v');
  CHECK( cropimage( file, concat( dir, "v.bmp"      ), 105, gbm.h -  6, 5, 6 ));
  TSAR ('w');
  CHECK( cropimage( file, concat( dir, "w.bmp"      ), 110, gbm.h -  6, 5, 6 ));
  TSAR ('x');
  CHECK( cropimage( file, concat( dir, "x.bmp"      ), 115, gbm.h -  6, 5, 6 ));
  TSAR ('y');
  CHECK( cropimage( file, concat( dir, "y.bmp"      ), 120, gbm.h -  6, 5, 6 ));
  TSAR ('z');
  CHECK( cropimage( file, concat( dir, "z.bmp"      ), 125, gbm.h -  6, 5, 6 ));
  TSAR ('_');
  CHECK( cropimage( file, concat( dir, "_.bmp"      ),  90, gbm.h - 12, 5, 6 ));
  TSAR ('-');
  CHECK( cropimage( file, concat( dir, "line.bmp"   ),  75, gbm.h - 12, 5, 6 ));
  TSAR ('(');
  CHECK( cropimage( file, concat( dir, "parenl.bmp" ),  65, gbm.h - 12, 5, 6 ));
  TSAR (')');
  CHECK( cropimage( file, concat( dir, "parenr.bmp" ),  70, gbm.h - 12, 5, 6 ));
  TSAR ('&');
  CHECK( cropimage( file, concat( dir, "amp.bmp"    ), 125, gbm.h - 12, 5, 6 ));
  TSAR ('.');
  CHECK( cropimage( file, concat( dir, "dot.bmp"    ), 135, gbm.h - 12, 5, 6 ));
  TSAR (':');
  CHECK( cropimage( file, concat( dir, "colon.bmp"  ),  60, gbm.h - 12, 5, 6 ));
  TSAR ('0');
  CHECK( cropimage( file, concat( dir, "0.bmp"      ),   0, gbm.h - 12, 5, 6 ));
  TSAR ('1');
  CHECK( cropimage( file, concat( dir, "1.bmp"      ),   5, gbm.h - 12, 5, 6 ));
  TSAR ('2');
  CHECK( cropimage( file, concat( dir, "2.bmp"      ),  10, gbm.h - 12, 5, 6 ));
  TSAR ('3');
  CHECK( cropimage( file, concat( dir, "3.bmp"      ),  15, gbm.h - 12, 5, 6 ));
  TSAR ('4');
  CHECK( cropimage( file, concat( dir, "4.bmp"      ),  20, gbm.h - 12, 5, 6 ));
  TSAR ('5');
  CHECK( cropimage( file, concat( dir, "5.bmp"      ),  25, gbm.h - 12, 5, 6 ));
  TSAR ('6');
  CHECK( cropimage( file, concat( dir, "6.bmp"      ),  30, gbm.h - 12, 5, 6 ));
  TSAR ('7');
  CHECK( cropimage( file, concat( dir, "7.bmp"      ),  35, gbm.h - 12, 5, 6 ));
  TSAR ('8');
  CHECK( cropimage( file, concat( dir, "8.bmp"      ),  40, gbm.h - 12, 5, 6 ));
  TSAR ('9');
  CHECK( cropimage( file, concat( dir, "9.bmp"      ),  45, gbm.h - 12, 5, 6 ));
  TSAR ('”');
  CHECK( cropimage( file, concat( dir, "”.bmp"      ),   5, gbm.h - 18, 5, 6 ));
  TSAR ('„');
  CHECK( cropimage( file, concat( dir, "„.bmp"      ),  10, gbm.h - 18, 5, 6 ));
  TSAR (' ');
  CHECK( cropimage( file, concat( dir, "space.bmp"  ), 150, gbm.h -  6, 5, 6 ));
  TSAR (',');
  CHECK( cropimage( file, concat( dir, "plat.bmp"   ), 135, gbm.h - 12, 5, 6 ));
  TSAR ('+');
  CHECK( cropimage( file, concat( dir, "plus.bmp"   ),  95, gbm.h - 12, 5, 6 ));
  TSAR ('%');
  CHECK( cropimage( file, concat( dir, "percent.bmp"), 125, gbm.h - 12, 5, 6 ));
  TSAR ('[');
  CHECK( cropimage( file, concat( dir, "2parenl.bmp"), 110, gbm.h - 12, 5, 6 ));
  TSAR (']');
  CHECK( cropimage( file, concat( dir, "2parenr.bmp"), 115, gbm.h - 12, 5, 6 ));

  pos[ POS_R_TEXT ].x = 111; pos[ POS_R_TEXT ].y = 80;
  pos[ POS_S_TEXT ].x =  79; pos[ POS_S_TEXT ].y =  4;
  pos[ POS_BPS    ].x = 110; pos[ POS_BPS    ].y = 67;

  flg[ UL_R_MSG_LEN    ] = 155;
  flg[ UL_R_MSG_HEIGHT ] = 12;
  flg[ UL_S_MSG_LEN    ] = 81;
  flg[ UL_S_MSG_HEIGHT ] = 7;

  printf ( "\n" );
  sprintf( file, "%sposbar.bmp", dir );
  printf ( "Processing %s.", file );

  CHECK( infoimage( file, &gbm ));
  DOT;
  CHECK( cropimage( file, concat( dir, "shaft.bmp"  ),   0, 0, 248, gbm.h ));
  DOT;
  CHECK( cropimage( file, concat( dir, "slider.bmp" ), 248, 0,  29, gbm.h ));

  pos[ POS_SLIDER_SHAFT ].x = 16; pos[ POS_SLIDER_SHAFT ].y = 34 + 10 - gbm.h;
  pos[ POS_SLIDER       ].x = 16; pos[ POS_SLIDER       ].y = 34 + 10 - gbm.h;

  printf ( "\n" );
  sprintf( file, "%svolume.bmp", dir );
  printf ( "Processing %s.", file );

  CHECK( infoimage( file, &gbm ));

  if( gbm.h <= 422 ) {
    flg[ UL_VOLUME_SLIDER ] = FALSE;
    DOT;
    CHECK( cropimage( file, concat( dir, "volbar.bmp"  ),  0, gbm.h - (( gbm.h / 15 ) * 15) + 2, 66, 13 ));
  } else {
    flg[ UL_VOLUME_SLIDER ] = TRUE;
    DOT;
    CHECK( cropimage( file, concat( dir, "volbar.bmp"  ),  0, gbm.h - 418, 66, 13 ));
    DOT;
    CHECK( cropimage( file, concat( dir, "volhndl.bmp" ), 15,  0, 14, gbm.h - 422 ));
  }

  pos[ POS_VOLBAR    ].x = 107; pos[ POS_VOLBAR    ].y = 46;
  pos[ POS_VOLSLIDER ].x =   0; pos[ POS_VOLSLIDER ].y = 12 - ( gbm.h - 422 );

  printf ( "\n" );
  sprintf( file, "%sshufrep.bmp", dir );
  printf ( "Processing %s.", file );

  DOT;
  CHECK( cropimage( file, concat( dir, "repup.bmp" ),  0, 70, 28, 15 ));
  CHECK( cropimage( file, concat( dir, "repdn.bmp" ),  0, 40, 28, 15 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "rndup.bmp" ), 28, 70, 47, 14 ));
  CHECK( cropimage( file, concat( dir, "rnddn.bmp" ), 28, 40, 47, 14 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "equp.bmp"  ),  0, 12, 23, 12 ));
  CHECK( cropimage( file, concat( dir, "eqdn.bmp"  ), 46, 12, 23, 12 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "plup.bmp"  ), 23, 12, 23, 12 ));
  CHECK( cropimage( file, concat( dir, "pldn.bmp"  ), 69, 12, 23, 12 ));

  pos[ POS_R_PL      ].x = 242; pos[ POS_R_PL      ].y =  46;
  pos[ POS_R_REPEAT  ].x = 210; pos[ POS_R_REPEAT  ].y =  12;
  pos[ POS_R_SHUFFLE ].x = 164; pos[ POS_R_SHUFFLE ].y =  12;
  pos[ POS_S_PL      ].x =  -1; pos[ POS_S_PL      ].y =  -1;
  pos[ POS_S_REPEAT  ].x =  -1; pos[ POS_S_REPEAT  ].y =  -1;
  pos[ POS_S_SHUFFLE ].x =  -1; pos[ POS_S_SHUFFLE ].y =  -1;
  pos[ POS_T_PL      ].x =  -1; pos[ POS_T_PL      ].y =  -1;
  pos[ POS_T_REPEAT  ].x =  -1; pos[ POS_T_REPEAT  ].y =  -1;
  pos[ POS_T_SHUFFLE ].x =  -1; pos[ POS_T_SHUFFLE ].y =  -1;

  printf ( "\n" );
  sprintf( file, "%smonoster.bmp", dir );
  printf ( "Processing %s.", file );

  DOT;
  CHECK( cropimage( file, concat( dir, "stereo.bmp" ),  0, 12, 29, 12 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "mono.bmp"   ), 29, 12, 27, 12 ));

  pos[ POS_STEREO      ].x = 239; pos[ POS_STEREO      ].y = 63;
  pos[ POS_MONO        ].x = 212; pos[ POS_MONO        ].y = 63;
  pos[ POS_NO_CHANNELS ].x =  -1; pos[ POS_NO_CHANNELS ].y = -1;

  printf ( "\n" );
  sprintf( file, "%snumbers.bmp", dir );

  if( !fexists( file )) {
    sprintf( file, "%snums_ex.bmp", dir );
  }

  printf ( "Processing %s.", file );

  CHECK( infoimage( file, &gbm ));
  DOT;
  CHECK( cropimage( file, concat( dir, "t0.bmp"  ),  0, gbm.h - 13, 9, 13 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "t1.bmp"  ),  9, gbm.h - 13, 9, 13 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "t2.bmp"  ), 18, gbm.h - 13, 9, 13 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "t3.bmp"  ), 27, gbm.h - 13, 9, 13 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "t4.bmp"  ), 36, gbm.h - 13, 9, 13 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "t5.bmp"  ), 45, gbm.h - 13, 9, 13 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "t6.bmp"  ), 54, gbm.h - 13, 9, 13 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "t7.bmp"  ), 63, gbm.h - 13, 9, 13 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "t8.bmp"  ), 72, gbm.h - 13, 9, 13 ));
  DOT;
  CHECK( cropimage( file, concat( dir, "t9.bmp"  ), 81, gbm.h - 13, 9, 13 ));

  pos[ POS_TIMER      ].x = 48;
  pos[ POS_TIMER      ].y = 77;
  flg[ UL_TIMER_SPACE ]   =  3;

  printf ("\n" );
  sprintf( file, "%smain.bmp", dir );
  printf ( "Processing %s.", file );

  DOT;
  CHECK( cropimage( file, concat( dir, "bg.bmp" ), 0, 0, 275, 116 ));

  pos[ POS_NOTL      ].x = -1; pos[ POS_NOTL      ].y = -1;
  pos[ POS_TL        ].x = -1; pos[ POS_TL        ].y = -1;
  pos[ POS_NOPLIST   ].x = -1; pos[ POS_NOPLIST   ].y = -1;
  pos[ POS_PLIST     ].x = -1; pos[ POS_PLIST     ].y = -1;
  pos[ POS_TIME_LEFT ].x = -1; pos[ POS_TIME_LEFT ].y = -1;
  pos[ POS_PL_LEFT   ].x = -1; pos[ POS_PL_LEFT   ].y = -1;
  pos[ POS_PL_MODE   ].x = -1; pos[ POS_PL_MODE   ].y = -1;
  pos[ POS_PL_INDEX  ].x = -1; pos[ POS_PL_INDEX  ].y = -1;
  pos[ POS_PL_TOTAL  ].x = -1; pos[ POS_PL_TOTAL  ].y = -1;

  strcpy( file, dir );
  strcat( file, "pledit.txt" );

  flg[ UL_FG_MSG_COLOR ] = 0x00FFFFFFUL;
  flg[ UL_FG_COLOR     ] = 0xFFFFFFFFUL;
  flg[ UL_BG_COLOR     ] = 0xFFFFFFFFUL;
  f01 = fopen( file, "r" );
  if( f01 ) {
    while( !feof( f01 )) {
      fgets( file, sizeof( file ), f01 );
      if( strnicmp( file, "MbFG=#", 6 ) == 0 ) {
        sscanf( file +  6, "%08lX", &flg[ UL_FG_MSG_COLOR ] );
      }
      if( strnicmp( file, "Normal=#", 6 ) == 0 ) {
        sscanf( file +  8,  "%08lX", &flg[ UL_FG_COLOR ] );
      }
      if( strnicmp( file, "NormalBG=#", 6 ) == 0 ) {
        sscanf( file + 10, "%08lX", &flg[ UL_BG_COLOR ] );
      }
      if( strnicmp( file, "Current=#", 6 ) == 0 ) {
        sscanf( file +  9, "%08lX", &flg[ UL_HI_FG_COLOR ] );
      }
      if( strnicmp( file, "SelectedBG=#", 6 ) == 0 ) {
        sscanf( file + 12, "%08lX", &flg[ UL_HI_BG_COLOR ] );
      }
    }
    fclose( f01 );
  }


  printf( "\n" );
  strcpy( file, dir );
  strcat( file, "viscolor.txt" );

  f01 = fopen( file, "r" );
  if( f01 )
  {
    strcpy( file, dir );
    file[ strlen(file) - 1 ] = 0;
    strcat( file, ".dat" );
    printf( "Creating %s...", file );
    f02 = fopen( file, "w" );
    if( f02 == NULL )
    {
      printf( "%s: Couldn't open file.\n", file );
      return 0;
    }

    while( !feof( f01 ))
    {
      fgets( file, sizeof( file ), f01 );
      fprintf( f02, "%s", file );
    }

    fclose( f01 );
    fclose( f02 );
    have_specana = 1;
    printf( "\n" );
  }

  strcpy( file, dir );
  file[ strlen( file ) - 1 ] = 0;
  strcat( file, ".pak" );
  printf( "Creating %s\n", file );

  skin = fopen( "pm123.$$$", "w" );

  fprintf( skin, "# WinAmp skin conversion bundle template.\n" );
  fprintf( skin, "1900,%sbg.bmp\n",      dir );
  fprintf( skin, "1630,%svolbar.bmp\n",  dir );
  fprintf( skin, "1820,%stb.bmp\n",      dir );
  fprintf( skin, "1821,%stbd.bmp\n",     dir );
  fprintf( skin, "1905,%sstat.bmp\n",    dir );
  fprintf( skin, "1906,%sshaft.bmp\n",   dir );
  fprintf( skin, "1300,%splaydn.bmp\n",  dir );
  fprintf( skin, "1310,%splayup.bmp\n",  dir );
  fprintf( skin, "1302,%spausedn.bmp\n", dir );
  fprintf( skin, "1312,%spauseup.bmp\n", dir );
  fprintf( skin, "1303,%srewdn.bmp\n",   dir );
  fprintf( skin, "1313,%srewup.bmp\n",   dir );
  fprintf( skin, "1304,%sfwddn.bmp\n",   dir );
  fprintf( skin, "1314,%sfwdup.bmp\n",   dir );
  fprintf( skin, "1321,%spldn.bmp\n",    dir );
  fprintf( skin, "1320,%splup.bmp\n",    dir );
  fprintf( skin, "1318,%srndup.bmp\n",   dir );
  fprintf( skin, "1308,%srnddn.bmp\n",   dir );
  fprintf( skin, "1319,%srepup.bmp\n",   dir );
  fprintf( skin, "1309,%srepdn.bmp\n",   dir );
  fprintf( skin, "1317,%snextup.bmp\n",  dir );
  fprintf( skin, "1307,%snextdn.bmp\n",  dir );
  fprintf( skin, "1316,%sprevup.bmp\n",  dir );
  fprintf( skin, "1306,%sprevdn.bmp\n",  dir );
  fprintf( skin, "1315,%spwrup.bmp\n",   dir );
  fprintf( skin, "1305,%spwrdn.bmp\n",   dir );
  fprintf( skin, "1323,%sstopup.bmp\n",  dir );
  fprintf( skin, "1322,%sstopdn.bmp\n",  dir );
  fprintf( skin, "1324,%sloaddn.bmp\n",  dir );
  fprintf( skin, "1325,%sloadup.bmp\n",  dir );
  fprintf( skin, "1611,%sstb.bmp\n",     dir );
  fprintf( skin, "5500,%ssplaydn.bmp\n", dir );
  fprintf( skin, "5510,%ssplayup.bmp\n", dir );
  fprintf( skin, "5502,%sspausedn.bmp\n",dir );
  fprintf( skin, "5512,%sspauseup.bmp\n",dir );
  fprintf( skin, "5503,%ssrewdn.bmp\n",  dir );
  fprintf( skin, "5513,%ssrewup.bmp\n",  dir );
  fprintf( skin, "5504,%ssfwddn.bmp\n",  dir );
  fprintf( skin, "5514,%ssfwdup.bmp\n",  dir );
  fprintf( skin, "5522,%ssstopup.bmp\n", dir );
  fprintf( skin, "5523,%ssstopdn.bmp\n", dir );
  fprintf( skin, "5524,%ssloaddn.bmp\n", dir );
  fprintf( skin, "5525,%ssloadup.bmp\n", dir );
  fprintf( skin, "5518,%ssrndup.bmp\n",  dir );
  fprintf( skin, "5508,%ssrnddn.bmp\n",  dir );
  fprintf( skin, "5519,%ssrepup.bmp\n",  dir );
  fprintf( skin, "5509,%ssrepdn.bmp\n",  dir );
  fprintf( skin, "5517,%ssnextup.bmp\n", dir );
  fprintf( skin, "5507,%ssnextdn.bmp\n", dir );
  fprintf( skin, "5516,%ssprevup.bmp\n", dir );
  fprintf( skin, "5506,%ssprevdn.bmp\n", dir );
  fprintf( skin, "1400,%sa.bmp\n",       dir );
  fprintf( skin, "1401,%sb.bmp\n",       dir );
  fprintf( skin, "1402,%sc.bmp\n",       dir );
  fprintf( skin, "1403,%sd.bmp\n",       dir );
  fprintf( skin, "1404,%se.bmp\n",       dir );
  fprintf( skin, "1405,%sf.bmp\n",       dir );
  fprintf( skin, "1406,%sg.bmp\n",       dir );
  fprintf( skin, "1407,%sh.bmp\n",       dir );
  fprintf( skin, "1408,%si.bmp\n",       dir );
  fprintf( skin, "1409,%sj.bmp\n",       dir );
  fprintf( skin, "1410,%sk.bmp\n",       dir );
  fprintf( skin, "1411,%sl.bmp\n",       dir );
  fprintf( skin, "1412,%sm.bmp\n",       dir );
  fprintf( skin, "1413,%sn.bmp\n",       dir );
  fprintf( skin, "1414,%so.bmp\n",       dir );
  fprintf( skin, "1415,%sp.bmp\n",       dir );
  fprintf( skin, "1416,%sq.bmp\n",       dir );
  fprintf( skin, "1417,%sr.bmp\n",       dir );
  fprintf( skin, "1418,%ss.bmp\n",       dir );
  fprintf( skin, "1419,%st.bmp\n",       dir );
  fprintf( skin, "1420,%su.bmp\n",       dir );
  fprintf( skin, "1421,%sv.bmp\n",       dir );
  fprintf( skin, "1422,%sw.bmp\n",       dir );
  fprintf( skin, "1423,%sx.bmp\n",       dir );
  fprintf( skin, "1424,%sy.bmp\n",       dir );
  fprintf( skin, "1425,%sz.bmp\n",       dir );
  fprintf( skin, "1426,%sline.bmp\n",    dir );
  fprintf( skin, "1427,%s_.bmp\n",       dir );
  fprintf( skin, "1428,%samp.bmp\n",     dir );
  fprintf( skin, "1429,%sdot.bmp\n",     dir );
  fprintf( skin, "1430,%s0.bmp\n",       dir );
  fprintf( skin, "1431,%s1.bmp\n",       dir );
  fprintf( skin, "1432,%s2.bmp\n",       dir );
  fprintf( skin, "1433,%s3.bmp\n",       dir );
  fprintf( skin, "1434,%s4.bmp\n",       dir );
  fprintf( skin, "1435,%s5.bmp\n",       dir );
  fprintf( skin, "1436,%s6.bmp\n",       dir );
  fprintf( skin, "1437,%s7.bmp\n",       dir );
  fprintf( skin, "1438,%s8.bmp\n",       dir );
  fprintf( skin, "1439,%s9.bmp\n",       dir );
  fprintf( skin, "1440,%sspace.bmp\n",   dir );
  fprintf( skin, "1441,%sparenl.bmp\n",  dir );
  fprintf( skin, "1442,%sparenr.bmp\n",  dir );
  fprintf( skin, "1443,%s„.bmp\n",       dir );
  fprintf( skin, "1444,%s”.bmp\n",       dir );
  fprintf( skin, "1445,%scolon.bmp\n",   dir );
  fprintf( skin, "1446,%splat.bmp\n",    dir );
  fprintf( skin, "1447,%splus.bmp\n",    dir );
  fprintf( skin, "1448,%spercent.bmp\n", dir );
  fprintf( skin, "1449,%s2parenl.bmp\n", dir );
  fprintf( skin, "1450,%s2parenr.bmp\n", dir );
  fprintf( skin, "1800,%sbnorate.bmp\n", dir );
  fprintf( skin, "1602,%smono.bmp\n",    dir );
  fprintf( skin, "1601,%sstereo.bmp\n",  dir );
  fprintf( skin, "1608,%snoch.bmp\n",    dir );
  fprintf( skin, "1901,%stldark.bmp\n",  dir );
  fprintf( skin, "1902,%stl.bmp\n",      dir );
  fprintf( skin, "1904,%spleft.bmp\n",   dir );
  fprintf( skin, "1903,%spleftd.bmp\n",  dir );
  fprintf( skin, "1640,%ss0.bmp\n",      dir );
  fprintf( skin, "1641,%ss1.bmp\n",      dir );
  fprintf( skin, "1642,%ss2.bmp\n",      dir );
  fprintf( skin, "1643,%ss3.bmp\n",      dir );
  fprintf( skin, "1644,%ss4.bmp\n",      dir );
  fprintf( skin, "1645,%ss5.bmp\n",      dir );
  fprintf( skin, "1646,%ss6.bmp\n",      dir );
  fprintf( skin, "1647,%ss7.bmp\n",      dir );
  fprintf( skin, "1648,%ss8.bmp\n",      dir );
  fprintf( skin, "1649,%ss9.bmp\n",      dir );
  fprintf( skin, "1650,%ssdd.bmp\n",     dir );
  fprintf( skin, "1651,%ss0dark.bmp\n",  dir );
  fprintf( skin, "1652,%ssddark.bmp\n",  dir );
  fprintf( skin, "1200,%st0.bmp\n",      dir );
  fprintf( skin, "1201,%st1.bmp\n",      dir );
  fprintf( skin, "1202,%st2.bmp\n",      dir );
  fprintf( skin, "1203,%st3.bmp\n",      dir );
  fprintf( skin, "1204,%st4.bmp\n",      dir );
  fprintf( skin, "1205,%st5.bmp\n",      dir );
  fprintf( skin, "1206,%st6.bmp\n",      dir );
  fprintf( skin, "1207,%st7.bmp\n",      dir );
  fprintf( skin, "1208,%st8.bmp\n",      dir );
  fprintf( skin, "1209,%st9.bmp\n",      dir );
  fprintf( skin, "1610,%sslider.bmp\n",  dir );
  fprintf( skin, "1660,%s0.bmp\n",       dir );
  fprintf( skin, "1661,%s1.bmp\n",       dir );
  fprintf( skin, "1662,%s2.bmp\n",       dir );
  fprintf( skin, "1663,%s3.bmp\n",       dir );
  fprintf( skin, "1664,%s4.bmp\n",       dir );
  fprintf( skin, "1665,%s5.bmp\n",       dir );
  fprintf( skin, "1666,%s6.bmp\n",       dir );
  fprintf( skin, "1667,%s7.bmp\n",       dir );
  fprintf( skin, "1668,%s8.bmp\n",       dir );
  fprintf( skin, "1669,%s9.bmp\n",       dir );
  fprintf( skin, "1830,%s0.bmp\n",       dir );
  fprintf( skin, "1831,%s1.bmp\n",       dir );
  fprintf( skin, "1832,%s2.bmp\n",       dir );
  fprintf( skin, "1833,%s3.bmp\n",       dir );
  fprintf( skin, "1834,%s4.bmp\n",       dir );
  fprintf( skin, "1835,%s5.bmp\n",       dir );
  fprintf( skin, "1836,%s6.bmp\n",       dir );
  fprintf( skin, "1837,%s7.bmp\n",       dir );
  fprintf( skin, "1838,%s8.bmp\n",       dir );
  fprintf( skin, "1839,%s9.bmp\n",       dir );
  fprintf( skin, "1840,%sh.bmp\n",       dir );
  fprintf( skin, "1630,%svolbar.bmp\n",  dir );
  fprintf( skin, "1620,%svolhndl.bmp\n", dir );

  fclose( skin );
  strcpy( file, dir );
  file[ strlen( file ) - 1 ] = 0;
  skin_bundle( "pm123.$$$", file );
  remove( "pm123.$$$" );

  strcpy( file, dir );
  file[ strlen( file ) - 1 ] = 0;
  strcat( file, ".skn" );
  printf( "Creating %s\n", file );

  skin = fopen( file, "w" );

  strcpy( file, dir );
  file[ strlen( file ) - 1 ] = 0;

  fprintf( skin, "; Created by PM123 Skin Utility (c) Taneli Lepp„ <rosmo@sektori.com>\n" );
  fprintf( skin, "\n" );
  fprintf( skin, "30,%s.pak\n", file );
  fprintf( skin, "6,1\n"  );
  fprintf( skin, "7,1\n"  );
  fprintf( skin, "9,1\n"  );
  fprintf( skin, "10,1\n" );
  fprintf( skin, "11,1\n" );
  fprintf( skin, "13,1\n" );
  fprintf( skin, "14,%ld\n", flg[ UL_R_MSG_HEIGHT ]);
  fprintf( skin, "15,%ld\n", flg[ UL_S_MSG_HEIGHT ]);

  fprintf( skin, "16,%ld/%ld/%ld\n", flg[ UL_FG_MSG_COLOR ] >> 16 & 0x000000FFUL,
                                     flg[ UL_FG_MSG_COLOR ] >>  8 & 0x000000FFUL,
                                     flg[ UL_FG_MSG_COLOR ]       & 0x000000FFUL );

  if( flg[ UL_FG_COLOR ] != 0xFFFFFFFFUL ) {
    fprintf( skin, "31,%ld/%ld/%ld\n", flg[ UL_FG_COLOR ] >> 16 & 0x000000FFUL,
                                       flg[ UL_FG_COLOR ] >>  8 & 0x000000FFUL,
                                       flg[ UL_FG_COLOR ]       & 0x000000FFUL );
  }
  if( flg[ UL_BG_COLOR ] != 0xFFFFFFFFUL ) {
    fprintf( skin, "32,%ld/%ld/%ld\n", flg[ UL_BG_COLOR ] >> 16 & 0x000000FFUL,
                                       flg[ UL_BG_COLOR ] >>  8 & 0x000000FFUL,
                                       flg[ UL_BG_COLOR ]       & 0x000000FFUL );
  }
  if( flg[ UL_HI_FG_COLOR ] != 0xFFFFFFFFUL ) {
    fprintf( skin, "33,%ld/%ld/%ld\n", flg[ UL_HI_FG_COLOR ] >> 16 & 0x000000FFUL,
                                       flg[ UL_HI_FG_COLOR ] >>  8 & 0x000000FFUL,
                                       flg[ UL_HI_FG_COLOR ]       & 0x000000FFUL );
  }
  if( flg[ UL_HI_BG_COLOR ] != 0xFFFFFFFFUL ) {
    fprintf( skin, "34,%ld/%ld/%ld\n", flg[ UL_HI_BG_COLOR ] >> 16 & 0x000000FFUL,
                                       flg[ UL_HI_BG_COLOR ] >>  8 & 0x000000FFUL,
                                       flg[ UL_HI_BG_COLOR ]       & 0x000000FFUL );
  }

  fprintf( skin, "20,%ld\n", flg[ UL_R_MSG_LEN   ]);
  fprintf( skin, "22,%ld\n", flg[ UL_S_MSG_LEN   ]);
  fprintf( skin, "21,219\n" );
  fprintf( skin, "23,0\n"   );
  fprintf( skin, "24,%ld\n", flg[ UL_TIMER_SPACE ]);
  fprintf( skin, "25,0\n"   );
  fprintf( skin, "26,1\n"   );

  if( flg[ UL_VOLUME_SLIDER ]) {
    fprintf( skin, "27,1\n" );
  }

  fprintf( skin, "28,1\n" );
  fprintf( skin, "29,1\n" );
  fprintf( skin, "\n" );

  fprintf( skin, "1:%ld,%ld\n" , pos[ POS_TIMER        ].x, pos[ POS_TIMER        ].y );
  fprintf( skin, "2:%ld,%ld\n" , pos[ POS_R_SIZE       ].x, pos[ POS_R_SIZE       ].y );
  fprintf( skin, "3:%ld,%ld\n" , pos[ POS_R_PLAY       ].x, pos[ POS_R_PLAY       ].y );
  fprintf( skin, "4:%ld,%ld\n" , pos[ POS_R_PAUSE      ].x, pos[ POS_R_PAUSE      ].y );
  fprintf( skin, "5:%ld,%ld\n" , pos[ POS_R_REW        ].x, pos[ POS_R_REW        ].y );
  fprintf( skin, "6:%ld,%ld\n" , pos[ POS_R_FWD        ].x, pos[ POS_R_FWD        ].y );
  fprintf( skin, "7:%ld,%ld\n" , pos[ POS_R_PL         ].x, pos[ POS_R_PL         ].y );
  fprintf( skin, "8:%ld,%ld\n" , pos[ POS_R_REPEAT     ].x, pos[ POS_R_REPEAT     ].y );
  fprintf( skin, "9:%ld,%ld\n" , pos[ POS_R_SHUFFLE    ].x, pos[ POS_R_SHUFFLE    ].y );
  fprintf( skin, "10:%ld,%ld\n", pos[ POS_R_PREV       ].x, pos[ POS_R_PREV       ].y );
  fprintf( skin, "11:%ld,%ld\n", pos[ POS_R_NEXT       ].x, pos[ POS_R_NEXT       ].y );
  fprintf( skin, "12:%ld,%ld\n", pos[ POS_R_POWER      ].x, pos[ POS_R_POWER      ].y );
  fprintf( skin, "13:%ld,%ld\n", pos[ POS_R_TEXT       ].x, pos[ POS_R_TEXT       ].y );
  fprintf( skin, "14:%ld,%ld\n", pos[ POS_S_TEXT       ].x, pos[ POS_S_TEXT       ].y );
  fprintf( skin, "15:%ld,%ld\n", pos[ POS_NOTL         ].x, pos[ POS_NOTL         ].y );
  fprintf( skin, "16:%ld,%ld\n", pos[ POS_TL           ].x, pos[ POS_TL           ].y );
  fprintf( skin, "17:%ld,%ld\n", pos[ POS_NOPLIST      ].x, pos[ POS_NOPLIST      ].y );
  fprintf( skin, "18:%ld,%ld\n", pos[ POS_PLIST        ].x, pos[ POS_PLIST        ].y );
  fprintf( skin, "19:%ld,%ld\n", pos[ POS_TIME_LEFT    ].x, pos[ POS_TIME_LEFT    ].y );
  fprintf( skin, "20:%ld,%ld\n", pos[ POS_PL_LEFT      ].x, pos[ POS_PL_LEFT      ].y );
  fprintf( skin, "21:%ld,%ld\n", pos[ POS_PL_MODE      ].x, pos[ POS_PL_MODE      ].y );
  fprintf( skin, "22:%ld,%ld\n", pos[ POS_LED          ].x, pos[ POS_LED          ].y );
  fprintf( skin, "23:%ld,%ld\n", pos[ POS_N_LED        ].x, pos[ POS_N_LED        ].y );
  fprintf( skin, "24:%ld,%ld\n", pos[ POS_SLIDER       ].x, pos[ POS_SLIDER       ].y );
  fprintf( skin, "25:%ld,%ld\n", pos[ POS_VOLBAR       ].x, pos[ POS_VOLBAR       ].y );
  fprintf( skin, "26:%ld,%ld\n", pos[ POS_NO_CHANNELS  ].x, pos[ POS_NO_CHANNELS  ].y );
  fprintf( skin, "27:%ld,%ld\n", pos[ POS_MONO         ].x, pos[ POS_MONO         ].y );
  fprintf( skin, "28:%ld,%ld\n", pos[ POS_STEREO       ].x, pos[ POS_STEREO       ].y );
  fprintf( skin, "29:%ld,%ld\n", pos[ POS_BPS          ].x, pos[ POS_BPS          ].y );
  fprintf( skin, "30:%ld,%ld\n", pos[ POS_S_SIZE       ].x, pos[ POS_S_SIZE       ].y );
  fprintf( skin, "31:%ld,%ld\n", pos[ POS_T_SIZE       ].x, pos[ POS_T_SIZE       ].y );
  fprintf( skin, "33:%ld,%ld\n", pos[ POS_S_PLAY       ].x, pos[ POS_S_PLAY       ].y );
  fprintf( skin, "34:%ld,%ld\n", pos[ POS_S_PAUSE      ].x, pos[ POS_S_PAUSE      ].y );
  fprintf( skin, "35:%ld,%ld\n", pos[ POS_S_REW        ].x, pos[ POS_S_REW        ].y );
  fprintf( skin, "36:%ld,%ld\n", pos[ POS_S_FWD        ].x, pos[ POS_S_FWD        ].y );
  fprintf( skin, "37:%ld,%ld\n", pos[ POS_S_PL         ].x, pos[ POS_S_PL         ].y );
  fprintf( skin, "38:%ld,%ld\n", pos[ POS_S_REPEAT     ].x, pos[ POS_S_REPEAT     ].y );
  fprintf( skin, "39:%ld,%ld\n", pos[ POS_S_SHUFFLE    ].x, pos[ POS_S_SHUFFLE    ].y );
  fprintf( skin, "40:%ld,%ld\n", pos[ POS_S_PREV       ].x, pos[ POS_S_PREV       ].y );
  fprintf( skin, "41:%ld,%ld\n", pos[ POS_S_NEXT       ].x, pos[ POS_S_NEXT       ].y );
  fprintf( skin, "42:%ld,%ld\n", pos[ POS_S_POWER      ].x, pos[ POS_S_POWER      ].y );
  fprintf( skin, "53:%ld,%ld\n", pos[ POS_T_PLAY       ].x, pos[ POS_T_PLAY       ].y );
  fprintf( skin, "54:%ld,%ld\n", pos[ POS_T_PAUSE      ].x, pos[ POS_T_PAUSE      ].y );
  fprintf( skin, "55:%ld,%ld\n", pos[ POS_T_REW        ].x, pos[ POS_T_REW        ].y );
  fprintf( skin, "56:%ld,%ld\n", pos[ POS_T_FWD        ].x, pos[ POS_T_FWD        ].y );
  fprintf( skin, "57:%ld,%ld\n", pos[ POS_T_PL         ].x, pos[ POS_T_PL         ].y );
  fprintf( skin, "58:%ld,%ld\n", pos[ POS_T_REPEAT     ].x, pos[ POS_T_REPEAT     ].y );
  fprintf( skin, "59:%ld,%ld\n", pos[ POS_T_SHUFFLE    ].x, pos[ POS_T_SHUFFLE    ].y );
  fprintf( skin, "60:%ld,%ld\n", pos[ POS_T_PREV       ].x, pos[ POS_T_PREV       ].y );
  fprintf( skin, "61:%ld,%ld\n", pos[ POS_T_NEXT       ].x, pos[ POS_T_NEXT       ].y );
  fprintf( skin, "62:%ld,%ld\n", pos[ POS_T_POWER      ].x, pos[ POS_T_POWER      ].y );
  fprintf( skin, "63:%ld,%ld\n", pos[ POS_PL_INDEX     ].x, pos[ POS_PL_INDEX     ].y );
  fprintf( skin, "64:%ld,%ld\n", pos[ POS_PL_TOTAL     ].x, pos[ POS_PL_TOTAL     ].y );
  fprintf( skin, "65:%ld,%ld\n", pos[ POS_R_STOP       ].x, pos[ POS_R_STOP       ].y );
  fprintf( skin, "66:%ld,%ld\n", pos[ POS_R_FLOAD      ].x, pos[ POS_R_FLOAD      ].y );
  fprintf( skin, "67:%ld,%ld\n", pos[ POS_SLIDER_SHAFT ].x, pos[ POS_SLIDER_SHAFT ].y );
  fprintf( skin, "68:%ld,%ld\n", pos[ POS_S_STOP       ].x, pos[ POS_S_STOP       ].y );
  fprintf( skin, "69:%ld,%ld\n", pos[ POS_S_FLOAD      ].x, pos[ POS_S_FLOAD      ].y );
  fprintf( skin, "70:%ld,%ld\n", pos[ POS_T_STOP       ].x, pos[ POS_T_STOP       ].y );
  fprintf( skin, "71:%ld,%ld\n", pos[ POS_T_FLOAD      ].x, pos[ POS_T_FLOAD      ].y );
  fprintf( skin, "72:%ld,%ld\n", pos[ POS_VOLSLIDER    ].x, pos[ POS_VOLSLIDER    ].y );

  if( have_specana ) {
    fprintf( skin, "1=visplug/analyzer.dll,24,57,76,16,%s.dat\n", file );
  } else {
    fprintf( skin, "1=visplug/analyzer.dll,24,57,76,16\n" );
  }

  fclose( skin );
  printf( "Cleaning up...\n" );

  remove( concat( dir, "prevup.bmp" ));
  remove( concat( dir, "prevdn.bmp" ));
  remove( concat( dir, "playup.bmp" ));
  remove( concat( dir, "prevdn.bmp" ));
  remove( concat( dir, "pauseup.bmp" ));
  remove( concat( dir, "pausedn.bmp" ));
  remove( concat( dir, "nextup.bmp" ));
  remove( concat( dir, "nextdn.bmp" ));
  remove( concat( dir, "stopup.bmp" ));
  remove( concat( dir, "stopdn.bmp" ));
  remove( concat( dir, "loadup.bmp" ));
  remove( concat( dir, "loaddn.bmp" ));
  remove( concat( dir, "tbd.bmp" ));
  remove( concat( dir, "tb.bmp" ));
  remove( concat( dir, "stb.bmp" ));
  remove( concat( dir, "sprevup.bmp" ));
  remove( concat( dir, "splayup.bmp" ));
  remove( concat( dir, "spauseup.bmp" ));
  remove( concat( dir, "sstopup.bmp" ));
  remove( concat( dir, "snextup.bmp" ));
  remove( concat( dir, "sloadup.bmp" ));
  remove( concat( dir, "sprevdn.bmp" ));
  remove( concat( dir, "splaydn.bmp" ));
  remove( concat( dir, "spausedn.bmp" ));
  remove( concat( dir, "sstopdn.bmp" ));
  remove( concat( dir, "snextdn.bmp" ));
  remove( concat( dir, "sloaddn.bmp" ));
  remove( concat( dir, "volbar.bmp" ));
  remove( concat( dir, "volhndl.bmp" ));
  remove( concat( dir, "pwrup.bmp" ));
  remove( concat( dir, "pwrdn.bmp" ));
  remove( concat( dir, "playdn.bmp" ));
  remove( concat( dir, "bg.bmp" ));
  remove( concat( dir, "a.bmp" ));
  remove( concat( dir, "b.bmp" ));
  remove( concat( dir, "c.bmp" ));
  remove( concat( dir, "d.bmp" ));
  remove( concat( dir, "e.bmp" ));
  remove( concat( dir, "f.bmp" ));
  remove( concat( dir, "g.bmp" ));
  remove( concat( dir, "h.bmp" ));
  remove( concat( dir, "i.bmp" ));
  remove( concat( dir, "j.bmp" ));
  remove( concat( dir, "k.bmp" ));
  remove( concat( dir, "l.bmp" ));
  remove( concat( dir, "m.bmp" ));
  remove( concat( dir, "n.bmp" ));
  remove( concat( dir, "o.bmp" ));
  remove( concat( dir, "p.bmp" ));
  remove( concat( dir, "q.bmp" ));
  remove( concat( dir, "r.bmp" ));
  remove( concat( dir, "s.bmp" ));
  remove( concat( dir, "t.bmp" ));
  remove( concat( dir, "u.bmp" ));
  remove( concat( dir, "v.bmp" ));
  remove( concat( dir, "w.bmp" ));
  remove( concat( dir, "x.bmp" ));
  remove( concat( dir, "y.bmp" ));
  remove( concat( dir, "z.bmp" ));
  remove( concat( dir, "”.bmp" ));
  remove( concat( dir, "„.bmp" ));
  remove( concat( dir, "_.bmp" ));
  remove( concat( dir, "line.bmp" ));
  remove( concat( dir, "parenl.bmp" ));
  remove( concat( dir, "parenr.bmp" ));
  remove( concat( dir, "amp.bmp" ));
  remove( concat( dir, "dot.bmp" ));
  remove( concat( dir, "colon.bmp" ));
  remove( concat( dir, "0.bmp" ));
  remove( concat( dir, "1.bmp" ));
  remove( concat( dir, "2.bmp" ));
  remove( concat( dir, "3.bmp" ));
  remove( concat( dir, "4.bmp" ));
  remove( concat( dir, "5.bmp" ));
  remove( concat( dir, "6.bmp" ));
  remove( concat( dir, "7.bmp" ));
  remove( concat( dir, "8.bmp" ));
  remove( concat( dir, "9.bmp" ));
  remove( concat( dir, "space.bmp" ));
  remove( concat( dir, "repup.bmp" ));
  remove( concat( dir, "repdn.bmp" ));
  remove( concat( dir, "rndup.bmp" ));
  remove( concat( dir, "rnddn.bmp" ));
  remove( concat( dir, "equp.bmp" ));
  remove( concat( dir, "eqdn.bmp" ));
  remove( concat( dir, "plup.bmp" ));
  remove( concat( dir, "pldn.bmp" ));
  remove( concat( dir, "slider.bmp" ));
  remove( concat( dir, "volbar.bmp" ));
  remove( concat( dir, "stereo.bmp" ));
  remove( concat( dir, "mono.bmp" ));
  remove( concat( dir, "t0.bmp" ));
  remove( concat( dir, "t1.bmp" ));
  remove( concat( dir, "t2.bmp" ));
  remove( concat( dir, "t3.bmp" ));
  remove( concat( dir, "t4.bmp" ));
  remove( concat( dir, "t5.bmp" ));
  remove( concat( dir, "t6.bmp" ));
  remove( concat( dir, "t7.bmp" ));
  remove( concat( dir, "t8.bmp" ));
  remove( concat( dir, "t9.bmp" ));
  remove( concat( dir, "2parenr.bmp" ));
  remove( concat( dir, "2parenl.bmp" ));
  remove( concat( dir, "percent.bmp" ));
  remove( concat( dir, "plus.bmp" ));
  remove( concat( dir, "plat.bmp" ));
  remove( concat( dir, "shaft.bmp" ));

  return 1;
}

int
main( int argc, char *argv[] )
{
  printf( "\x1b[1m%s Skin Utility.\x1b[0m\n", AMP_FULLNAME );
  printf( "Copyright (c) 2005-2006 Dmitry A.Steklenev <glass@ptv.ru>\n" );
  printf( "Copyright (c) 1998-2000 Taneli Lepp„ <rosmo@sektori.com>\n\n"  );

  if( argc < 3 )
  {
    printf( "Usage: %s {convert|bundle} <parameters...>\n\n", argv[0] );
    printf( "\x1b[1mconvert\x1b[0m  Converts a WinAmp skin to a PM123 skin\n" );
    printf( "         Usage:    convert <skin directory>\n" );
    printf( "         Example:  skinutil convert foo\n" );
    printf( "\n" );
    printf( "\x1b[1mbundle\x1b[0m   Bundles skin bitmaps into one file\n" );
    printf( "         Usage:    bundle <source.skn> <destination base name>\n" );
    printf( "         Example:  skinutil bundle foo.skn foodist\n" );
    return 1;
  }

  if( stricmp( argv[1], "bundle" ) == 0 )
  {
    if( skin_bundle( argv[2], argv[3] )) {
      return 0;
    } else {
      return 2;
    }
  }
  else if( stricmp( argv[1], "convert" ) == 0 )
  {
    if( skin_convert( argv[2] )) {
      return 0;
    } else {
      return 2;
    }
  }

  fprintf( stderr, "\x1b%s\x1b: Unknown command.", argv[1] );
  return 3;
}

void
dtlf( char *fixorz )
{
  while ( fixorz[ strlen( fixorz ) - 1 ] == 10 || fixorz[ strlen( fixorz ) ] == 10 ||
          fixorz[ strlen( fixorz ) - 1 ] == 13 || fixorz[ strlen( fixorz ) ] == 13 )
  {
    if( fixorz[ strlen( fixorz ) - 1 ] == 10 ) {
        fixorz[ strlen( fixorz ) - 1 ] = '\0';
    }
    if( fixorz[ strlen( fixorz ) ] == 10 ) {
        fixorz[ strlen( fixorz ) ] = '\0';
    }
    if( fixorz[ strlen( fixorz ) - 1 ] == 13 ) {
        fixorz[ strlen( fixorz ) - 1 ] = '\0';
    }
    if( fixorz[ strlen( fixorz ) ] == 13 ) {
        fixorz[ strlen( fixorz ) ] = '\0';
    }
  }
}

int
infoimage( char* src, GBM* gbm )
{
  char*   opt = "";
  int     fd;
  int     ft;
  GBM_ERR rc;

  gbm_init();

  if( gbm_guess_filetype( src, &ft ) != GBM_ERR_OK )
  {
    sprintf( suberror, "Cannot guess bitmap file format for %s", src );
    gbm_deinit();
    return 0;
  }

  if(( fd = gbm_io_open( src, GBM_O_RDONLY )) == -1 )
  {
    sprintf( suberror, "Cannot open %s", src );
    gbm_deinit();
    return 0;
  }

  if(( rc = gbm_read_header( src, fd, ft, gbm, opt )) != GBM_ERR_OK )
  {
    sprintf( suberror, "Cannot read header of %s: %s", src, gbm_err( rc ));
    gbm_io_close( fd );
    gbm_deinit();
    return 0;
  } else {
    gbm_io_close( fd );
    gbm_deinit();
    return 1;
  }
}

int
cropimage( char* src, char* dst, int xx, int yy, int ww, int hh )
{
  char    fn_src[500+1], fn_dst[500+1], *opt_src, *opt_dst;
  int     fd, ft_src, ft_dst, stride_src, flag = 0, bytes;
  int     x, y, w, h;
  GBM_ERR rc;
  GBMFT   gbmft;
  GBM     gbm;
  GBMRGB  gbmrgb[0x100];
  byte*   data;

  x = xx;
  y = yy;
  w = ww;
  h = hh;

  strcpy( fn_src, src );
  strcpy( fn_dst, dst );

  opt_src = "";
  opt_dst = "";

  gbm_init();

  if( gbm_guess_filetype( fn_src, &ft_src ) != GBM_ERR_OK )
  {
    sprintf( suberror, "Cannot guess bitmap file format for %s", fn_src );
    return 0;
  }

  if( gbm_guess_filetype( fn_dst, &ft_dst ) != GBM_ERR_OK )
  {
    sprintf(suberror, "Cannot guess bitmap file format for %s", fn_dst );
    return 0;
  }

  if(( fd = gbm_io_open( fn_src, GBM_O_RDONLY )) == -1 )
  {
    sprintf( suberror, "Cannot open %s", fn_src );
    return 0;
  }

  if(( rc = gbm_read_header( fn_src, fd, ft_src, &gbm, opt_src )) != GBM_ERR_OK )
  {
    gbm_io_close( fd );
    sprintf( suberror, "Cannot read header of %s: %s", fn_src, gbm_err( rc ));
    return 0;
  }

  if( w <= 0 ) {
    gbm_io_close( fd );
    sprintf( suberror, "w <= 0" );
    return 0;
  }
  if( h <= 0 ) {
    gbm_io_close( fd );
    sprintf( suberror, "h <= 0" );
    return 0;
  }

  if( x >= gbm.w ) {
    gbm_io_close( fd );
    sprintf( suberror, "x >= bitmap width" );
    return 0;
  }
  if( y >= gbm.h ) {
    gbm_io_close( fd );
    sprintf( suberror, "y >= bitmap height" );
    return 0;
  }
  if( x + w > gbm.w ) {
    gbm_io_close( fd );
    sprintf( suberror, "x+w > bitmap width" );
    return 0;
  }
  if( y + h > gbm.h ) {
    gbm_io_close( fd );
    sprintf( suberror, "y+h > bitmap height" );
    return 0;
  }

  gbm_query_filetype( ft_dst, &gbmft );
  switch( gbm.bpp )
  {
    case 24:
      flag = GBM_FT_W24;
      break;
    case 8:
      flag = GBM_FT_W8;
      break;
    case 4:
      flag = GBM_FT_W4;
      break;
    case 1:
      flag = GBM_FT_W1;
      break;
  }

  if(( gbmft.flags & flag ) == 0 )
  {
    gbm_io_close(fd);
    sprintf( suberror, "Output bitmap format %s does not support writing %d bpp data",
             gbmft.short_name, gbm.bpp);
    return 0;
  }

  if(( rc = gbm_read_palette( fd, ft_src, &gbm, gbmrgb )) != GBM_ERR_OK )
  {
    gbm_io_close( fd );
    sprintf( suberror, "Cannot read palette of %s: %s", fn_src, gbm_err( rc ));
    return 0;
  }

  stride_src = ((( gbm.w * gbm.bpp + 31 ) / 32 ) * 4 );
  bytes = stride_src * gbm.h;
  if(( data = malloc((size_t)bytes )) == NULL )
  {
    gbm_io_close(fd);
    sprintf( suberror, "Out of memory allocating %d bytes for bitmap", bytes );
    return 0;
  }

  if(( rc = gbm_read_data( fd, ft_src, &gbm, data )) != GBM_ERR_OK )
  {
    gbm_io_close(fd);
    sprintf( suberror, "Cannot read bitmap data of %s: %s", fn_src, gbm_err( rc ));
    return 0;
  }

  gbm_io_close( fd );
  gbm_subrectangle( &gbm, x, y, w, h, data, data );

  if(( fd = gbm_io_create( fn_dst, O_WRONLY | O_BINARY )) == -1 )
  {
    sprintf( suberror, "Cannot create %s", fn_dst );
    return 0;
  }

  gbm.w = w;
  gbm.h = h;

  if(( rc = gbm_write( fn_dst, fd, ft_dst, &gbm, gbmrgb, data, opt_dst)) != GBM_ERR_OK )
  {
    gbm_io_close( fd );
    remove( fn_dst );
    sprintf( suberror, "Cannot write %s: %s", fn_dst, gbm_err( rc ));
    return 0;
  }

  gbm_io_close( fd );
  free( data );
  gbm_deinit();

  return 1;
}

int
skin_bundle( char *src, char *dst )
{
  FILE *f, *d, *q, *tak;
  char buf[256], *buf4, orig[256];
  int i, images = 0;
  char c;
  unsigned long bwritten = 0;
  bundle_hdr hdr;
  char fbuf[32768];

  f = fopen( src, "r" );
  if( f == NULL )
  {
    printf( "%s: File not found.\n", src );
    return 0;
  }
  sprintf( buf, "%s.skn", dst );
  if( strcmp( src, buf ) == 0 )
  {
    printf( "Please name the destination skin with a different base name.\n" );
    printf( "(cannot overwrite %s with %s, you see)\n", src, buf );
    return 0;
  }

  sprintf( buf, "%s.pak", dst );
  d = fopen( buf, "wb" );
  if( d == NULL )
  {
    printf( "%s: Cannot create file.\n", buf );
    return 0;
  }

  sprintf( buf, "%s.skn", dst );
  tak = fopen( buf, "w" );
  if( tak == NULL )
  {
    printf( "%s: Cannot create file.\n", buf );
    return 0;
  }

  fprintf( tak, "; Created with PM123 Skin Utility (C) 1998-2000 Taneli Lepp„\n" );
  fprintf( tak, "; The following line enables the use of the pack file:\n" );
  fprintf( tak, "30,%s.pak\n", dst );

  *buf = '\0';

  printf( "Bundling...\n" );
  /* Background */
  while( !feof( f ))
  {
    *buf = '\0';
    while( !*buf || buf[0] == '#' || buf[0] == ';' )
    {
      if( *buf ) {
        fprintf( tak, "%s\n", buf );
      }
      if( feof( f )) {
        break;
      }
      fgets( buf, 256, f );
      if( *buf ) {
        dtlf( buf );
      }
      strcpy( orig, buf );
    }

    if( *buf && ( buf[0] != '#' || buf[0] != ';' ))
    {
      buf4 = buf;

      while( *buf4 != 0 && *buf4 != ',' && *buf4 != ':' && *buf4 != '=') {
        buf4++;
      }

      if( *buf4 )
      {
        c = *buf4;
        *buf4 = '\0';
        buf4++;

        /* Bitmap */
        if( *buf && atoi( buf ) > 0 && c == ',' )
        {
          if( atoi( buf ) > 1000 )
          {
            q = fopen( buf4, "rb" );
            if( q != NULL )
            {
              /* Seek to end */
              fseek( q, 0, SEEK_END );
              hdr.length = ftell( q );
              /* We don't want crap */
              memset( hdr.filename, '\0', 128 );
              strcpy( hdr.filename, buf4 );
              hdr.resource = atoi( buf );

              /* Seek to beginning */
              fseek( q, 0, SEEK_SET );
              fwrite( &hdr, 1, sizeof( bundle_hdr ), d );

              while( !feof( q ))
              {
                i = fread( fbuf, 1, sizeof( fbuf ), q );
                fwrite( fbuf, 1, i, d );
              }

              fclose( q );
              images++;
              bwritten += hdr.length + sizeof( bundle_hdr );
            }
          } else if( atoi( buf ) != 30 ) {
            fprintf( tak, "%s\n", orig );
          }
        } else {
          fprintf( tak, "%s\n", orig );
        }
      }
    } else {
      fprintf( tak, "%s\n", orig );
    }
  }

  fclose( f );
  fclose( d );
  fclose( tak );

  printf( "%u images, total %lu bytes (%lu kB).\n", images, bwritten, bwritten / 1024 );
  return 1;
}

