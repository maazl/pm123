/*
 * Copyright (C) 1998, 1999, 2002 Espen Skoglund
 * Copyright (C) 2000-2004 Haavard Kvaalen
 * Copyright (C) 2007 Dmitry A.Steklenev
 *
 * $Id: id3v2_header.h,v 1.1 2007/10/08 07:54:14 glass Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef ID3V2_HEADER_H
#define ID3V2_HEADER_H

#define  INCL_DOS
#include <os2.h>

/*
 * Layout for the ID3 tag header.
 */

#if 0
typedef struct _ID3V2_TAGHDR
{
  uint8_t  th_version;
  uint8_t  th_revision;
  uint8_t  th_flags;
  uint32_t th_size;

} ID3V2_TAGHDR;
#endif

/* Header size excluding "ID3" */
#define ID3V2_TAGHDR_SIZE 7   /* Size on disk */

#define ID3V2_THFLAG_USYNC    0x80
#define ID3V2_THFLAG_EXT      0x40
#define ID3V2_THFLAG_EXP      0x20
#define ID3V2_THFLAG_FOOTER   0x10

#define ID3_SET_SIZE28( size, a, b, c, d )  \
  {                                         \
    a = ( size >> ( 24 - 3 )) & 0x7f;       \
    b = ( size >> ( 16 - 2 )) & 0x7f;       \
    c = ( size >> (  8 - 1 )) & 0x7f;       \
    d = size & 0x7f;                        \
  }

#define ID3_GET_SIZE28( a, b, c, d )        \
  ((( a & 0x7f ) << ( 24 - 3 )) |           \
   (( b & 0x7f ) << ( 16 - 2 )) |           \
   (( c & 0x7f ) << (  8 - 1 )) |           \
   (( d & 0x7f )))                          \

/*
 * Layout for the extended header.
 */

#if 0
typedef struct _ID3V2_EXTHDR
{
  uint32_t eh_size;
  uint16_t eh_flags;
  uint32_t eh_padsize;

} ID3V2_EXTHDR;
#endif

#define ID3V2_EXTHDR_SIZE     10

#define ID3V2_EHFLAG_V23_CRC  0x80
#define ID3V2_EHFLAG_V24_CRC  0x20

/*
 * Layout for the frame header.
 */

#if 0
typedef struct _ID3V2_FRAMEHDR
{
  uint32_t fh_id;
  uint32_t fh_size;
  uint16_t fh_flags;

} ID3V2_FRAMEHDR;
#endif

#define ID3V2_FRAMEHDR_SIZE   10

#define ID3V2_FHFLAG_TAGALT   0x8000
#define ID3V2_FHFLAG_FILEALT  0x4000
#define ID3V2_FHFLAG_RO       0x2000
#define ID3V2_FHFLAG_COMPRESS 0x0080
#define ID3V2_FHFLAG_ENCRYPT  0x0040
#define ID3V2_FHFLAG_GROUP    0x0020

/*
 * Version 2.2.0
 */

/*
 * Layout for the frame header.
 */

#if 0
typedef struct _ID3V2_FRAMEHDR
{
  char    fh_id[3];
  uint8_t fh_size[3];
};
#endif

#define ID3V2_FRAMEHDR_SIZE_22 6

#endif /* ID3V2_HEADER_H */
