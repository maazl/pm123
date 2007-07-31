/*
 * Copyright 2007-2007 M.Mueller
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


#include "url.h"
#include <strutils.h>
#include <ctype.h>

void url::parse()
{ 
}

bool url::hasScheme(const char* str)
{ // exception for drive letters
  if (isalpha(str[0]) && str[1] == ':')
    return false;
  for (;; ++str)
    switch (*str)
    {case 0:
      return false;

     case ':':
      return true;
     
     default:
      if (!isalpha(*str))
        return false;
     case '+':
     case '-':
     case '.':;
    }
}

bool url::isAbsolute(const char* str)
{ DEBUGLOG(("url::isAbsolute(%s)\n", str));
  const char* cp = strpbrk(str, ":/\\");
  return cp != NULL && *cp == ':';
}

url url::normalizeURL(const char* str)
{ DEBUGLOG(("url::normalzieURL(%s)\n", str));
  url ret;
  if (str == NULL)
    return ret;
  char* cp; // store part of string to check here
  size_t len;
  if (isalpha(str[0]) && str[1] == ':')
  { // File name => prepend with file:///
    len = strlen(str);
    cp = ret.raw_init(len+8);
    memcpy(cp, "file:///", 8);
    cp += 8;
    memcpy(cp, str, len);
  } else if (isPathDelimiter(str[0]) && isPathDelimiter(str[1]))
  { // UNC path => prepend with file:
    len = strlen(str);
    cp = ret.raw_init(len+5);
    memcpy(cp, "file:///", 5); // make string common with above
    cp += 5;
    memcpy(cp, str, len);
  } else if (!hasScheme(str))
  { // broken URL
    DEBUGLOG(("url::normalizeURL - broken url %s\n", str));
    return ret;
  } else if (strnicmp(str, "cd:", 3) == 0)
  { // turn into cdda:
    len = strlen(str);
    cp = ret.raw_init(len+2);
    memcpy(cp, "cdda:", 5);
    cp += 5;
    len -= 3;
    memcpy(cp, str+3, len);
  } else
  { len = strlen(str);
    cp = ret.raw_init(len, str);
  }
  // convert \ to /
  char* cp2;
  for (cp2 = strchr(cp, '\\'); cp2; cp2 = strchr(cp2+1, '\\'))
    *cp2 = '/';
  // reduce /xxx/.. - s/\/[^\/]\/\.\.//g;
  len = ret.length();
  for (cp2 = strstr(cp, "/.."); cp2; cp2 = strstr(cp2, "/..")) 
  { // find previous '/'
    char* cp3 = cp2;
    cp2 += 3; // move behind the match
    while (cp3 != cp)
    { if (*--cp3 == '/')
      { // found => remove part
        memmove(cp3, cp2, len-(cp2-cp));
        len -= cp2-cp3;
        cp2 = cp3;
        break;
      }
    }
  }
  if (len != ret.length())
    ret.assign(ret, len); // shorten string
  DEBUGLOG(("url::normalzieURL: %s\n", ret.cdata()));
  return ret;
}

xstring url::getDisplayName() const
{ const char* cp = *this;
  if (strnicmp(cp, "file:", 5) == 0)
  { cp += 5;
    if (memcmp(cp, "///", 3) == 0)
      cp += 3;
  }
  return cp;
}

xstring url::getBasePath() const
{ const char* cp = strrchr(*this, '/');
  assert(cp);
  return xstring(*this, 0, cp-cdata()+1);
}

xstring url::getObjName() const
{ const char* cp = strrchr(*this, '/');
  assert(cp);
  ++cp;
  const char* cp2 = strchr(cp, '?');
  const char* cp3 = cp2 ? strnrchr(cp, '.', cp2-cp) : strrchr(cp, '.');
  if (cp3)
    cp2 = cp3;
  DEBUGLOG(("url::getObjName()\n"));
  return cp2 ? xstring(cp, cp2-cp) : xstring(cp);
}

xstring url::getExtension() const
{ const char* cp = strrchr(*this, '/');
  assert(cp);
  ++cp;
  const char* cp2 = strchr(cp, '?');
  const char* cp3 = cp2 ? strnrchr(cp, '.', cp2-cp) : strrchr(cp, '.');
  if (cp3 == NULL)
    return xstring::empty;
  return cp3;
}

xstring url::getParameter() const
{ const char* cp = strchr(*this, '?');
  return cp ? xstring(cp) : xstring::empty;
}

url url::makeAbsolute(const char* rel) const
{ DEBUGLOG(("url(%p{%s})::makeAbsolute(%s)\n", this, cdata(), rel));
  // Already absolute?
  if (isAbsolute(rel))
    return normalizeURL(rel);
  // extract path of current URL
  const char* cp = strrchr(*this, '/');
  DEBUGLOG2(("url::makeAbsolute - %p %s %s\n", rel, rel, cp));
  assert(cp);
  ++cp;
  // join strings
  size_t len1 = cp - cdata();
  size_t len2 = strlen(rel);
  sco_arr<char> dp = new char[len1+len2+1];
  memcpy(dp.get(), cdata(), len1);
  memcpy(dp.get() + len1, rel, len2);
  dp[len1+len2] = 0;
  return normalizeURL(dp.get());
}

xstring url::makeRelative(const char* root, bool useupdir) const
{ DEBUGLOG(("url(%p{%s})::makeRelative(%s, %u)\n", this, cdata(), root, useupdir));
  // find common part
  const char* sp1 = *this;
  const char* sp2 = root;
  while (*sp1 == *sp2 && *sp1)
    ++sp1, ++sp2;
  
  // Count number of '/' in root after common part of root URL.
  size_t updirs = 0;
  { const char* cp = strchr(sp2, '/');
    if (cp)
    { if (!useupdir) // no subpath => impossible without ../
        return *this;
      do
      { ++updirs;
        cp = strchr(cp+1, '/');
      } while (cp);
  } }
  // go back to the previous slash of the absolute URL.
  for (;;)
  { if (sp1 == cdata()) // start of URL => relative impossible
      return *this;
    if (sp1[-1] == '/')
      break;
    --sp1;
  }
  // relative path contains colon => impossible
  // and do not replace the server name with ../
  if (strchr(sp1, ':') != NULL || sp1[-2] == '/')
    return *this;
  // Possible! => concatenate relative URL
  size_t len = strlen(sp1);
  xstring ret;
  char* dp = ret.raw_init(3*updirs + len);
  while (updirs--)
  { memcpy(dp, "../", 3);
    dp += 3;
  }
  memcpy(dp, sp1, len);
  return ret;
}

