/*
 * Copyright 2006-2011 Marcel Mueller
 * Copyright 2004-2006 Dmitry A.Steklenev <glass@ptv.ru>
 * Copyright 1997-2003 Samuel Audet  <guardia@step.polymtl.ca>
 *                     Taneli Lepp�  <rosmo@sektori.com>
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


#define INCL_PM
#include <output_plug.h>
#include <decoder_plug.h>
#include "playable.h"
#include <os2.h>


/****************************************************************************
*
*  Control interface for the decoder engine
*  Not thread safe
*
****************************************************************************/
/// Invoke decoder to play an URL
ULONG dec_play(const APlayable& song, PM123_TIME offset, PM123_TIME start, PM123_TIME stop);
/// Stop the current decoder immediately
ULONG dec_stop();
/// Set fast forward/rewind mode
ULONG dec_fast(DECFASTMODE mode);
/// Jump to absolute position
ULONG dec_jump(PM123_TIME location);
/// Set savefilename to save the raw stream data
ULONG dec_save(const char* file);
/// get the minimum sample position of a block from the decoder since the last dec_play
PM123_TIME dec_minpos();
/// get the maximum sample position of a block from the decoder since the last dec_play
PM123_TIME dec_maxpos();
/// Decoder events
typedef struct
{ DECEVENTTYPE type;
  void*        param;
} dec_event_args;
extern event<const dec_event_args> dec_event;

/// Output events
extern event<const OUTEVENTTYPE> out_event;


/****************************************************************************
*
*  Control interface for the output engine
*  Not thread safe
*
****************************************************************************/
ULONG out_setup( const APlayable& song );
ULONG out_close( );
void  out_set_volume( double volume ); // volume: [0,1]
ULONG out_pause( BOOL pause );
BOOL  out_flush( );
BOOL  out_trash( );

/****************************************************************************
*
*  Status interface for the output engine
*  Thread safe
*
****************************************************************************/
ULONG DLLENTRY out_playing_samples( FORMAT_INFO2* info, float* buf, int len );
PM123_TIME DLLENTRY out_playing_pos( );
BOOL  DLLENTRY out_playing_data( );


#endif /* PM123_GLUE_H */

