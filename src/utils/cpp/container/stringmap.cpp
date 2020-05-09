/*
 * Copyright 2008-2011 M.Mueller
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


#include "stringmap.h"


int TFNENTRY strabbrevicmp(const char* str, const char* abbrev)
{ return strnicmp(str, abbrev, strlen(abbrev));
}

const char* mapsearcha2_core(const char* cmd, const char* map, size_t count, size_t size)
{ const char* elem = (const char*)bsearch(cmd, map, count, size, (int(TFNENTRY*)(const void*, const void*))&strabbrevicmp);
  if (elem)
  { // Work around to find more precise matches in case of ambiguous abbreviations.
    const char* const last = map + count*size;
    const char* elem2 = elem;
    while ( (elem2 += size) != last        // not the end of the array
      && strabbrevicmp(elem2, elem) == 0 ) // elem2 is still based on elem
    { if (strabbrevicmp(cmd, elem2) == 0)  // it matches
        elem = elem2;
    }
  }
  return elem;
}
