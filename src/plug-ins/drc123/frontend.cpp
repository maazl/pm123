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
#include "Deconvolution.h"
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



/********** Ini file stuff */

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
};

MRESULT Frontend::DeconvolutionPage::DlgProc(ULONG msg, MPARAM mp1, MPARAM mp2)
{
  switch (msg)
  {case WM_INITDLG:
    ComboBox(+GetCtrl(CB_FIRORDER)).InsertItems(FIROrders, countof(FIROrders));
    // Load initial values
    PostCommand(PB_UNDO);
    break;

   case WM_DESTROY:
    Result.Detach();
    break;

   case WM_CONTROL:
    switch (SHORT1FROMMP(mp1))
    {case EF_WORKDIR:
      if (SHORT2FROMMP(mp1) == EN_CHANGE)
        PostCommand(PB_RELOAD);
      break;
     case LB_KERNEL:
      if (SHORT2FROMMP(mp1) == LN_SELECT)
      { xstringbuilder sb;
        sb.append(ControlBase(+GetCtrl(EF_WORKDIR)).Text());
        if (sb.length() && sb[sb.length()-1] != '\\')
          sb.append('\\');
        ListBox lb(+GetCtrl(LB_KERNEL));
        int sel = lb.NextSelection();
        sb.append(lb.ItemText(sel));
        Params.FilterFile = sb.get();
        //DEBUGLOG(("%s\n", Params.FilterFile.cdata()));
        PostMsg(UM_UPDATEDESCR, 0, 0);
      }
      break;
    }
    return 0;

   case WM_COMMAND:
    switch (SHORT1FROMMP(mp1))
    {case PB_DEFAULT:
      Deconvolution::GetDefaultParameters(Params);
      goto load;

     case PB_UNDO:
      Deconvolution::GetParameters(Params);
      if (Params.FilterFile)
      { char path[_MAX_PATH];
        sdrivedir(path, Params.FilterFile, sizeof path);
        EntryField(+GetCtrl(EF_WORKDIR)).Text(path);
        UpdateDir();
      }
     load:
      { // Load GUI from current configuration
        CheckBox(+GetCtrl(CB_ENABLE)).Enabled(Params.FilterFile != NULL);
        EntryField(+GetCtrl(EF_RECURI)).Text(xstring(OpenLoop::RecURI));
        // Populate list box with filter kernels
        PostCommand(PB_RELOAD);
        CheckBox(+GetCtrl(CB_ENABLE)).Enabled(Params.Enabled);
        RadioButton(+GetCtrl(RB_WIN_NONE + Params.WindowFunction)).CheckState();
        int selected; // default
        switch (Params.FIROrder)
        {case 16384:
          selected = 0; break;
         case 24576:
          selected = 1; break;
         case 32768:
          selected = 2; break;
         default: //case 49152:
          selected = 3; break;
         case 65536:
          selected = 4; break;
         case 98304:
          selected = 5; break;
        }
        ComboBox(+GetCtrl(CB_FIRORDER)).Select(selected);
      }
      break;

     case PB_APPLY:
      { // Update configuration from GUI
        OpenLoop::RecURI = EntryField(+GetCtrl(EF_RECURI)).Text();
        Params.WindowFunction = (Deconvolution::WFN)(RadioButton(+GetCtrl(RB_WIN_NONE)).CheckID() - RB_WIN_NONE);
        switch (ComboBox(+GetCtrl(CB_FIRORDER)).NextSelection())
        {case 0: // 16384
          Params.FIROrder = 16384;
          Params.PlanSize = 32768;
          break;
         case 1: // 24576
          Params.FIROrder = 24576;
          Params.PlanSize = 32768;
          break;
         case 2: // 32768
          Params.FIROrder = 32768;
          Params.PlanSize = 65536;
          break;
         case 3: // 49152
          Params.FIROrder = 49152;
          Params.PlanSize = 65536;
          break;
         case 4: // 65536
          Params.FIROrder = 65536;
          Params.PlanSize = 131072;
          break;
         case 5: // 98304
          Params.FIROrder = 98304;
          Params.PlanSize = 131072;
          break;
        }
        Deconvolution::SetParameters(Params);
        save_config();
      }
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
      ControlBase enabled(+GetCtrl(CB_ENABLE));
      if (!Params.FilterFile)
      { descr.Text("No working directory");
        goto disable;
      }
      if (!Kernel.Load(Params.FilterFile))
      { descr.Text(strerror(errno));
       disable:
        Result.Detach();
        enabled.Enabled(false);
        return 0;
      }
      Result.Attach(GetCtrl(CC_RESULT));
      descr.Text(Kernel.Description.length() ? Kernel.Description.cdata() : "no description");
      enabled.Enabled(true);
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
      const char* currentname = Params.FilterFile ? sfnameext2(Params.FilterFile) : NULL;
      do
      { FILEFINDBUF3* fb = (FILEFINDBUF3*)buf;
        const char** np = names;
        for(;;)
        { // selected entry?
          if (currentname && stricmp(currentname, sfnameext2(fb->achName)) == 0)
          { selected = np - names;
            path.erase(pathlen);
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
        Params.FilterFile = path.get();
      } else
      { Params.FilterFile.reset();
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
