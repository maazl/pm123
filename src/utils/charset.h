/*
 * Copyright 2005 Dmitry A.Steklenev
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

#ifndef __CHARSET_H
#define __CHARSET_H

#include <string.h>


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

extern const CH_ENTRY ch_list[];
extern const int      ch_list_size;

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

/*
 * ch_convert: convert a characters string from one character
 *             set to another.
 *
 *    hab       program anchor block handle
 *    cp_source source codepage or CH_CP_NONE to use the application's default
 *    source    source string
 *    cp_target target codepage or CH_CP_NONE to use the application's default
 *    target    result buffer
 *    size      size of result buffer
 *
 *    return    != NULL: converted string
 *              == NULL: error
 */
char* ch_convert( ULONG cp_source, const char* source, 
                  ULONG cp_target, char* target, size_t size );

#ifdef __cplusplus
}
#endif

#endif /* __CHARSET_H */
