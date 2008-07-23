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


#include "url123.h"
#include <strutils.h>
#include <ctype.h>

#include <string.h>

#undef DEBUG
#include <debuglog.h>


const url123 url123::EmptyURL = "";

size_t url123::decode(char* dst, const char* src, size_t len)
{ const char*const ep = src + len;
  while (src != ep)
  { switch (*src)
    {case '+':
      *dst = ' ';
      break;
     case '%':
      if (ep - src >= 3) // too short?
      { // replace % escapes
        char cvt[3] = {src[0], src[1], 0};
        char* res;
        *dst = (char)strtoul(cvt, &res, 16);
        if (res == cvt+2)
        { src += 2;
          len -= 2;
          break;
        }
        // Error => restore
      }
     default:
      *dst = *src;
    }
    ++src;
    ++dst;
  }
  return len;
}

bool url123::hasScheme(const char* str)
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

bool url123::isAbsolute(const char* str)
{ DEBUGLOG(("url123::isAbsolute(%s)\n", str));
  const char* cp = strpbrk(str, ":/\\");
  return cp != NULL && *cp == ':';
}

void url123::parseParameter(stringmap& dest, const char* params)
{ if (params == NULL)
    return;
  if (*params == '?')
    ++params; // skip '?'

  xstring key;
  xstring val;

  while (*params)   
  { // find next '&'
    const char* ap = strchr(params, '&');
    const char* np; 
    if (ap == NULL)
      np = ap = params + strlen(params);
    else
      np = ap + 1;
    // find '='
    const char* ep = strnchr(params, '=', ap-params);
    if (ep == NULL)
    { // no value
      ep = ap;
      val = NULL;
    } else
    { const size_t len = ap-ep-1;
      char* vp = val.raw_init(len);
      size_t nlen = decode(vp, ep+1, len);
      if (nlen != len)
        val.assign(val, 0, nlen); // shorten
    }
    // key
    const size_t len = ep-params;
    char* kp = key.raw_init(len);
    size_t nlen = decode(kp, params, len);
    if (nlen != len)
      key.assign(key, 0, nlen); // shorten
    // store
    stringmapentry*& sep = dest.get(key);
    if (sep)
      // exists => overwrite
      sep->Value.swap(val);
    else
      // new
      sep = new stringmapentry(key, val);
    // next      
    params = np;
  } 
}

xstring url123::makeParameter(const stringmap& params)
{ xstring ret;
  stringmapentry*const* p;
  // calculate string length
  size_t len = 0;
  for (p = params.begin(); p != params.end(); ++p)
  { len += (*p)->Key.length() +1;
    if ((*p)->Value)
      len += (*p)->Value.length() +1;
  }
  if (len != 0)
  { --len;
    // make parameter string
    char* cp = ret.raw_init(len);
    p = params.begin();
    for (;;)
    { memcpy(cp, (*p)->Key.cdata(), (*p)->Key.length()+1);
      cp += (*p)->Key.length();
      if ((*p)->Value)
      { *cp++ = '=';
        memcpy(cp, (*p)->Value.cdata(), (*p)->Value.length()+1);
        cp += (*p)->Value.length();
      }
      if (++p == params.end())
        break;
      *cp++ = '&'; // delimiter 
    }
  }
  return ret;
}

bool* url123::parseBoolean(const char* val)
{ if (val == NULL)
    return NULL;
  static const struct mapentry
  { char Text[6];
    bool Val;
  } textmap[] =
  { {"0",     false},
    {"1",     true},
    {"false", false},
    {"no",    false},
    {"off",   false},
    {"on",    true},
    {"true",  true},
    {"yes",   true}
  };
  mapentry* mep = (mapentry*)bsearch(val, textmap, sizeof textmap/sizeof *textmap, sizeof *textmap, (int(TFNENTRY*)(const void*, const void*))&stricmp);
  return mep ? &mep->Val : NULL;
}

url123 url123::normalizeURL(const char* str)
{ DEBUGLOG(("url123::normalzieURL(%s)\n", str));
  url123 ret;
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
    DEBUGLOG(("url123::normalizeURL - broken url %s\n", str));
    return ret; // NULL
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
  for (cp2 = strstr(cp, "/."); cp2; cp2 = strstr(cp2, "/."))
  { DEBUGLOG(("url123::normalzieURL: removing? %s\n", cp2));
    char* cp3 = cp2;
    cp2 += 2; // move behind the match
    switch (*cp2)
    {case '/':
     case '?':
     case 0:
      // Found /. => eliminating
      memmove(cp3, cp2, len-(cp2-cp)+1);
      len -= 2;
      cp2 = cp3;
      DEBUGLOG(("url123::normalzieURL: converted to %s\n", cp));
     default: // ignore names starting with .
      continue;
     case '.':
      ++cp2;
    }
    switch (*cp2)
    {case '/':
     case '?':
     case 0:
      // Found /.. => eliminating
      // find previous '/'
      while (cp3 != cp)
      { if (*--cp3 == '/')
        { // found => remove part
          memmove(cp3, cp2, len-(cp2-cp)+1);
          len -= cp2-cp3;
          cp2 = cp3;
          break;
        }
      }
      DEBUGLOG(("url123::normalzieURL: converted to %s\n", cp));
     //default: // ignore names that start with ..
    }
  }
  if (len != ret.length())
    ret.assign(ret, 0, len); // shorten string
  DEBUGLOG(("url123::normalzieURL: %u, %s\n", len, ret.cdata()));
  return ret;
}

xstring url123::getBasePath() const
{ const char* cp = strrchr(*this, '/');
  ASSERT(cp);
  return xstring(*this, 0, cp-cdata()+1);
}

xstring url123::getObjectName() const
{ const char* cp = strrchr(*this, '/');
  ASSERT(cp);
  ++cp;
  const char* cp2 = strchr(cp, '?');
  return cp2 ? xstring(cp, cp2-cp) : xstring(cp);
}

xstring url123::getExtension() const
{ const char* cp = strrchr(*this, '/');
  ASSERT(cp);
  ++cp;
  const char* cp2 = strchr(cp, '?');
  const char* cp3 = cp2 ? strnrchr(cp, '.', cp2-cp) : strrchr(cp, '.');
  if (cp3 == NULL)
    return xstring::empty;
  return cp3;
}

xstring url123::getParameter() const
{ const char* cp = strchr(*this, '?');
  return cp ? xstring(cp) : xstring::empty;
}

xstring url123::getDisplayName() const
{ const char* cp = *this;
  if (strnicmp(cp, "file:", 5) == 0)
  { cp += 5;
    if (memcmp(cp, "///", 3) == 0)
      cp += 3;
  }
  return cp;
}

xstring url123::getShortName() const
{ const char* cp = strrchr(*this, '/');
  if (cp)
    ++cp;
  else
    cp = *this;
  // Exception for Path URLs: return the last path component
  if (*cp == 0 || *cp == '?')
  { const char* cp2 = --cp;
   next:
    //DEBUGLOG(("url123::getObjName - %c\n", cp2[-1]));
    switch (*--cp2)
    {case '/':
     case ':':
      ++cp2;
      break;
     default:
      if (cp2 != cdata()) 
        goto next;
    }
    return xstring(cp2, cp-cp2);
  } else
  { const char* cp2 = strchr(cp, '?');
    const char* cp3 = cp2 ? strnrchr(cp, '.', cp2-cp) : strrchr(cp, '.');
    if (cp3)
      cp2 = cp3;
    return cp2 ? xstring(cp, cp2-cp) : xstring(cp);
  }
}

url123 url123::makeAbsolute(const char* rel) const
{ DEBUGLOG(("url123(%p{%s})::makeAbsolute(%s)\n", this, cdata(), rel));
  // Already absolute?
  if (isAbsolute(rel))
    return normalizeURL(rel);
  // extract path of current URL
  const char* cp = strrchr(*this, '/');
  DEBUGLOG2(("url123::makeAbsolute - %p %s %s\n", rel, rel, cp));
  ASSERT(cp);
  ++cp;
  // join strings
  size_t len1 = cp - cdata();
  size_t len2 = strlen(rel);
  sco_arr<char> dp(new char[len1+len2+1]);
  memcpy(dp.get(), cdata(), len1);
  memcpy(dp.get() + len1, rel, len2);
  dp[len1+len2] = 0;
  return normalizeURL(dp.get());
}

xstring url123::makeRelative(const char* root, bool useupdir) const
{ DEBUGLOG(("url123(%p{%s})::makeRelative(%s, %u)\n", this, cdata(), root, useupdir));
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
