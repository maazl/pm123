/*
** Copyright (C) 2003-2005 Erik de Castro Lopo <erikd@mega-nerd.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU Lesser General Public License as published by
** the Free Software Foundation; either version 2.1 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU Lesser General Public License for more details.
**
** You should have received a copy of the GNU Lesser General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include    "sfconfig.h"
#include    "sfendian.h"

void
endswap_short_array (short *ptr, int len)
{   short   temp ;

    while (--len >= 0)
    {   temp = ptr [len] ;
        ptr [len] = ENDSWAP_SHORT (temp) ;
        } ;
} /* endswap_short_array */

void
endswap_short_copy (short *dest, const short *src, int len)
{
    while (--len >= 0)
    {   dest [len] = ENDSWAP_SHORT (src [len]) ;
        } ;
} /* endswap_short_copy */

void
endswap_int_array (int *ptr, int len)
{   int temp ;

    while (--len >= 0)
    {   temp = ptr [len] ;
        ptr [len] = ENDSWAP_INT (temp) ;
        } ;
} /* endswap_int_array */

void
endswap_int_copy (int *dest, const int *src, int len)
{
    while (--len >= 0)
    {   dest [len] = ENDSWAP_INT (src [len]) ;
        } ;
} /* endswap_int_copy */

#if HAVE_BYTESWAP_H && defined(SIZEOF_INT64_T) && (SIZEOF_INT64_T == 8)

void
endswap_int64_t_array (int64_t *ptr, int len)
{   int64_t value ;

    while (--len >= 0)
    {   value = ptr [len] ;
        ptr [len] = bswap_64 (value) ;
        } ;
} /* endswap_int64_t_array */

void
endswap_int64_t_copy (int64_t *dest, const int64_t *src, int len)
{   int64_t value ;

    while (--len >= 0)
    {   value = src [len] ;
        dest [len] = bswap_64 (value) ;
        } ;
} /* endswap_int64_t_copy */

#else

#if defined(SIZEOF_INT64_T) && (SIZEOF_INT64_T == 8)
void endswap_int64_t_array (int64_t *ptr, int len)
#else
void endswap_double_array (double *ptr, int len)
#endif
{   unsigned char *ucptr, temp ;

    ucptr = (unsigned char *) ptr ;
    ucptr += 8 * len ;
    while (--len >= 0)
    {   ucptr -= 8 ;

        temp = ucptr [0] ;
        ucptr [0] = ucptr [7] ;
        ucptr [7] = temp ;

        temp = ucptr [1] ;
        ucptr [1] = ucptr [6] ;
        ucptr [6] = temp ;

        temp = ucptr [2] ;
        ucptr [2] = ucptr [5] ;
        ucptr [5] = temp ;

        temp = ucptr [3] ;
        ucptr [3] = ucptr [4] ;
        ucptr [4] = temp ;
        } ;
} /* endswap_int64_t_array */

#if defined(SIZEOF_INT64_T) && (SIZEOF_INT64_T == 8)
void endswap_int64_t_copy (int64_t *dest, const int64_t *src, int len)
#else
void endswap_double_copy (double *dest, const double *src, int len)
#endif
{   const unsigned char *psrc ;
    unsigned char *pdest ;

    if (dest == src)
    {   
        #if defined(SIZEOF_INT64_T) && (SIZEOF_INT64_T == 8)
        endswap_int64_t_array (dest, len) ;
        #else
        endswap_double_array (dest, len) ;
        #endif
        return ;
        } ;

    psrc = ((const unsigned char *) src) + 8 * len ;
    pdest = ((unsigned char *) dest) + 8 * len ;
    while (--len >= 0)
    {   psrc -= 8 ;
        pdest -= 8 ;

        pdest [0] = psrc [7] ;
        pdest [2] = psrc [5] ;
        pdest [4] = psrc [3] ;
        pdest [6] = psrc [1] ;
        pdest [7] = psrc [0] ;
        pdest [1] = psrc [6] ;
        pdest [3] = psrc [4] ;
        pdest [5] = psrc [2] ;
        } ;
} /* endswap_int64_t_copy */

#endif

