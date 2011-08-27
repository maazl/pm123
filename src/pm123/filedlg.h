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
#include <decoder_plug.h>
#include <cpp/container/vector.h>
#include <os2.h>


FLAGSATTRIBUTE(DECODER_TYPE);

/* file dialog additional flags */
#define FDU_DIR_ENABLE       0x0001
#define FDU_RECURSEBTN       0x0002
#define FDU_RECURSE_ON       0x0004
#define FDU_RELATIVBTN       0x0008
#define FDU_RELATIV_ON       0x0010

#define FDT_ALL "<All supported files>"

/* Helper class for amp_file_types(). */
class APSZ_list : public vector<char>
{public:
  APSZ_list() {}
  APSZ_list(size_t size) : vector<char>(size) {}
  ~APSZ_list();
  operator APSZ*() const;
};

/** This function returns a list of file types with a storage compatible to APSZ.
 * The result contains only entries that have the corresponding flagsreq set.
 * flagsreq is a bit vector of DECODER_FLAGS_PLAYLIST.
 * You MUST delete the list with delete when you no longer need it.
 */
APSZ_list* amp_file_types(DECODER_TYPE flagsreq);

/** Search for a decoder that supports the specified file type.
 * @param flags Only search for decoders that have this flags.
 * @param filter File type to search for. Syntax: "EA Type (File mask)".
 * @return Name of the matching decoder or NULL of none is found.
 */
xstring amp_decoder_by_type(DECODER_TYPE flagsreq, const char* filter, xstring& format);

/** This function creates and displays the file dialog
 * and returns the user's selection or selections.
 * Important note: \c filedialog->pszIType must point
 * to a \e writable string of at least \c _MAX_PATH bytes.
 * It contains the selected type on return.
 */
HWND amp_file_dlg( HWND hparent, HWND howner, PFILEDLG filedialog );

#endif /* PM123_FILEDLG_H */
