/*
 * Copyright 2008 Dmitry A.Steklenev
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

#define  INCL_DOS
#define  INCL_ERRORS
#include <os2.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "debuglog.h"

/* Copies extended attributes from one file or directory to another.
 * Attributes are added to a target file or replaced.
 */
APIRET
eacopy( const char* source, const char* target )
{
  APIRET rc       = 0;
  EAOP2  ea_op    = { 0 };
  DENA2* ea_names = NULL;
  DENA2* ea_found = NULL;
  ULONG  ea_count = -1;
  ULONG  ea_size;
  GEA2*  p;

  GEA2LIST* ea_names_list = NULL;
  FEA2LIST* ea_value_list = NULL;

  FILESTATUS4 fs;

  for(;;) {
    rc = DosQueryPathInfo((PSZ)source, FIL_QUERYEASIZE, &fs, sizeof( fs ));

    if( rc != NO_ERROR ) {
      DEBUGLOG(( "eacopy: Fail at DosQueryPathInfo of file %s, rc= %08X\n", source, rc ));
      break;
    }

    DEBUGLOG2(( "eacopy: EAs size is %d bytes.\n", fs.cbList ));

    // The buffer size is less than or equal to twice the size
    // of the file's entire EA set on disk.
    ea_size  = fs.cbList * 2;
    ea_names = malloc( ea_size );
    ea_found = ea_names;

    if( !ea_names ) {
      DEBUGLOG(( "eacopy: Not enough memory.\n" ));
      rc = ERROR_NOT_ENOUGH_MEMORY;
      break;
    }

    rc = DosEnumAttribute( ENUMEA_REFTYPE_PATH, (PVOID)source, 1, ea_names, ea_size,
                           &ea_count, ENUMEA_LEVEL_NO_VALUE );

    if( rc != NO_ERROR ) {
      DEBUGLOG(( "eacopy: Fail at DosEnumAttribute of file %s, rc= %08X\n", source, rc ));
      break;
    }

    DEBUGLOG2(( "eacopy: Have %d EAs.\n", ea_count ));

    if( !ea_count ) {
      break;
    }

    ea_names_list = calloc( 1, ea_size );

    if( !ea_names_list ) {
      DEBUGLOG(( "eacopy: Not enough memory.\n" ));
      rc = ERROR_NOT_ENOUGH_MEMORY;
      break;
    }

    p = ea_names_list->list;

    while( ea_count-- )
    {
      int size;
      DEBUGLOG2(( "eacopy: %s\n", ea_found->szName ));

      p->cbName = ea_found->cbName;
      strcpy( p->szName, ea_found->szName );
      size = sizeof( GEA2 ) + p->cbName;
      // The GEA2 data structures must be aligned on a doubleword boundary.
      size = (( size - 1 ) / sizeof( ULONG ) + 1 ) * sizeof( ULONG );
      p->oNextEntryOffset = ea_count ? size : 0;
      p = (GEA2*)((char*)p + size );
      ea_found = (DENA2*)((char*)ea_found + ea_found->oNextEntryOffset );
    }

    ea_value_list = (FEA2LIST*)ea_names;
    ea_names_list->cbList = (char*)p - (char*)ea_names_list;
    ea_value_list->cbList = ea_size;
    ea_op.fpGEA2List = ea_names_list;
    ea_op.fpFEA2List = ea_value_list;

    rc = DosQueryPathInfo((PSZ)source, FIL_QUERYEASFROMLIST, &ea_op, sizeof( ea_op ));

    if( rc != NO_ERROR ) {
      DEBUGLOG(( "eacopy: Fail at DosQueryPathInfo of file %s, rc= %08X\n", source, rc ));
      break;
    }

    rc = DosSetPathInfo((PSZ)target, FIL_QUERYEASIZE, &ea_op, sizeof( ea_op ), 0 );

    if( rc != NO_ERROR ) {
      DEBUGLOG(( "eacopy: Fail at DosSetPathInfo of file %s, rc= %08X\n", target, rc ));
      break;
    }

    rc = NO_ERROR;
    break;
  }

  free( ea_names );
  free( ea_names_list );

  return rc;
}

/* Copies extended attributes from one file or directory to another.
 * Attributes are added to a target file or replaced.
 * The source file must be opened with read access and deny write access.
 */
/*APIRET eacopy2( HFILE source, const char* target )
{
  APIRET rc       = 0;
  EAOP2  ea_op    = { 0 };
  DENA2* ea_names = NULL;
  DENA2* ea_found = NULL;
  ULONG  ea_count = -1;
  ULONG  ea_size;
  GEA2*  p;

  GEA2LIST* ea_names_list = NULL;
  FEA2LIST* ea_value_list = NULL;

  FILESTATUS4 fs;

  for(;;) {
    rc = DosQueryFileInfo(source, FIL_QUERYEASIZE, &fs, sizeof( fs ));

    if( rc != NO_ERROR ) {
      DEBUGLOG(( "eacopy: Fail at DosQueryPathInfo of, rc= %08X\n", rc ));
      break;
    }

    DEBUGLOG2(( "eacopy: EAs size is %d bytes.\n", fs.cbList ));

    // The buffer size is less than or equal to twice the size
    // of the file's entire EA set on disk.
    ea_size  = fs.cbList * 2;
    ea_names = malloc( ea_size );
    ea_found = ea_names;

    if( !ea_names ) {
      DEBUGLOG(( "eacopy: Not enough memory.\n" ));
      rc = ERROR_NOT_ENOUGH_MEMORY;
      break;
    }

    rc = DosEnumAttribute( ENUMEA_REFTYPE_FHANDLE, (PVOID)source, 1, ea_names, ea_size,
                           &ea_count, ENUMEA_LEVEL_NO_VALUE );

    if( rc != NO_ERROR ) {
      DEBUGLOG(( "eacopy: Fail at DosEnumAttribute, rc= %08X\n", rc ));
      break;
    }

    DEBUGLOG2(( "eacopy: Have %d EAs.\n", ea_count ));

    if( !ea_count ) {
      break;
    }

    ea_names_list = calloc( 1, ea_size );

    if( !ea_names_list ) {
      DEBUGLOG(( "eacopy: Not enough memory.\n" ));
      rc = ERROR_NOT_ENOUGH_MEMORY;
      break;
    }

    p = ea_names_list->list;

    while( ea_count-- )
    {
      int size;
      DEBUGLOG2(( "eacopy: %s\n", ea_found->szName ));

      p->cbName = ea_found->cbName;
      strcpy( p->szName, ea_found->szName );
      size = sizeof( GEA2 ) + p->cbName;
      // The GEA2 data structures must be aligned on a doubleword boundary.
      size = (( size - 1 ) / sizeof( ULONG ) + 1 ) * sizeof( ULONG );
      p->oNextEntryOffset = ea_count ? size : 0;
      p = (GEA2*)((char*)p + size );
      ea_found = (DENA2*)((char*)ea_found + ea_found->oNextEntryOffset );
    }

    ea_value_list = (FEA2LIST*)ea_names;
    ea_names_list->cbList = (char*)p - (char*)ea_names_list;
    ea_value_list->cbList = ea_size;
    ea_op.fpGEA2List = ea_names_list;
    ea_op.fpFEA2List = ea_value_list;

    rc = DosQueryFileInfo(source, FIL_QUERYEASFROMLIST, &ea_op, sizeof( ea_op ));

    if( rc != NO_ERROR ) {
      DEBUGLOG(( "eacopy: Fail at DosQueryPathInfo, rc= %08X\n", rc ));
      break;
    }

    rc = DosSetPathInfo((PSZ)target, FIL_QUERYEASIZE, &ea_op, sizeof( ea_op ), 0 );

    if( rc != NO_ERROR ) {
      DEBUGLOG(( "eacopy: Fail at DosSetPathInfo of file %s, rc= %08X\n", target, rc ));
      break;
    }

    rc = NO_ERROR;
    break;
  }

  free( ea_names );
  free( ea_names_list );

  return rc;
}*/

APIRET eaget( const char* file, const char* eaname, char** eadata, size_t* easize )
{
  APIRET rc;
  EAOP2  eaop2;
  size_t len = strlen(eaname);
  DEBUGLOG(("eaget(%s, %s, %p, *%u)\n", file, eaname, eadata, *easize));
  eaop2.fpGEA2List = (GEA2LIST*)alloca(sizeof(GEA2LIST) + len);
  eaop2.fpGEA2List->cbList = sizeof(GEA2LIST) + len;
  eaop2.fpGEA2List->list[0].oNextEntryOffset = 0;
  eaop2.fpGEA2List->list[0].cbName = len;
  memcpy(eaop2.fpGEA2List->list[0].szName, eaname, len + 1);
  
  // Since there is nor reasonable efficient way to determine the size of one EA
  // a rather large buffer is allocated. EAs must not be larger than 64k anyway.
  eaop2.fpFEA2List = (FEA2LIST*)malloc(sizeof(FEA2LIST) + sizeof(FEA2) + 65534);
  eaop2.fpFEA2List->cbList = sizeof(FEA2LIST) + sizeof(FEA2) + 65534;

  rc = DosQueryPathInfo((PSZ)file, FIL_QUERYEASFROMLIST, &eaop2, sizeof eaop2);
  if (rc == NO_ERROR)
  {
    if (easize)
      *easize = eaop2.fpFEA2List->list[0].cbValue;
    *eadata = malloc(eaop2.fpFEA2List->list[0].cbValue);
    memcpy(*eadata, eaop2.fpFEA2List->list[0].szName + eaop2.fpFEA2List->list[0].cbName + 1, eaop2.fpFEA2List->list[0].cbValue);
  }

  free (eaop2.fpFEA2List);
  return rc;
}
