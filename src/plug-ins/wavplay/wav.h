/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
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

typedef struct
{
   char R1;
   char I2;
   char F3;
   char F4;
   unsigned long WAVElen;
   struct
   {
      char W1;
      char A2;
      char V3;
      char E4;
      char f5;
      char m6;
      char t7;
      char space;
      unsigned long fmtlen;
      struct
      {
         unsigned short FormatTag;
         unsigned short Channels;
         unsigned long SamplesPerSec;
         unsigned long AvgBytesPerSec;
         unsigned short BlockAlign;
         unsigned short BitsPerSample; /* format specific for PCM */
      } fmt;
      char d8;
      char a9;
      char t10;
      char a11;
      unsigned long datalen;
      /* from here you insert your PCM data */
   } WAVE;
} RIFF;

/* warning: append mode doesn't take in account unknown RIFF chunks */
enum MODE { READ, CREATE, APPEND };

/* AFAIK, all of those also have BitsPerSample as format specific */
#define WAVE_FORMAT_PCM       0x0001
#define WAVE_FORMAT_ADPCM     0x0002
#define WAVE_FORMAT_ALAW      0x0006
#define WAVE_FORMAT_MULAW     0x0007
#define WAVE_FORMAT_OKI_ADPCM 0x0010
#define WAVE_FORMAT_DIGISTD   0x0015
#define WAVE_FORMAT_DIGIFIX   0x0016
#define IBM_FORMAT_MULAW      0x0101
#define IBM_FORMAT_ALAW       0x0102
#define IBM_FORMAT_ADPCM      0x0103

#define NO_WAV_FILE 200

class WAV
{
   public:

      WAV();
      ~WAV();
      int open(const char *filename, MODE mode, int &samplerate,
               int &channels, int &bits, int &format);
      int close();
      int readData(char *buffer, int bytes);
      int writeData(char *buffer, int bytes);
      int filepos();
      int jumpto(long offset);
      int filelength();

   protected:

      void initHeader(int samplerate, int channels, int bits, int format);
      int  readHeader();

      RIFF header;
      int  hfile;
      int  headersize;
};
