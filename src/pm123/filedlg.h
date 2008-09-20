/*
 * Copyright 2004-2008 Dmitry A.Steklenev <glass@ptv.ru>
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

#ifndef PM123_FILEDLG_H
#define PM123_FILEDLG_H

#define  INCL_WIN
#include <os2.h>

#ifdef __cplusplus
extern "C" {
#endif

/* file dialog additional flags */
#define FDU_DIR_ENABLE       0x0001
#define FDU_RECURSEBTN       0x0002
#define FDU_RECURSE_ON       0x0004
#define FDU_RELATIVBTN       0x0008
#define FDU_RELATIV_ON       0x0010

/* file dialog standard types */
#define FDT_PLAYLIST         "Playlist files (*.LST;*.MPL;*.M3U;*.M3U8;*.PLS)"
#define FDT_PLAYLIST_LST     "PM123 playlist files (*.LST)"
#define FDT_PLAYLIST_M3U     "Internet playlist files (*.M3U)"
#define FDT_PLAYLIST_M3U8    "Unicode playlist files (*.M3U8)"
#define FDT_AUDIO            "All supported audio files ("
#define FDT_AUDIO_ALL        "All supported types (*.LST;*.MPL;*.M3U;*.M3U8;*.PLS;"
#define FDT_SKIN             "Skin files (*.SKN)"
//#define FDT_EQUALIZER        "Equalizer presets (*.EQ)"
#define FDT_PLUGIN           "Plug-in (*.DLL)"

/* This function creates and displays the file dialog
 * and returns the user's selection or selections.
 */

HWND amp_file_dlg( HWND hparent, HWND howner, PFILEDLG filedialog );

#ifdef __cplusplus
}
#endif
#endif /* PM123_FILEDLG_H */
