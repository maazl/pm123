/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
 * Copyright 2004-2005 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2007-2008 M.Mueller
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

#ifndef  PM123_UTIL_H
#define  PM123_UTIL_H

#include <decoder_plug.h>
#include <cpp/xstring.h>
#include <cpp/url123.h>


/* visualize errors from anywhere */
void DLLENTRY pm123_display_info ( const char* );
void DLLENTRY pm123_display_error( const char* );

void DLLENTRY pm123_control( int index, void* param );

int  DLLENTRY pm123_getstring( int index, int subindex, size_t bufsize, char* buf );

/* Constructs a string of the displayable text from the file information. [123_utils] */
const xstring amp_construct_tag_string( const INFO_BUNDLE_CV* info );

/* Get current working directory */
const url123 amp_get_cwd();

/* Reads url from specified file. [123_utils] */
const xstring amp_url_from_file(const char* filename);

/* Reads an string from a drag and drop structure. [123_utils] */
const xstring amp_string_from_drghstr(HSTR hstr);

/* Make readable string from font attributes */ 
const xstring amp_font_attrs_to_string(const FATTRS& attrs, unsigned size);
/* Make font attributes from string. Return false on error */ 
bool amp_string_to_font_attrs(FATTRS& attrs, unsigned& size, const char* name);


#endif

