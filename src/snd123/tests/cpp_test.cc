/*
** Copyright (C) 2006 Erik de Castro Lopo <erikd@mega-nerd.com>
**
** This program is free software; you can redistribute it and/or modify
** it under the terms of the GNU General Public License as published by
** the Free Software Foundation; either version 2 of the License, or
** (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <sndfile.hh>

#include "utils.h"

static short    sbuffer [100] ;
static int      ibuffer [100] ;
static float    fbuffer [100] ;
static double   dbuffer [100] ;

static void
create_file (const char * filename, int format)
{   SndfileHandle file ;

    if (file.refCount () != 0)
    {   printf ("\n\n%s %d : Error : Reference count (%d) should be zero.\n\n", __func__, __LINE__, file.refCount ()) ;
        exit (1) ;
        } ;

    file = SndfileHandle (filename, SFM_WRITE, format, 2, 48000) ;

    if (file.refCount () != 1)
    {   printf ("\n\n%s %d : Error : Reference count (%d) should be 1.\n\n", __func__, __LINE__, file.refCount ()) ;
        exit (1) ;
        } ;

    file.setString (SF_STR_TITLE, filename) ;

    /* Item write. */
    file.write (sbuffer, ARRAY_LEN (sbuffer)) ;
    file.write (ibuffer, ARRAY_LEN (ibuffer)) ;
    file.write (fbuffer, ARRAY_LEN (fbuffer)) ;
    file.write (dbuffer, ARRAY_LEN (dbuffer)) ;

    /* Frame write. */
    file.writef (sbuffer, ARRAY_LEN (sbuffer) / file.channels ()) ;
    file.writef (ibuffer, ARRAY_LEN (ibuffer) / file.channels ()) ;
    file.writef (fbuffer, ARRAY_LEN (fbuffer) / file.channels ()) ;
    file.writef (dbuffer, ARRAY_LEN (dbuffer) / file.channels ()) ;

    /* RAII takes care of the SndfileHandle. */
} /* create_file */

static void
check_title (const SndfileHandle & file, const char * filename)
{   const char *title = NULL ;

    title = file.getString (SF_STR_TITLE) ;

    if (title == NULL)
    {   printf ("\n\n%s %d : Error : No title.\n\n", __func__, __LINE__) ;
        exit (1) ;
        } ;

    if (strcmp (filename, title) != 0)
    {   printf ("\n\n%s %d : Error : title '%s' should be '%s'\n\n", __func__, __LINE__, title, filename) ;
        exit (1) ;
        } ;

    return ;
} /* check_title */

static void
read_file (const char * filename, int format)
{   SndfileHandle file ;
    sf_count_t count ;

    if (file)
    {   printf ("\n\n%s %d : Error : should not be here.\n\n", __func__, __LINE__) ;
        exit (1) ;
        } ;

    file = SndfileHandle (filename) ;

    if (1)
    {   SndfileHandle file2 = file ;

        if (file.refCount () != 2 || file2.refCount () != 2)
        {   printf ("\n\n%s %d : Error : Reference count (%d) should be two.\n\n", __func__, __LINE__, file.refCount ()) ;
            exit (1) ;
            } ;
        } ;

    if (file.refCount () != 1)
    {   printf ("\n\n%s %d : Error : Reference count (%d) should be one.\n\n", __func__, __LINE__, file.refCount ()) ;
        exit (1) ;
        } ;

    if (! file)
    {   printf ("\n\n%s %d : Error : should not be here.\n\n", __func__, __LINE__) ;
        exit (1) ;
        } ;

    if (file.format () != format)
    {   printf ("\n\n%s %d : Error : format 0x%08x should be 0x%08x.\n\n", __func__, __LINE__, file.format (), format) ;
        exit (1) ;
        } ;

    if (file.channels () != 2)
    {   printf ("\n\n%s %d : Error : channels %d should be 2.\n\n", __func__, __LINE__, file.channels ()) ;
        exit (1) ;
        } ;

    if (file.frames () != ARRAY_LEN (sbuffer) * 4)
    {   printf ("\n\n%s %d : Error : frames %ld should be %d.\n\n", __func__, __LINE__,
                SF_COUNT_TO_LONG (file.frames ()), ARRAY_LEN (sbuffer) * 4 / 2) ;
        exit (1) ;
        } ;

    switch (format & SF_FORMAT_TYPEMASK)
    {   case SF_FORMAT_AU :
                break ;

        default :
            check_title (file, filename) ;
            break ;
        } ;

    /* Item read. */
    file.read (sbuffer, ARRAY_LEN (sbuffer)) ;
    file.read (ibuffer, ARRAY_LEN (ibuffer)) ;
    file.read (fbuffer, ARRAY_LEN (fbuffer)) ;
    file.read (dbuffer, ARRAY_LEN (dbuffer)) ;

    /* Frame read. */
    file.readf (sbuffer, ARRAY_LEN (sbuffer) / file.channels ()) ;
    file.readf (ibuffer, ARRAY_LEN (ibuffer) / file.channels ()) ;
    file.readf (fbuffer, ARRAY_LEN (fbuffer) / file.channels ()) ;
    file.readf (dbuffer, ARRAY_LEN (dbuffer) / file.channels ()) ;

    count = file.seek (file.frames () - 10, SEEK_SET) ;
    if (count != file.frames () - 10)
    {   printf ("\n\n%s %d : Error : offset (%ld) should be %ld\n\n", __func__, __LINE__,
                SF_COUNT_TO_LONG (count), SF_COUNT_TO_LONG (file.frames () - 10)) ;
        exit (1) ;
        } ;

    count = file.read (sbuffer, ARRAY_LEN (sbuffer)) ;
    if (count != 10 * file.channels ())
    {   printf ("\n\n%s %d : Error : count (%ld) should be %ld\n\n", __func__, __LINE__,
                SF_COUNT_TO_LONG (count), SF_COUNT_TO_LONG (10 * file.channels ())) ;
        exit (1) ;
        } ;

    /* RAII takes care of the SndfileHandle. */
} /* read_file */

static void
ceeplusplus_test (const char *filename, int format)
{
    print_test_name ("ceeplusplus_test", filename) ;

    create_file (filename, format) ;
    read_file (filename, format) ;

    remove (filename) ;
    puts ("ok") ;
} /* ceeplusplus_test */

int
main (void)
{
    ceeplusplus_test ("cpp_test.wav", SF_FORMAT_WAV | SF_FORMAT_PCM_16) ;
    ceeplusplus_test ("cpp_test.aiff", SF_FORMAT_AIFF | SF_FORMAT_PCM_S8) ;
    ceeplusplus_test ("cpp_test.au", SF_FORMAT_AU | SF_FORMAT_FLOAT) ;

    return 0 ;
} /* main */

/*
** Do not edit or modify anything in this comment block.
** The following line is a file identity tag for the GNU Arch
** revision control system.
**
** arch-tag: 06e48ee6-b19d-4453-9999-a5cf2d7bf0b6
*/
