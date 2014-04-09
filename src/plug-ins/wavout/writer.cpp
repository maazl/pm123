/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
 *           2013      Marcel Mueller
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
 *
 * PM123 WAVE Output plug-in.
 */

#define  INCL_PM
#define  INCL_DOS
#define PLUGIN_INTERFACE_LEVEL 3
#include "writer.h"

#include <output_plug.h>
#include <debuglog.h>
#include <os2.h>

#include <stdlib.h>
#include <time.h>

volatile xstring WAVOUT::OutPath;
bool WAVOUT::UseTimestamp = false;


WAVOUT::WAVE_HEADER::WAVE_HEADER()
{ Riff.IdRiff      = *(ID*)"RIFF";
  Riff.IdWave      = *(ID*)"WAVE";
  FormatHeader.Id  = *(ID*)"fmt ";
  FormatHeader.Len = sizeof Format;
  DataHeader.Id    = *(ID*)"data";
}

void WAVOUT::WAVE_HEADER::SetFormat(unsigned channels, unsigned samplingrate, unsigned bitspersample)
{ Format.FormatTag = 1; // PCM
  Format.Channels = channels;
  Format.SamplesPerSec = samplingrate;
  Format.AvgBytesPerSec = samplingrate * channels * bitspersample / 8;
  Format.BlockAlign = channels * bitspersample / 8;
  Format.BitsPerSample = bitspersample;
}

void WAVOUT::WAVE_HEADER::SetLength(int64_t bytes)
{ bytes -= 8;
  DataHeader.Len = bytes > 0xffffffffU ? 0xffffffffU : (unsigned)bytes;
  bytes -= sizeof *this - 8;
  Riff.Len = bytes > 0xffffffffU ? 0xffffffffU : (unsigned)bytes;
}

// control interface
WAVOUT::OUTPUT_STRUCT()
: File(NULL)
, IsError(false)
, EvPause()
, CurrentBuffer(0)
{ Format.channels = 0;
  memset(FloatBuffer, 0, sizeof FloatBuffer);
  EvPause.Set();
}

// data interface
bool WAVOUT::OpenFile(const url123& song)
{
  // create file name
  FileName.clear();
  FileName.append(xstring(OutPath));
  if ((IsTimestampFile = UseTimestamp) != 0)
  { time_t ti;
    time(&ti);
    struct tm* t = localtime(&ti);
    FileName.appendf("%04u-%02u-%02u_%02u-%02u-%02u.wav",
      t->tm_year+1900, t->tm_mon+1, t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
  } else
  { FileName.append(song.getShortName());
    FileName.append(".wav");
  }
  // open target
  File = xio_fopen(FileName.cdata(), "wbU");
  if (File == NULL)
  { Ctx.plugin_api->message_display(MSG_ERROR,
      xstring().sprintf("Could not open file %s for output:\n%s",
        FileName.cdata(), xio_strerror(xio_errno())));
    return false;
  }
  // write preliminary WAV header.
  Header.SetFormat(Format.channels, Format.samplerate, 16);
  Header.SetLength(0xffffffd4U);
  return WriteFile(&Header, sizeof Header);
}

bool WAVOUT::WriteFile(const void* buffer, size_t count)
{
  if (xio_fwrite(buffer, count, 1, File) != 1)
  { Ctx.plugin_api->message_display(MSG_ERROR,
      xstring().sprintf("Could not write data to file %s:\n%s",
        FileName.cdata(), xio_strerror(xio_ferror(File))));
    return false;
  }
  return true;
}

void WAVOUT::CloseFile()
{ DEBUGLOG(("WAVOUT::CloseFile() - %p\n", File));
  if (File)
  { Header.SetLength(xio_ftelll(File));
    if (xio_fseekl(File, 0, XIO_SEEK_SET) == 0)
      xio_fwrite(&Header, 1, sizeof Header, File);
    xio_fclose(File);
    File = NULL;
  }
}

void WAVOUT::ConvertSamples(const float* sp, size_t count)
{
  short* dp = ShortBuffer;
  short* dpe = dp + count;
  while (dp != dpe)
  { double value = *sp++ * 32767.;
    *dp++ = value > 32767. ? 32767 : value < -32768. ? -32768 : (short)value;
  }
}

/* This function is used by visual plug-ins so the user can
   visualize what is currently being played. */
ULONG WAVOUT::PlayingSamples(PM123_TIME offset, OUTPUT_PLAYING_BUFFER_CB cb, void* param)
{ int buf;
  FORMAT_INFO2 format;
  PM123_TIME pos;
  { Mutex::Lock lock(Mtx);
    buf = CurrentBuffer ^ 1;
    format = Format;
    pos = PlayingPos;
  }
  if (IsError || !format.channels)
    return PLUGIN_FAILED;
  BOOL done = true;
  (*cb)(param, &format, FloatBuffer[buf], BUFFERSIZE / format.channels, pos, &done);
  return PLUGIN_OK;
}


/* This function is called by the decoder or last in chain
   filter plug-in to play samples. */
int WAVOUT::RequestBuffer(const FORMAT_INFO2* format, float** buf)
{ DEBUGLOG(("WAVOUT(%p)::RequestBuffer({ %i, %i}, %p) - %i, %i\n", this,
    format ? format->samplerate : -2, format ? format->channels : -2, buf, Format.samplerate, Format.channels));
  ASSERT(PlayQueue.size());

  if (!buf)
  { (*OutEvent)(W, OUTEVENT_END_OF_DATA);
    return 0;
  }

  if (IsError)
    return -1;

  // Check for format changes.
  if (File && (Format.channels != format->channels || Format.samplerate != format->samplerate))
  { // Format has changed.
    if (!IsTimestampFile)
    { (*Ctx.plugin_api->message_display)(MSG_WARNING, xstring().sprintf(
        "The sampling rate or the no. of channels has changed within a song.\n"
        "(%i, %i) -> (%i, %i)\n"
        "Generation of a probably invalid WAV file.",
        Format.channels, Format.samplerate, format->channels, format->samplerate));
    } else
    { CloseFile();
    }
  }
  { Mutex::Lock lock(Mtx);
    Format = *format;
  }

  EvPause.Wait();

  *buf = FloatBuffer[CurrentBuffer];
  return BUFFERSIZE / format->channels;
}

void WAVOUT::CommitBuffer(int len, PM123_TIME pos)
{ DEBUGLOG(("WAVOUT(%p)::CommitBuffer(%i, %f)\n", this, len, pos));

  if (IsError || len <= 0)
    return;

  PM123_TIME end_pos = pos + (PM123_TIME)Format.samplerate * len;
  const float* data = FloatBuffer[CurrentBuffer];
 restart:
  // Open file first?
  if (!File)
  { const url123* urlp;
    { Mutex::Lock lock(Mtx);
      urlp = &PlayQueue[0]->URL;
    }
    if ((IsError = !OpenFile(*urlp)) != false)
      return;
  }

  int count;
  int reduced_len = len;
  { // Check for next song.
    Mutex::Lock lock(Mtx);
    if (PlayQueue.size() > 1 && end_pos > PlayQueue[1]->Start)
    { double part1 = PlayQueue[1]->Start - pos;
      if (part1 <= 0)
        goto nextsong;
      reduced_len = (int)(part1 * Format.samplerate + .5);
    }
  }

  // Convert samples to 16 bit
  count = reduced_len * Format.channels;
  ConvertSamples(data, count);
  IsError = !WriteFile(ShortBuffer, count * sizeof(short));

  len -= reduced_len;
  if (len)
  { data += count;
   nextsong:
    // Was next song condition.
    if (!IsTimestampFile)
      CloseFile();
    Mutex::Lock lock(Mtx);
    delete PlayQueue.erase(0);
    goto restart;
  }

  Mutex::Lock lock(Mtx);
  PlayingPos = pos;
  CurrentBuffer ^= 1;
}

