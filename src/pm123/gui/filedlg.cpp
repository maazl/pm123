/*
 * Copyright 2004-2008 Dmitry A.Steklenev <glass@ptv.ru>
 *           2009-2013 Marcel Mueller
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

#define  INCL_WIN
#define  INCL_DOS
#include <string.h>

#include "filedlg.h"
#include "pm123.rc.h"
#include "../configuration.h"
#include "dialog.h"
#include "../engine/plugman.h"
#include "../engine/decoder.h"
#include <utilfct.h> // for do_warpsans
#include <fileutil.h>
#include <wildcards.h>
#include <cpp/xstring.h>
#include <cpp/container/stringset.h>
#include <cpp/container/stringmap.h>

#include <os2.h>

#include <debuglog.h>


APSZ_list::~APSZ_list()
{ for(char*const* cpp = end(); cpp-- != begin();)
    delete[] *cpp;
}


class FileTypesEnumerator
{private:
  int_ptr<PluginList>     Decoders;
  const vector<const DECODER_FILETYPE>* Current;
  size_t                  DecNo;
  size_t                  Item;
 public:
  FileTypesEnumerator();
  const DECODER_FILETYPE* GetCurrent() const { return (*Current)[Item]; }
  Decoder*                GetDecoder() const { return (Decoder*)(*Decoders)[DecNo].get(); }
  bool                    Next();
};

FileTypesEnumerator::FileTypesEnumerator()
: Decoders(Plugin::GetPluginList(PLUGIN_DECODER))
, Current(NULL)
{}

bool FileTypesEnumerator::Next()
{ DEBUGLOG(("FileTypesEnumerator(%p)::Next() - %p\n", this, Current));
  if (!Current)
  { DecNo = 0;
    goto start;
  }
  ++Item;
 load_decoder:
  if (Item < Current->size())
    return true;
  for (;;)
  { ++DecNo;
   start:
    if (DecNo >= Decoders->size())
    { Current = NULL;
      return false;
    }
    if ((*Decoders)[DecNo]->GetEnabled())
    { Current = &GetDecoder()->GetFileTypes();
      Item = 0;
      goto load_decoder;
    }
  }
}


typedef strmapentry<stringset> AggregateEntry;
typedef sorted_vector<AggregateEntry,xstring,&AggregateEntry::compare> Aggregate;

static void do_aggregate(Aggregate& dest, const char* key, const char* types)
{ if (types == NULL || *types == 0)
    return;
  AggregateEntry*& agg = dest.get(key);
  if (agg == NULL)
    agg = new AggregateEntry(key);
  for (;;)
  { const char* cp = strchr(types, ';');
    xstring val(types, cp ? cp-types: strlen(types));
    DEBUGLOG(("filedlg:do_aggregate: %s\n", val.cdata()));
    agg->Value.ensure(val);
    if (cp == NULL)
      return;
    types = cp+1;
  }
}

APSZ_list* amp_file_types(DECODER_TYPE flagsreq)
{ // Create aggregate[EA type][extension]
  Aggregate aggregate;
  
  sco_ptr<FileTypesEnumerator> en(new FileTypesEnumerator());
  while (en->Next())
  { const DECODER_FILETYPE* filetype = en->GetCurrent();
    if (~filetype->flags & flagsreq)
      continue;
    do_aggregate(aggregate, filetype->eatype ? filetype->eatype : "", filetype->extension);
    // Aggregate
    if (filetype->category)
      do_aggregate(aggregate, filetype->category, filetype->extension);
    do_aggregate(aggregate, FDT_ALL, filetype->extension);
  }

  // Create result
  APSZ_list* result = new APSZ_list(40);
  foreach (AggregateEntry,*const*, app, aggregate)
  { // Join extensions
    stringset& set = (*app)->Value;
    // Calculate length
    // The calculated length may be a few byte more than required, if either eatype or extensions are missing. 
    size_t len = (*app)->Key.length() + 3 + set.size()-1 +1; // EAtype + " ()" + delimiters + '\0'
    for (const xstring* xpp = set.end(); xpp-- != set.begin();)
      len += xpp->length();
    char* dp = new char[len];
    result->append() = dp;
    // Store EA type
    if ((*app)->Key.length())
    { memcpy(dp, (*app)->Key.cdata(), (*app)->Key.length());
      dp += (*app)->Key.length();
      if (set.size())
      { *dp++ = ' ';
        goto both; // bypass if below
    } }
    // Store extensions
    if (set.size())
    {both:
      *dp++ = '(';
      const xstring* xpp = set.begin();
      goto start; // 1st item
      while (++xpp != set.end())
      { *dp++ = ';';
       start:
        memcpy(dp, xpp->cdata(), xpp->length());
        dp += xpp->length();
      }
      *dp++ = ')';
    }
    *dp = 0; // Terminator
  }
  // NULL terminate the list to be APSZ compatible.
  result->append();
  return result; 
}

xstring amp_decoder_by_type(DECODER_TYPE flagsreq, const char* filter, xstring& format)
{ DEBUGLOG(("amp_decoder_by_type(%x, %s, )\n", flagsreq, filter));
  if (!filter || !*filter)
    return xstring();

  // Parse filter string
  size_t typelen = strlen(filter);
  { const char* cp = strchr(filter, '(');
    if (cp)
    { typelen = cp - filter;
      while (typelen && isblank(filter[typelen-1]))
        --typelen;
    }
  }
  DEBUGLOG(("amp_decoder_by_type %i (->%x)\n", typelen, filter[typelen-1]));

  //int matchlevel = 0; // 0 => no match, 1 => Category match, 2 => Exact match
  int_ptr<Decoder> decoder;
  //Mutex::Lock lock(PluginMtx);
  { sco_ptr<FileTypesEnumerator> en(new FileTypesEnumerator());
    while (en->Next())
    { const DECODER_FILETYPE* ft = en->GetCurrent();
      if (~ft->flags & flagsreq)
        continue;
      DEBUGLOG(("amp_decoder_by_type %s %s %s\n", ft->eatype, ft->category, ft->extension));
      // Exact match?
      if (strnicmp(ft->eatype, filter, typelen) == 0)
      { decoder = en->GetDecoder();
        format = ft->eatype;
        break;
      }
      // Category match?
      if (strnicmp(ft->category, filter, typelen) == 0)
      { decoder = en->GetDecoder();
        format = ft->eatype;
      }
    }
  }
  DEBUGLOG(("amp_decoder_by_type: %s %s\n", decoder ? decoder->ModRef->Key.cdata() : "(null)", format.cdata()));
  return decoder ? decoder->ModRef->Key : xstring();
}

/* Default dialog procedure for the file dialog. */
static MRESULT EXPENTRY
amp_file_dlg_proc( HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2 )
{ DEBUGLOG2(("amp_file_dlg_proc(%x, %x, %x, %x)\n", hwnd, msg, mp1, mp2));
  
  FILEDLG* filedialog =
    (FILEDLG*)WinQueryWindowULong( hwnd, QWL_USER );

  switch( msg )
  {
    case WM_INITDLG:
      if (filedialog && !(filedialog->ulUser & FDU_RECURSEBTN))
        WinShowWindow( WinWindowFromID( hwnd, CB_RECURSE ), FALSE );
      else
        WinCheckButton( hwnd, CB_RECURSE, Cfg::Get().add_recursive );
      if (filedialog && !(filedialog->ulUser & FDU_RELATIVBTN))
        WinShowWindow(WinWindowFromID( hwnd, CB_RELATIVE ), FALSE);
      else
        WinCheckButton(hwnd, CB_RELATIVE, Cfg::Get().save_relative);
      if (filedialog && (filedialog->ulUser & FDU_DIR_ENABLE))
        WinEnableControl(hwnd, DID_OK, TRUE);
      do_warpsans(hwnd);
      Cfg::RestWindowPos(hwnd);
      break;

    case WM_DESTROY:
      Cfg::SaveWindowPos(hwnd);
      break;

    case WM_ADJUSTWINDOWPOS:
      { SWP* pswp = (SWP*)PVOIDFROMMP(mp1);
        if (pswp->fl & SWP_SIZE)
          dlg_adjust_resize(hwnd, pswp);
      }
      break;

    case WM_WINDOWPOSCHANGED:
      { SWP* pswp = (SWP*)PVOIDFROMMP(mp1);
        if (pswp->fl & SWP_SIZE)
          dlg_do_resize(hwnd, pswp, pswp+1);
      }
      break;

    case WM_CONTROL:
      if( SHORT1FROMMP(mp1) == DID_FILENAME_ED && SHORT2FROMMP(mp1) == EN_CHANGE )
      {
        char file[_MAX_PATH];
        WinQueryDlgItemText( hwnd, DID_FILENAME_ED, sizeof(file), file );

        if (filedialog->ulUser & FDU_RECURSEBTN)
        { WinEnableControl( hwnd, CB_RECURSE, !*file
              || strcmp( file, "*"   ) == 0
              || strcmp( file, "*.*" ) == 0 );
        }

        // Prevents DID_OK from being greyed out.
        if (filedialog->ulUser & FDU_DIR_ENABLE)
          return 0;
      }
      break;

    case WM_COMMAND:
      if( SHORT1FROMMP(mp1) == DID_OK )
      {
        // Retrieve selected file type
        WinQueryDlgItemText( hwnd, DID_FILTER_CB, _MAX_PATH, filedialog->pszIType );

        Cfg::ChangeAccess cfg;
        if (filedialog->ulUser & FDU_RELATIVBTN)
        { cfg.save_relative = WinQueryButtonCheckstate(hwnd, CB_RELATIVE);
          filedialog->ulUser = (filedialog->ulUser & ~FDU_RELATIV_ON) | FDU_RELATIV_ON*cfg.save_relative;
        }

        if (filedialog->ulUser & FDU_DIR_ENABLE)
        {
          char file[_MAX_PATH];
          WinQueryDlgItemText(hwnd, DID_FILENAME_ED, sizeof(file), file);

          if( !*file
            || strcmp( file, "*"   ) == 0
            || strcmp( file, "*.*" ) == 0 )
          {
            if (!is_root( filedialog->szFullFile))
              filedialog->szFullFile[strlen(filedialog->szFullFile)-1] = 0;

            filedialog->lReturn    = DID_OK;
            filedialog->ulFQFCount = 1;

            if (filedialog->ulUser & FDU_RECURSEBTN)
            { cfg.add_recursive = WinQueryButtonCheckstate( hwnd, CB_RECURSE );
              filedialog->ulUser = (filedialog->ulUser & ~FDU_RECURSE_ON) | FDU_RECURSE_ON*cfg.add_recursive;
            }

            WinDismissDlg(hwnd, DID_OK);
            return 0;
          }
        }
      }
      break;

    case FDM_FILTER:
    {
      HWND  hcbox = WinWindowFromID( hwnd, DID_FILTER_CB );
      ULONG pos   = WinQueryLboxSelectedItem( hcbox );
      ULONG len   = LONGFROMMR( WinSendMsg( hcbox, LM_QUERYITEMTEXTLENGTH, MPFROMSHORT(pos), 0 ));
      char* type  = (char*)malloc( len );
      BOOL  rc    = FALSE;
      char* filt;
      char  file[_MAX_PATH];

      if( !type ) {
        return WinDefFileDlgProc( hwnd, msg, mp1, mp2 );
      }

      WinQueryLboxItemText( hcbox, pos, type, len );
      WinQueryDlgItemText ( hwnd, DID_FILENAME_ED, sizeof(file), file );

      // If the selected type is not have extensions list - that it <All Files>
      // which OS/2 always adds in the list.
      if( !strchr( type, '(' )) {
        rc = TRUE;
      } else {
        strtok( type, "(" );

        while(( filt = strtok( NULL, ";)" )) != NULL ) {
          if( wildcardfit( filt, (char*)mp1 )) {
            rc = TRUE;
            break;
          }
        }
      }

      if( rc && ( strchr( file, '*' ) || strchr( file, '?' ))) {
        rc = wildcardfit( file, (char*)mp1 );
      }

      free( type );
      return MRFROMLONG( rc );
    }
  }
  return WinDefFileDlgProc( hwnd, msg, mp1, mp2 );
}

/* This function creates and displays the file dialog
 * and returns the user's selection or selections.
 */
HWND DLLENTRY amp_file_dlg(HWND hparent, HWND howner, PFILEDLG filedialog)
{ // Convert slashes. OS/2 does not care much, but it might look confusing when not unique.
  { char* cp = filedialog->szFullFile;
    while ((cp = strchr(cp, '/')) != NULL)
      *cp++ = '\\';
  }
  filedialog->hMod       = NULLHANDLE;
  switch (filedialog->ulUser & FDU_DIR_ONLY)
  {case FDU_DIR_ONLY:
    filedialog->usDlgId  = DLG_DIR;
    { size_t len = strlen(filedialog->szFullFile);
      if (len && len < sizeof(filedialog->szFullFile) -1 && filedialog->szFullFile[len-1] != '\\')
      { filedialog->szFullFile[len] = '\\';
        filedialog->szFullFile[len+1] = 0;
    } }
    break;
   case FDU_FILE_ONLY:
    filedialog->usDlgId  = DLG_FILE;
    break;
   default:
    filedialog->usDlgId  = DLG_FILEDIR;
    break;
  }
  filedialog->pfnDlgProc = amp_file_dlg_proc;
  filedialog->fl        |= FDS_CUSTOM;

  return WinFileDlg(hparent, howner, filedialog);
}

