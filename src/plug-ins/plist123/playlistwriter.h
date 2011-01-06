/*
 * Copyright 2009-2010 M.Mueller
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

#define DECODER_PLUGIN_LEVEL 2
#include <decoder_plug.h>

#include <xio.h>

#include <debuglog.h>


/** @brief Abstract class for playlist writers.
 * @details Use the Factory \c PlaylistWriter::FormatFactory to create a writer.
 */
class PlaylistWriter
{public:
  /// Structure to hold all properties of a playlist item to save.
  struct Item
  { DSTRING                 Url;
    const INFO_BUNDLE*      Info;
    int                     Valid;
    int                     Override;
  };
 protected: // Source
  const char* const         ListUrl;  // initialized by constructor
  const INFO_BUNDLE*        ListInfo; // initialized by Init
  int                       Valid;    // initialized by Init
 protected: // Destination
  XFILE*                    Stream;   // initialized by Init
  int                       Options;  // initialized by Init
 protected: // Internal
  bool                      OK;
 protected:
                            PlaylistWriter(const char* list);
  virtual void              Write(const char* data);
 public:
  virtual                   ~PlaylistWriter() {}
  /// Factory to create playlist writers.
  /// @return Returns NULL on error (unknown format).
  static PlaylistWriter*    FormatFactory(const char* list, const char* format);
  /// @brief Initialize the playlist writer.
  /// @param info All known information about the playlist object.
  /// @param valid Bit vector with valid information in \c *info.
  /// @param stream Destination to write to.
  /// @return false in case of an I/O Error.
  /// @details This is the first function to be called.
  virtual bool              Init(const INFO_BUNDLE* info, int valid, XFILE* stream);
  /// @brief Finish writing of the playlist.
  /// @return false in case of an I/O Error.
  /// @details This is the last function to be called. It does not close the stream.
  virtual bool              Finish();
  /// @brief append a playlist item.
  /// @param item Structure with all properties of the playlist item.
  /// @return false in case of an I/O Error.
  virtual bool              AppendItem(const Item& item) = 0;
};

