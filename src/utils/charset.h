/*
 * Copyright 2005-2007 Dmitry A.Steklenev
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

#ifndef CHARSET_H
#define CHARSET_H

#include <string.h>

#define CH_DEFAULT       0
#define CH_CYR_KOI8R   878
#define CH_CYR_DOS     866
#define CH_CYR_866     866
#define CH_CYR_OS2     866
#define CH_ISO_8859_1  819
#define CH_UCS_2      1200
#define CH_UCS_2LE    1200
#define CH_UCS_2BOM   8200
#define CH_UCS_2BE    9200
#define CH_UTF_8      1208
#define CH_CYR_WIN    1251
#define CH_CYR_1251   1251
#define CH_CYR_AUTO     -1

#define CH_SBCS       0x01
#define CH_MBCS       0x02
#define CH_DBCS       0x04

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*_CH_PFN)( const char* );

typedef struct _CH_ENTRY {

  char    name[128];
  int     id;
  int     type;
  _CH_PFN pfn;

} CH_ENTRY;

extern const CH_ENTRY ch_list[];
extern const int      ch_list_size;

/*
 * ch_default: returns the current system character set.
 */

int ch_default( void );

/*
 * ch_detect: determine a characters string character set.
 *
 *    ch_source source character set
 *    source    source string
 *
 *    return    character set identifier
 */

int ch_detect( int ch_source, const char* source );

/*
 * ch_convert: convert a characters string from one character
 *             set to another.
 *
 *    ch_source source character set
 *    source    source string
 *    ch_target target character set
 *    target    result buffer
 *    size      size of result buffer
 *
 *    return    != NULL: converted string
 *              == NULL: error
 */

char* ch_convert( int ch_source, const char* source,
                  int ch_target, char* target, size_t size );

#ifdef __cplusplus
}
#endif

#endif /* CHARSET_H */
