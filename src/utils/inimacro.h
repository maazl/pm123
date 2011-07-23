/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
 *
 * Copyright 2006 Dmitry A.Steklenev <glass@ptv.ru>
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

#ifndef INIMACRO_H
#define INIMACRO_H

#ifndef INI_SECTION
#define INI_SECTION  "Settings"
#endif

#define  INCL_WIN
#include <config.h>
#include <os2.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Opens the specified profile file. */
HINI open_ini( const char* filename );
/* Opens a profile file by the name of the module in the program directory. */
HINI open_module_ini( void );
/* Closes a opened profile file. */
BOOL close_ini( HINI hini );

/* Saves a binary data to the specified profile file. */
#define save_ini_data( hini, var, size ) \
  PrfWriteProfileData( hini, INI_SECTION, #var, (void*)var, size )

/* Saves a generic value binary to the specified profile file. */
#define save_ini_value( hini, var ) \
  PrfWriteProfileData ( hini, INI_SECTION, #var, (void*)&var, sizeof var )

/* Saves a generic value binary to the specified profile file. */
#ifdef __cplusplus
#define save_ini_bool( hini, var ) \
  PrfWriteProfileData( hini, INI_SECTION, #var, (void*)&var, 1 )
#endif

/* Saves a characters string to the specified profile file. */
#define save_ini_string( hini, var ) \
  PrfWriteProfileString( hini, INI_SECTION, #var, (char*)var )

/* Querys a size of the binary data saved to the specified profile file. */
#define load_ini_data_size( hini, var, size )                        \
{                                                                    \
  size = 0;                                                          \
  PrfQueryProfileSize( hini, INI_SECTION, #var, &size );             \
}

/* Loads a binary data from the specified profile file. */
void load_ini_data_core( HINI hini, const char* section, const char* key, void* dst, size_t size );
#define load_ini_data( hini, var, size )                             \
  load_ini_data_core( hini, INI_SECTION, #var, &var, size );

/* Loads a generic value binary from the specified profile file. */
#define load_ini_value( hini, var )                                  \
  load_ini_data_core( hini, INI_SECTION, #var, &var, sizeof var );

/* Loads a generic value binary from the specified profile file. */
void load_ini_int_core( HINI hini, const char* section, const char* key, void* dst, size_t size );
#define load_ini_int( hini, var )                                   \
  load_ini_int_core( hini, INI_SECTION, #var, &var, sizeof var );

/* Loads a characters string from the specified profile file. */
#define load_ini_string( hini, var, size ) \
  PrfQueryProfileString( hini, INI_SECTION, #var, NULL, var, size )

#ifdef __cplusplus
}
#endif

#endif
