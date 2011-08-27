/*
 * Copyright 2006-2011 M.Mueller
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

/* This is the core interface to the plug-ins. It loads the plug-ins and
 * virtualizes them if required to refect always the most recent interface
 * level to the application.
 *
 * This interface is used only by plugman.cpp!
 */ 

#ifndef PM123_DECODER_H
#define PM123_DECODER_H

#include "plugman.h"
#include <decoder_plug.h>
#include <cpp/container/stringmap.h>


/// Set of entry points related to decoder plug-ins.
struct DecoderProcs
{ void*  W;
  int    DLLENTRYP(decoder_init     )(void** w);
  BOOL   DLLENTRYP(decoder_uninit   )(void*  w);
  ULONG  DLLENTRYP(decoder_command  )(void*  w, ULONG msg, const DECODER_PARAMS2* params);
  void   DLLENTRYP(decoder_event    )(void*  w, OUTEVENTTYPE event);
  ULONG  DLLENTRYP(decoder_status   )(void*  w);
  double DLLENTRYP(decoder_length   )(void*  w);
  ULONG  DLLENTRYP(decoder_fileinfo )(const char* url, int* what, const INFO_BUNDLE* info,
                                      DECODER_INFO_ENUMERATION_CB cb, void* param);
  ULONG  DLLENTRYP(decoder_saveinfo )(const char* url, const META_INFO* info, int haveinfo, xstring* errortxt);
  ULONG  DLLENTRYP(decoder_savefile )(const char* url, const char* format, int* what, const INFO_BUNDLE* info,
                                      DECODER_SAVE_ENUMERATION_CB cb, void* param);
  ULONG  DLLENTRYP(decoder_editmeta )(HWND owner, const char* url);
  const  DECODER_WIZARD*
         DLLENTRYP(decoder_getwizard)();
  // Init structure
         DecoderProcs()              { memset(this, 0, sizeof *this); } // Uh, well, allowed for PODs
};

class Playable;
struct DecoderProcs;
/****************************************************************************
 *
 * Decoder plug-in instance.
 *
 ***************************************************************************/
class Decoder : public Plugin, protected DecoderProcs
{public:
  typedef strmapentry<stringset_own> FileTypeEntry;

 public:
  /// Try unsupported file extensions and types too.
  bool         TryOthers;
 protected:
  /// Result from the decoder_support call. Supported data sources.
  DECODER_TYPE Type;
  /// Result from the decoder_support call. Supported file types.
  const DECODER_FILETYPE* FileTypes;
  /// Size of the array above.
  int          FileTypesCount;
  /// Additional supported file types (user setting).
  xstring      AddFileTypes;
 private:
  vector<const DECODER_FILETYPE> FileTypeList;
 private:
  DECODER_FILETYPE DFT_Add;
  xstring      FileTypeCache;
  xstring      FileExtensionCache;

 protected:
  // instances of this class are only created by the factory function below.
  Decoder(Module& module);
  // Fill file type and extension cache
  void         FillFileTypeCache();
  /// Load the plug-in that is identified as a decoder.
  /// @exception ModuleException Something went wrong.
  virtual void LoadPlugin();
 private:
  static  bool DoFileTypeMatch(const char* filetypes, USHORT type, const USHORT*& eadata);

 public:
  virtual      ~Decoder();
  /*/// Save ID3-data to the given file
  virtual ULONG SaveInfo(const char* url, const META_INFO* info, DECODERMETA haveinfo, xstring& errortxt) = 0;
  /// Save Playlist to file
  virtual ULONG SavePlaylist(const char* url, Playable& playlist, const char* format, bool relative) = 0;
  /// call special decoder dialog to edit ID3-data of the given file
  virtual ULONG EditMeta(HWND owner, const char* url) = 0;*/

  /// Initialize the decoder.
  /// @exception ModuleException Something went wrong.
  virtual void InitPlugin();
  // Uninitialize the decoder. Return TRUE on success.
  virtual bool UninitPlugin();
  // Implementation of CL_PLUGIN::is_initialized.
  virtual bool IsInitialized() const   { return W != NULL; }
  // Getter to the decoder entry points.
  //const DecoderProcs& GetProcs() const { return *this; }
  // Overloaded for parameter recognition
  virtual void GetParams(stringmap_own& map) const;
  virtual bool SetParam(const char* param, const xstring& value);

  /// Return a combination of DECODER_* flags to determine what kind of objects are supported.
  DECODER_TYPE GetObjectTypes() const  { return Type; }
  // Checks whether a decoder claims to support a certain URL.
  bool         IsFileSupported(const char* file, const char* eatype) const;
  /// Get Supported EA types or NULL
  const vector<const DECODER_FILETYPE>& GetFileTypes() const { return FileTypeList; }

  ULONG        DecoderCommand(DECMSGTYPE msg, const DECODER_PARAMS2* params) { return decoder_command(W, msg, params); }

  void         DecoderEvent(OUTEVENTTYPE event) { if (W && decoder_event) decoder_event(W, event); }

  ULONG        DecoderStatus()         { return W ? decoder_status(W) : DECODER_ERROR; }

  PM123_TIME   DecoderLength()         { return W ? decoder_length(W) : -1; }

  ULONG        Fileinfo(const char* url, int* what, const INFO_BUNDLE* info, DECODER_INFO_ENUMERATION_CB cb, void* param)
               { return decoder_fileinfo(url, what, info, cb, param); }

  ULONG        SaveInfo(const char* url, const META_INFO* info, int haveinfo, xstring& errortxt)
               { if (decoder_saveinfo) return decoder_saveinfo(url, info, haveinfo, &errortxt);
                 errortxt = xstring::sprintf("The plug-in %s cannot save meta information.", ModRef.Key.cdata());
                 return PLUGIN_NO_USABLE; }

  ULONG        EditMeta(HWND owner, const char* url) { return decoder_editmeta ? decoder_editmeta(owner, url) : 400; }

  ULONG        SaveFile(const char* url, const char* format, int* what, const INFO_BUNDLE* info, DECODER_SAVE_ENUMERATION_CB cb, void* param)
               { return decoder_savefile ? decoder_savefile(url, format, what, info, cb, param) : PLUGIN_NO_USABLE; }

  /// Add additional entries in load/add menu in the main and the playlist's pop-up menu.
  static void  AppendLoadMenu(HWND hMenu, ULONG id_base, SHORT where, DECODER_WIZARD_FUNC* callbacks, size_t size);
  /// Append accelerator table with plug-in specific entries.
  static void  AppendAccelTable(HACCEL& haccel, ULONG id_base, LONG offset, DECODER_WIZARD_FUNC* callbacks, size_t size);

 public:
  /// Return the \c Decoder instance on \a Module if it exists.
  /// Otherwise return \c NULL.
  static int_ptr<Decoder> FindInstance(const Module& module);
  /// Return the \c Decoder instance on \a module. Create a new one if required.
  /// @exception ModuleException Something went wrong.
  static int_ptr<Decoder> GetInstance(Module& module);
  /// Return the \c Decoder instance by module name.
  /// The function will only return ordinary when the decoder is found, currently configured and enabled.
  /// @exception ModuleException Something went wrong.
  static int_ptr<Decoder> GetDecoder(const char* name);
  // initialize global services
  static void  Init();
  static void  Uninit()                {}
};

#endif
