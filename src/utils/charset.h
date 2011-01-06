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

#define INCL_PM
#include <string.h>
#include <stdlib.h>
#include <os2.h>
#include <uconv.h>

// no code page
#define CH_CP_NONE    0UL


#ifdef __cplusplus
extern "C" {
#endif

typedef int (*_CH_PFN)( const char* );

typedef struct _CH_ENTRY {
  const char*  name;
  ULONG        codepage;
} CH_ENTRY;

/* List of character sets
 * Layout:                       size
 * 1  default entry              -+-  sbcs
 * [] non unicode sbcs codepages  |   =+=       unicode
 * [] unicode sbcs codepages      |        dbcs -+-
 + [] unicode dbcs codepages     -+-       =+=  -+-
 */
extern const CH_ENTRY ch_list[];
extern const size_t   ch_list_size;
extern const size_t   ch_list_sbcs;
extern const size_t   ch_list_dbcs;
extern const size_t   ch_list_unicode;

/*
 * ch_default: returns the current system character set.
 */
int ch_default( void );

/*
 * ch_find: returns a pointer to the character set entry for the
 *          specified codepage.
 *
 *    codepage codepage to find
 *
 *    return   pointer to matching CH_ENTRY or NULL if not found
 */
const CH_ENTRY* ch_find( ULONG codepage );

/*
 * ch_info: get info about specified codepage.
 *
 *    codepage codepage to find.
 *    attr     codepage attributes. See Unicode Programming Reference.
 *
 *    return   FALSE on error.
 */
BOOL ch_info( ULONG codepage, uconv_attribute_t* attr );

/*
 * ch_detect: determine a characters string character set.
 *            This will check the system's prepared codepages and the one
 *            specified as parameter.
 *
 *    cp_source probable source character set or CH_CP_NONE if unknown
 *    source    source string
 *
 *    return    identified codepage or CH_CP_NONE if unknown.
 */
ULONG ch_detect( ULONG codepage, const char* source );

/**
 * ch_convert: convert a characters string from one character set to another.
 *
 * @param cp_source Source codepage or CH_CP_NONE (0) to use the application's default.
 * @param source    Source string.
 * @param cp_target Target codepage or CH_CP_NONE (0) to use the application's default.
 * @param target    Result buffer.
 * @param size      Size of result buffer.
 * @param flags     See CH_FLAGS_...
 * @return          != NULL: converted string
 *                  == NULL: error
 */
char* ch_convert( ULONG cp_source, const char* source, 
                  ULONG cp_target, char* target,
                  size_t size, unsigned flags );

#define CH_FLAGS_WRITE_BOM 0x01 // Write U+FEFF character, (UCS-2 and UTF-8 only)

#ifdef __cplusplus
}
#endif

#endif /* CHARSET_H */
