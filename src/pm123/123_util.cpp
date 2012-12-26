/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
 * Copyright 2004-2005 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2007-2010 M.Mueller
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

#include "123_util.h"
#include "configuration.h"
#include <utilfct.h>

#include <os2.h>
#include <string.h>
#include <stdio.h>


/* Get current working directory */
const url123 amp_get_cwd()
{ static url123 CWD;
  if (!CWD)
  { char cdir[_MAX_PATH+1];
    RASSERT(_getcwd(cdir, sizeof cdir -1));
    size_t len = strlen(cdir);
    // Append trailing slash?
    if (cdir[len-1] != '\\')
    { cdir[len] = '\\';
      cdir[len+1] = 0;
    }
    CWD = url123::normalizeURL(cdir);
  }
  return CWD;
}

/* Reads url from specified file. */
const xstring amp_url_from_file(const char* filename)
{
  FILE* file = fopen(filename, "r");
  if (!file)
    return (char*)NULL;
  fseek(file, 0, SEEK_END);
  long len = ftell(file);
  if (len > 4095) // some limit...
    len = 4095;
  fseek(file, 0, SEEK_SET);
  xstring ret;
  if (len > 0)
  { char* dp = ret.allocate(len);
    if (fgets(dp, len+1, file))
      blank_strip(dp);
    else
      ret = NULL;
  }
  fclose(file);
  return ret;
}

const xstring& amp_dnd_temp_file()
{ static xstring DnDTempFile;
  if (!DnDTempFile)
  { const char* temp = getenv("TEMP");
    size_t len = 0;
    if (temp)
    { len = strlen(temp);
      if (len && temp[len-1] == '\\')
        --len;
    }
    char* cp = DnDTempFile.allocate(len+12);
    memcpy(cp, temp, len);
    memcpy(cp+len, "\\~123DnD.lst", 12);
  }
  return DnDTempFile;
}

const xstring amp_make_dir_url(const char* url, bool recursive)
{ size_t len = strlen(url);
  xstringbuilder sb(len + 50);
  sb.append(url);
  if (url[len-1] != '/')
    sb.append('/');
  sb.append('?');
  if (recursive)
    sb.append("recursive&");
  const volatile amp_cfg& cfg = Cfg::Get();
  if (cfg.folders_first)
    sb.append("foldersfirst&");
  sb.erase(sb.length()-1);
  return sb;
}

const xstring amp_font_attrs_to_string(const FATTRS& attrs, unsigned size)
{ xstringbuilder ret;
  ret.appendf("%u.%s", size, attrs.szFacename);
  if (attrs.fsSelection & FATTR_SEL_BOLD      )
    ret.append(".Bold");
  if (attrs.fsSelection & FATTR_SEL_UNDERSCORE)
    ret.append(".Underscore");
  if (attrs.fsSelection & FATTR_SEL_ITALIC    )
    ret.append(".Italic");
  if (attrs.fsSelection & FATTR_SEL_OUTLINE   )
    ret.append(".Outline");
  if (attrs.fsSelection & FATTR_SEL_STRIKEOUT )
    ret.append(".Strikeout");
  return ret;
}

bool amp_string_to_font_attrs(FATTRS& attrs, unsigned& size, const char* name)
{ size_t n;
  if (sscanf(name, "%u.%32[^.]%n", &size, attrs.szFacename, &n) != 2)
    return false;
  // Attributes
  name += n;
  if (strstr(name, ".Bold"))
    attrs.fsSelection |= FATTR_SEL_BOLD;
  if (strstr(name, ".Underscore"))
    attrs.fsSelection |= FATTR_SEL_UNDERSCORE;
  if (strstr(name, ".Italic"))
    attrs.fsSelection |= FATTR_SEL_ITALIC;
  if (strstr(name, ".Outline"))
    attrs.fsSelection |= FATTR_SEL_OUTLINE;
  if (strstr(name, ".Strikeout"))
    attrs.fsSelection |= FATTR_SEL_STRIKEOUT;
  return true;
}
