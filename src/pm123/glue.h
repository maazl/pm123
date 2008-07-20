/*
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp„  <rosmo@sektori.com>
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 2006-2008 Marcel Mueller
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

#ifndef PM123_GLUE_H
#define PM123_GLUE_H

//#include "plugman.h"

#define INCL_PM
#include <output_plug.h>
#include <decoder_plug.h>
#include "playable.h"
#include <cpp/stringmap.h>
#include <os2.h>


/****************************************************************************
*
*  Control interface for the decoder engine
*  Not thread safe
*
****************************************************************************/
/* invoke decoder to play an URL */
ULONG dec_play( const Song* song, double offset, double start, double stop );
/* stop the current decoder immediately */
ULONG dec_stop( void );
/* set fast forward/rewind mode */
ULONG dec_fast( DECFASTMODE mode );
/* jump to absolute position */
ULONG dec_jump( double location );
/* set savefilename to save the raw stream data */
ULONG dec_save( const char* file );
/* edit ID3-data of the given file, decoder_name is optional */
ULONG dec_editmeta( HWND owner, const char* url, const char* decoder_name );
/* get the minimum sample position of a block from the decoder since the last dec_play */
double dec_minpos();
/* get the maximum sample position of a block from the decoder since the last dec_play */
double dec_maxpos();
// Decoder events
typedef struct
{ DECEVENTTYPE type;
  void*        param;
} dec_event_args;
extern event<const dec_event_args> dec_event;
// Output events
extern event<const OUTEVENTTYPE> out_event;

/****************************************************************************
*
*  Status interface for the decoder engine
*  Thread safe
*
****************************************************************************/
/* check whether the specified decoder is currently in use */
BOOL  dec_is_active( int number );

ULONG DLLENTRY dec_fileinfo( const char* filename, INFOTYPE* what, DECODER_INFO2* info, char* name, size_t name_size );
ULONG DLLENTRY dec_cdinfo( const char* drive, DECODER_CDINFO* info );

ULONG DLLENTRY dec_status( void );
double DLLENTRY dec_length( void );

/* gets a merged list of the file extensions supported by the enabled decoders */
void dec_merge_extensions(stringset& list);
/* gets a merged list of the file types supported by the enabled decoders */
void dec_merge_file_types(stringset& list);

/****************************************************************************
*
*  Control interface for the output engine
*  Not thread safe
*
****************************************************************************/
ULONG out_setup( const Song* song );
ULONG out_close( void );
void  out_set_volume( double volume ); // volume: [0,1]
ULONG out_pause( BOOL pause );
BOOL  out_flush( void );
BOOL  out_trash( void );

/****************************************************************************
*
*  Status interface for the output engine
*  Thread safe
*
****************************************************************************/
/*ULONG DLLENTRY out_playing_samples( FORMAT_INFO* info, char* buf, int len );*/
double DLLENTRY out_playing_pos( void );
BOOL  DLLENTRY out_playing_data( void );

/****************************************************************************
*
*  Control interface for the visual plug-ins
*  Not thread safe
*
****************************************************************************/
/* initialize non-skinned visual plug-in */
bool  vis_init(size_t i);
/* initialize all skinned/non-skinned visual plug-ins */
void  vis_init_all(bool skin);
void  vis_broadcast( ULONG msg, MPARAM mp1, MPARAM mp2 );
/* deinitialize visual plug-in */
bool  vis_deinit(size_t i);
void  vis_deinit_all(bool skin);


#endif /* PM123_GLUE_H */

