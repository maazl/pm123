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

#define PLUGIN_INTERFACE_LEVEL 3
#define INCL_DOS
#include <format.h>
#include <decoder_plug.h>
#include <strutils.h>
#include <xio.h>

#include <debuglog.h>


class PlaylistReader
{public: // Source
  const char* const         URL;
  XFILE* const              Source;
 protected: // Destination
  DECODER_INFO_ENUMERATION_CB Cb;
  void*                     CbParam;
  int                       Count;
 protected: // State
  xstring                   Url;
  PHYS_INFO                 Phys;
  TECH_INFO                 Tech;
  OBJ_INFO                  Obj;
  ATTR_INFO                 Attr;
  RPL_INFO                  Rpl;
  DRPL_INFO                 Drpl;
  ITEM_INFO                 Item;
  INFO_BUNDLE               Info;
  int                       Cached;
  int                       Override;
 protected:
  void                      Reset();
  void                      Create();
 protected:
                            PlaylistReader(const char* url, XFILE* source);
  virtual bool              ParseLine(char* line) = 0;
  static  void              ParseInt(int& dst, const char* str)
                            { if (str) dst = atoi(str); }
  static  void              ParseFloat(double& dst, const char* str)
                            { if (str) dst = atof(str); }
 public:
  /** Identify the playlist type and return a reader.
   * @param source open XIO handle.
   * @return Reader to handle the playlist or NULL if the file seems not to be supported.
   */
  static PlaylistReader*    SnifferFactory(const char* url, XFILE* source);
  virtual const xstring&    GetFormat() const = 0;
          int               GetCount() const  { return Count; }
  virtual bool              Parse(DECODER_INFO_ENUMERATION_CB cb, void* param);
  virtual                   ~PlaylistReader() { Create(); }
};


