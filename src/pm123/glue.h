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
* virtual output interface, including filter plug-ins
* This class logically connects the decoder and the output interface.
*
****************************************************************************/
class Glue
{public:
  /// Decoder event arguments
  typedef struct
  { DECEVENTTYPE Type;
    void*        Param;
  } DecEventArgs;

 protected:
  static PM123_TIME MinPos;            ///< Minimum sample position of a block from the decoder since the last DecPlay
  static PM123_TIME MaxPos;            ///< Maximum sample position of a block from the decoder since the last DecPlay
  /// Decoder events
  static event<const DecEventArgs> DecEvent;
  /// Output events
  static event<const OUTEVENTTYPE> OutEvent;

 public: // Control interface for the decoder engine, not thread safe
  /// Invoke decoder to play an URL
  static ULONG DecPlay(APlayable& song, PM123_TIME offset, PM123_TIME start, PM123_TIME stop);
  /// Stop the current decoder immediately
  static ULONG DecStop();
  /// Close decoder plug-in. This may block until the decoder thread has ended.
  /// @pre DecStop must have been sent if the decoder has been started.
  static void  DecClose();
  /// Set fast forward/rewind mode
  static ULONG DecFast(DECFASTMODE mode);
  /// Jump to absolute position
  static ULONG DecJump(PM123_TIME location);
  /// Set savefilename to save the raw stream data
  static ULONG DecSave(const char* file);
  /// get the minimum sample position of a block from the decoder since the last dec_play
  static PM123_TIME DecMinPos() { return MinPos; }
  /// get the maximum sample position of a block from the decoder since the last dec_play
  static PM123_TIME DecMaxPos() { return MaxPos; }

  /// Decoder events
  static event_pub<const DecEventArgs>& GetDecEvent() { return DecEvent; }
  /// Output events
  static event_pub<const OUTEVENTTYPE>& GetOutEvent() { return OutEvent; }

 public: // Control interface for the output engine, not thread safe
  /// Prepare output for playback of \a song.
  /// If the output is already initialized this signals only the start of a new song.
  static ULONG OutSetup(const APlayable& song);
  /// Close the filter chain including the output plug-in.
  /// If the playback has not yet completed, discard all remaining buffers.
  /// \c OutClose must release all semaphores in the \c output_request_buffer and \c output_commit_buffer callbacks.
  /// The function may return an error in this case.
  static ULONG OutClose();
  /// Adjust output volume.
  static void  OutSetVolume(double volume); // volume: [0,1]
  /// Pause output.
  static ULONG OutPause(bool pause);
  /// Send Signal to the filter chain to flush all buffers and complete playing.
  /// When the last sample has been played OUTEVENT_END_OF_DATA is sent.
  static bool  OutFlush();
  /// Tell the filter chain to discard all internal buffers.
  /// @remarks This is e.g. used after a seek command.
  static bool  OutTrash();

 public: // Status interface for the output engine, thread safe
  PROXYFUNCDEF PM123_TIME DLLENTRY OutPlayingPos();
  PROXYFUNCDEF BOOL       DLLENTRY OutPlayingData();
  PROXYFUNCDEF ULONG      DLLENTRY OutPlayingSamples(PM123_TIME offset, OUTPUT_PLAYING_BUFFER_CB cb, void* param);
};

#endif /* PM123_GLUE_H */

