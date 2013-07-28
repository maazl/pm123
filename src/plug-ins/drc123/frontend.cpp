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
#define  INCL_BASE

#include "frontend.h"
#include "OpenLoop.h"

#include <stdlib.h>
#include <error.h>
#include <string.h>

#include <fileutil.h>
#include <plugin.h>
#include <cpp/dlgcontrols.h>
#include <cpp/container/stringmap.h>

#include <os2.h>

#include <debuglog.h>

#undef  VERSION
#define VERSION "Digital Room Correction Version 1.0"


xstring Frontend::WorkDir = xstring::empty;
xstring Frontend::CurrentFilter;
Frontend::WFN Frontend::WindowFunction;


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


xstringconst DefRecURI("record:///0?samp=48000&stereo&in=line&share=yes");

void Frontend::Init()
{
  OpenLoop::RecURI = xstring(DefRecURI);
}

Frontend::Frontend(HWND owner, HMODULE module)
: NotebookDialogBase(DLG_FRONTEND, module, DF_AutoResize)
{
  Pages.append() = new DeconvolutionPage(*this);
  Pages.append() = new GeneratePage(*this);
  Pages.append() = new MeasurePage(*this);
  Pages.append() = new CalibratePage(*this);
  StartDialog(owner, NB_FRONTEND);
}

static const char* const FIROrders[] =
{ "16384"
, "24576"
, "32768"
, "49152"
, "65536"
, "98304"
, "131072"
};

MRESULT Frontend::DeconvolutionPage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {case WM_INITDLG:
    ComboBox(+GetCtrl(CB_FIRORDER)).InsertItems(FIROrders, countof(FIROrders));
    SelectedFilter = CurrentFilter;
    CheckBox(+GetCtrl(CB_ENABLE)).Enabled(SelectedFilter != NULL);
    // Load initial values
    PostCommand(PB_UNDO);
    break;

   case WM_CONTROL:
    switch (SHORT1FROMMP(mp1))
    {case EF_WORKDIR:
      if (SHORT2FROMMP(mp1) == EN_CHANGE)
        PostCommand(PB_RELOAD);
      break;
     case LB_KERNEL:
      if (SHORT2FROMMP(mp1) == LN_SELECT)
      { CheckBox(+GetCtrl(CB_ENABLE)).Enabled(true);
        PostMsg(UM_UPDATEDESCR, 0, 0);
      }
      break;
    }
    return 0;

   case WM_COMMAND:
    switch (SHORT1FROMMP(mp1))
    {case PB_DEFAULT:
      { // defaults
        EntryField(+GetCtrl(EF_RECURI)).Text(xstring(DefRecURI));
        ComboBox(+GetCtrl(CB_FIRORDER)).Select(3); // 49152
      }
      break;
     case PB_UNDO:
      { EntryField(+GetCtrl(EF_WORKDIR)).Text(WorkDir);
        EntryField(+GetCtrl(EF_RECURI)).Text(xstring(OpenLoop::RecURI));
        // Populate list box with filter kernels
        PostCommand(PB_RELOAD);
        RadioButton(+GetCtrl(RB_WIN_NONE+WindowFunction)).CheckState();

        ComboBox(+GetCtrl(CB_FIRORDER)).Select(3); // 49152
      }
      break;
     case PB_APPLY:
      PostMsg(UM_SAVEDATA, 0, 0);
      break;
     case PB_BROWSE:
      { FILEDLG fdlg = { sizeof(FILEDLG) };
        char type[_MAX_PATH];
        fdlg.fl = FDS_OPEN_DIALOG;
        fdlg.ulUser = FDU_DIR_ENABLE|FDU_DIR_ONLY;
        fdlg.pszTitle = "Select DRC123 working directory";
        fdlg.pszIType = type;
        strlcpy(fdlg.szFullFile, ControlBase(+GetCtrl(EF_WORKDIR)).Text(), sizeof fdlg.szFullFile);

        (*Ctx.plugin_api->file_dlg)(GetHwnd(), GetHwnd(), &fdlg);
        if (fdlg.lReturn == DID_OK)
          ControlBase(+GetCtrl(EF_WORKDIR)).Text(fdlg.szFullFile);
      }
      break;
     case PB_RELOAD:
      UpdateDir();
      break;
    }
    return 0;

   case UM_UPDATEDESCR:
    { ControlBase descr(+GetCtrl(ST_DESCR));
      if (!SelectedFilter)
      { descr.Text("No working directory");
        return 0;
      }
      if (!FilterKernel.Load(SelectedFilter))
      { descr.Text(strerror(errno));
        return 0;
      }
      descr.Text(FilterKernel.Description.length() ? FilterKernel.Description.cdata() : "no description");
    }
    return 0;

   case UM_SAVEDATA:
    {

    }
    return 0;
  }
  return PageBase::DlgProc(msg, mp1, mp2);
}

void Frontend::DeconvolutionPage::UpdateDir()
{ ListBox lb(+GetCtrl(LB_KERNEL));
  ControlBase descr(+GetCtrl(ST_DESCR));
  lb.DeleteAll();
  xstringbuilder path;
  path.append(EntryField(+GetCtrl(EF_WORKDIR)).Text());
  if (path.length())
  { if (path[path.length()-1] != '\\' && path[path.length()-1] != '/')
      path.append('\\');
    size_t pathlen = path.length();
    path.append("*.target");
    HDIR hdir = HDIR_CREATE;
    char buf[1024];
    ULONG count = 100;
    APIRET rc = DosFindFirst(path.cdata(), &hdir,
      FILE_ARCHIVED|FILE_READONLY|FILE_HIDDEN,
      &buf, sizeof buf, &count, FIL_STANDARD);
    switch (rc)
    {default:
      descr.Text(os2_strerror(rc, buf, sizeof buf));
      break;
     case NO_ERROR:
      const char* names[100];
      int selected = LIT_NONE;
      const char* currentname = SelectedFilter ? sfnameext2(SelectedFilter) : NULL;
      do
      { FILEFINDBUF3* fb = (FILEFINDBUF3*)buf;
        const char** np = names;
        for(;;)
        { // selected entry?
          if (currentname && stricmp(currentname, sfnameext2(fb->achName)) == 0)
          { selected = np - names;
            path.clear();
            path.append(fb->achName);
          }
          // store entry
          *np++ = fb->achName;
          // next entry
          if (!fb->oNextEntryOffset)
            break;
          fb = (FILEFINDBUF3*)((char*)fb + fb->oNextEntryOffset);
        }
        // insert items
        lb.InsertItems(names, np - names, LIT_END);
        // next package
        count = 100;
      } while (DosFindNext(hdir, &buf, sizeof buf, &count) == NO_ERROR);
      // Select item
      if (selected != LIT_NONE)
      { lb.Select(selected);
        SelectedFilter = path.get();
      } else
      { SelectedFilter.reset();
        CheckBox(+GetCtrl(CB_ENABLE)).Enabled(false);
      }
      PostMsg(UM_UPDATEDESCR, 0, 0);
    }

  } else
  { // No working directory
    descr.Text("No working directory");
  }
}


MRESULT Frontend::GeneratePage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  return PageBase::DlgProc(msg, mp1, mp2);
}

MRESULT Frontend::MeasurePage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  return PageBase::DlgProc(msg, mp1, mp2);
}

MRESULT Frontend::CalibratePage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  return PageBase::DlgProc(msg, mp1, mp2);
}
