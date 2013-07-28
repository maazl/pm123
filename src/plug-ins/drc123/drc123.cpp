/*
 * Copyright 2009-2013 Marcel Mueller
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

#define  INCL_PM
#define  INCL_DOS

#include "drc123.h"
#include "frontend.h"
#include "Deconvolution.h"
#include "Measure.h"
#include "Calibrate.h"

#include <utilfct.h>
#include <fftw3.h>
#include <format.h>
#include <filter_plug.h>
#include <plugin.h>
#include <os2.h>

#include <stdlib.h>
#include <string.h>

#include <debuglog.h>

#undef  VERSION
#define VERSION "Digital Room Correction Version 1.0"


PLUGIN_CONTEXT Ctx;

enum FilterMode
{ MODE_DECONVOLUTION,
  MODE_MEASURE,
  MODE_CALIBRATE
};

static FilterMode CurrentMode;


#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif
#ifndef M_LN10
#define M_LN10 2.302585093
#endif



/* Hamming window
#define WINDOW_FUNCTION( n, N ) (0.54 - 0.46 * cos( 2 * M_PI * n / N ))
 * Well, since the EQ does not provide more tham 24dB dynamics
 * we should use a much more agressive window function.
 * This is verified by calculations.
 */
#define WINDOW_FUNCTION( n, N ) (0.8 - 0.2 * cos( 2 * M_PI * n / N ))

#define round(n) ((n) > 0 ? (n) + 0.5 : (n) - 0.5)


/********** Ini file stuff */

#if 0
static void
save_ini( void )
{
  HINI INIhandle;

  if(( INIhandle = open_module_ini()) != NULLHANDLE )
  {
    save_ini_value ( INIhandle, newFIRorder );
    save_ini_value ( INIhandle, newPlansize );
    save_ini_value ( INIhandle, eqenabled );
    save_ini_value ( INIhandle, locklr );
    save_ini_string( INIhandle, lasteq );

    close_ini( INIhandle );
  }
}

static void
load_ini( void )
{
  HINI INIhandle;

  memset(bandgain, 0, sizeof bandgain);
  memset(mute,     0, sizeof mute    );

  eqenabled   = FALSE;
  lasteq[0]   = 0;
  newPlansize = 8192;
  newFIRorder = 4096;
  locklr      = FALSE;

  if(( INIhandle = open_module_ini()) != NULLHANDLE )
  {
    load_ini_value ( INIhandle, newFIRorder );
    load_ini_value ( INIhandle, newPlansize );
    load_ini_value ( INIhandle, eqenabled );
    load_ini_value ( INIhandle, locklr );
    load_ini_string( INIhandle, lasteq, sizeof( lasteq ));

    close_ini( INIhandle );

    // avoid crash when INI-Content is bad
    if (newPlansize < 16)
      newPlansize = 16;
     else if (newPlansize > MAX_COEF)
      newPlansize = MAX_COEF;
    if (newPlansize <= newFIRorder)
      newFIRorder = newPlansize >> 1;
    if (newFIRorder > MAX_FIR)
      newFIRorder = MAX_FIR;
  }
  eqneedinit  = TRUE;
}

static BOOL
load_eq_file( char* filename, float* gains, BOOL* mutes, float* preamp )
{
  FILE* file;
  int   i = 0;
  char  line[256];

  if (filename == NULL || *filename == 0)
    return FALSE;
  file = fopen( filename, "r" );
  if( file == NULL ) {
    return FALSE;
  }

  while( !feof( file ))
  {
    fgets( line, sizeof(line), file );
    blank_strip( line );
    if( *line && line[0] != '#' && line[0] != ';' && i < 129 )
    {
      if( i < NUM_BANDS*2 ) {
        double gain = atof(line);
        if (gain < 0)
          gain = 0;
         else if (gain <= .2511886432)
          gain = -12.;
         else if (gain >= 3.981071706)
          gain = 12.;
         else
          gain = 20.*log10(gain); 
        gains[i] = gain;
      } else if( i > NUM_BANDS*2-1 && i < NUM_BANDS*4 ) {
        mutes[i-NUM_BANDS*2] = atoi(line);
      } else if( i == NUM_BANDS*4 ) {
        *preamp = atof(line);
      }
      i++;
    }
  }
  fclose( file );
  return TRUE;
}

#endif


ULONG DLLENTRY filter_init(Filter** F, FILTER_PARAMS2* params)
{
  switch (CurrentMode)
  {case MODE_DECONVOLUTION:
    *F = new Deconvolution(*params);
    return 0;
   case MODE_MEASURE:
    *F = new Measure(*params);
    return 0;
   case MODE_CALIBRATE:
    *F = new Calibrate(*params);
    return 0;
   default:
    return 1;
  }
}

void DLLENTRY filter_update(Filter* F, const FILTER_PARAMS2 *params)
{
  F->Update(*params);
}

BOOL DLLENTRY filter_uninit(Filter* F)
{
  delete F;
  return TRUE;
}

/********** GUI stuff *******************************************************/

/*static BOOL
save_eq( HWND hwnd, float* gains, BOOL* mutes, float preamp )
{
  FILEDLG filedialog;
  FILE*   file;
  int     i = 0;

  memset( &filedialog, 0, sizeof(FILEDLG));
  filedialog.cbSize   = sizeof(FILEDLG);
  filedialog.fl       = FDS_CENTER | FDS_SAVEAS_DIALOG;
  filedialog.pszTitle = "Save Equalizer as...";

  if( lasteq[0] == 0 ) {
    strcpy( filedialog.szFullFile, "*.REQ" );
  } else {
    strcpy( filedialog.szFullFile, lasteq );
  }

  WinFileDlg( HWND_DESKTOP, HWND_DESKTOP, &filedialog );

  if( filedialog.lReturn == DID_OK )
  {
    strcpy( lasteq, filedialog.szFullFile );
    file = fopen( filedialog.szFullFile, "w" );
    if( file == NULL ) {
      return FALSE;
    }

    fprintf( file, "#\n# Equalizer created with %s\n# Do not modify!\n#\n", VERSION );
    fprintf( file, "# Band gains\n" );
    for( i = 0; i < NUM_BANDS*2; i++ ) {
      fprintf( file, "%g\n", gains[i] );
    }
    fprintf(file, "# Mutes\n" );
    for( i = 0; i < NUM_BANDS*2; i++ ) {
      fprintf(file, "%u\n", mutes[i]);
    }
    fprintf( file, "# Preamplifier\n" );
    fprintf( file, "%g\n", preamp );

    fprintf( file, "# End of equalizer\n" );
    fclose ( file );
    return TRUE;
  }

  return FALSE;
}

static
void drivedir( char* buf, char* fullpath )
{
  char drive[_MAX_DRIVE],
       path [_MAX_PATH ];

  _splitpath( fullpath, drive, path, NULL, NULL );
  strcpy( buf, drive );
  strcat( buf, path  );
}

static BOOL
load_eq( HWND hwnd, float* gains, BOOL* mutes, float* preamp )
{
  FILEDLG filedialog;

  memset( &filedialog, 0, sizeof( FILEDLG ));
  filedialog.cbSize = sizeof(FILEDLG);
  filedialog.fl = FDS_CENTER | FDS_OPEN_DIALOG;
  filedialog.pszTitle = "Load Equalizer";
  drivedir( filedialog.szFullFile, lasteq );
  strcat( filedialog.szFullFile, "*.REQ" );

  WinFileDlg( HWND_DESKTOP, HWND_DESKTOP, &filedialog );

  if( filedialog.lReturn == DID_OK )
  {
    strcpy( lasteq, filedialog.szFullFile );
    return load_eq_file( filedialog.szFullFile, gains, mutes, preamp );
  }
  return FALSE;
}

static void
set_slider( HWND hwnd, int channel, int band, double value )
{ MRESULT rangevalue;
  DEBUGLOG2(("realeq:set_slider(%p, %d, %d, %f)\n", hwnd, channel, band, value));

  if (value < -12)
  { DEBUGLOG(("load_dialog: value out of range %d, %d, %f\n", channel, band, value));
    value = -12;
  } else if (value > 12)
  { DEBUGLOG(("load_dialog: value out of range %d, %d, %f\n", channel, band, value));
    value = 12;
  }

  rangevalue = WinSendDlgItemMsg( hwnd, 200+NUM_BANDS*channel+band, SLM_QUERYSLIDERINFO,
                                  MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ), 0 );

  WinSendDlgItemMsg( hwnd, 200+NUM_BANDS*channel+band, SLM_SETSLIDERINFO,
                     MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ),
                     MPFROMSHORT( (value/24.+.5) * (SHORT2FROMMR(rangevalue) - 1) +.5 ));
}*/

static void
load_dialog( HWND hwnd )
{
  dlg_addcontrol( hwnd, WC_CIRCULARSLIDER, "Volume", WS_VISIBLE|WS_TABSTOP|WS_GROUP|CSS_PROPORTIONALTICKS|CSS_NOBUTTON|CSS_POINTSELECT,
                   292, 54, 40, 40, 134, 201, NULL, NULL );

  /*WinCreateWindow( hwnd, WC_CIRCULARSLIDER, "Volume", WS_VISIBLE|WS_TABSTOP|WS_GROUP|CSS_PROPORTIONALTICKS|CSS_NOBUTTON|CSS_POINTSELECT,
                   2*292, 3*54, 2*40, 3*40, hwnd, WinWindowFromID(hwnd, 133), 201, NULL, NULL );*/
                   
  
  
  PMRASSERT(cs_init( hwnd, 201, 0, 100, 5, 25, 100 ));
  PMRASSERT(cs_init( hwnd, 135, 0, 100, 5, 25, 100 ));
  PMRASSERT(cs_init( hwnd, 134, 0, 100, 5, 25, 100 ));

  /*int i, e;

  for( e = 0; e < 2; e++ ) {
    for( i = 0; i < NUM_BANDS; i++ ) {
      SHORT   range;

      // mute check boxes
      WinSendDlgItemMsg( hwnd, 100 + NUM_BANDS*e + i, BM_SETCHECK, MPFROMCHAR( mute[e][i] ), 0 );

      // sliders
      range = SHORT2FROMMR( WinSendDlgItemMsg( hwnd, 200 + NUM_BANDS*e + i, SLM_QUERYSLIDERINFO,
                                               MPFROM2SHORT( SMA_SLIDERARMPOSITION, SMA_RANGEVALUE ), 0 ) );
          
      WinSendDlgItemMsg( hwnd, 200 + NUM_BANDS*e + i, SLM_ADDDETENT,
                         MPFROMSHORT( range - 1 ), 0 );
      WinSendDlgItemMsg( hwnd, 200 + NUM_BANDS*e + i, SLM_ADDDETENT,
                         MPFROMSHORT( range >> 1 ), 0 );
      WinSendDlgItemMsg( hwnd, 200 + NUM_BANDS*e + i, SLM_ADDDETENT,
                         MPFROMSHORT( 0 ), 0 );

      DEBUGLOG2(("load_dialog: %d %d %g\n", e, i, bandgain[e][i]));
      set_slider( hwnd, e, i, bandgain[e][i] );
    }
  }

  // eq enabled check box
  WinSendDlgItemMsg( hwnd, EQ_ENABLED, BM_SETCHECK, MPFROMSHORT( eqenabled ), 0 );
  WinSendDlgItemMsg( hwnd, ID_LOCKLR,  BM_SETCHECK, MPFROMSHORT( locklr ), 0 );

  WinSendDlgItemMsg( hwnd, ID_FIRORDER, SPBM_SETMASTER, MPFROMLONG( NULLHANDLE ), 0 );
  WinSendDlgItemMsg( hwnd, ID_FIRORDER, SPBM_SETLIMITS, MPFROMLONG( MAX_FIR ), MPFROMLONG( MIN_FIR ));
  WinSendDlgItemMsg( hwnd, ID_FIRORDER, SPBM_SETCURRENTVALUE, MPFROMLONG( newFIRorder ), 0 );
  WinSendDlgItemMsg( hwnd, ID_PLANSIZE, SPBM_SETMASTER, MPFROMLONG( NULLHANDLE ), 0 );
  WinSendDlgItemMsg( hwnd, ID_PLANSIZE, SPBM_SETLIMITS, MPFROMLONG( MAX_COEF ), MPFROMLONG( MIN_COEF ));
  WinSendDlgItemMsg( hwnd, ID_PLANSIZE, SPBM_SETCURRENTVALUE, MPFROMLONG( newPlansize ), 0 );*/
}

HWND DLLENTRY plugin_configure(HWND hwnd, HMODULE module)
{
  Frontend(hwnd, module).Process();
  return NULLHANDLE;
}

int DLLENTRY plugin_query(PLUGIN_QUERYPARAM *param)
{
  param->type         = PLUGIN_FILTER;
  param->author       = "Marcel Mueller";
  param->desc         = VERSION;
  param->configurable = TRUE;
  param->interface    = PLUGIN_INTERFACE_LEVEL;
  return 0;
}

/* init plug-in */
int DLLENTRY plugin_init(const PLUGIN_CONTEXT* ctx)
{
  Ctx = *ctx;
  return 0;
}

#if defined(__IBMC__)
unsigned long _System _DLL_InitTerm( unsigned long modhandle,
                                     unsigned long flag )
{
  if( flag == 0 ) {
    if( _CRT_init() == -1 ) {
      return 0UL;
    }
    return 1UL;
  } else {
    #ifdef __DEBUG_ALLOC__
    _dump_allocated( 0 );
    #endif
    _CRT_term();
    return 1UL;
  }
}
#endif
