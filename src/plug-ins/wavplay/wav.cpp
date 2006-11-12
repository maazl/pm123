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

/* RIFF stuff */

#define INCL_DOS
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <io.h>
#include <fcntl.h>
#include <sys\stat.h>
#include <sys\types.h>
#include <memory.h>

#include "wav.h"

WAV::WAV()
{
    hfile = -1;
    memset(&header,0,sizeof(header));
}

WAV::~WAV()
{
   if(hfile != -1)
      close();
}

int WAV::open(const char *filename, MODE mode, int &samplerate,
              int &channels, int &bits, int &format)
{
   switch(mode)
   {
      case READ:
         hfile = ::open(filename,O_RDONLY | O_BINARY, S_IWRITE);
         if(!hfile || hfile == -1) return errno;
         if((headersize = readHeader()) == 0 ) return NO_WAV_FILE;
         bits = header.WAVE.fmt.BitsPerSample;
         channels = header.WAVE.fmt.Channels;
         samplerate = header.WAVE.fmt.SamplesPerSec;
         format = header.WAVE.fmt.FormatTag;
         break;

      case CREATE:
         hfile = ::open(filename,O_CREAT | O_TRUNC | O_RDWR | O_BINARY, S_IWRITE);
         if(!hfile || hfile == -1) return errno;
         initHeader(samplerate, channels, bits, format);
         if(write(hfile, &header, sizeof(header)) != sizeof(header))
            return errno;
         break;

      case APPEND:
         struct stat fi = {0};
         hfile = ::open(filename,O_CREAT | O_RDWR | O_BINARY, S_IWRITE);
         if(!hfile || hfile == -1) return errno;
         lseek(hfile,0,SEEK_SET);
         if((headersize = readHeader()) == 0 ) return NO_WAV_FILE;
         fstat(hfile,&fi);
         header.WAVE.datalen = fi.st_size - sizeof(header);
         header.WAVElen = header.WAVE.datalen + sizeof(header.WAVE);
         if(write(hfile, &header, sizeof(header)) != sizeof(header))
            return errno;
         lseek(hfile,0,SEEK_END);
         break;
   }
   return 0;
}

int WAV::close()
{
   int temp = hfile;
   hfile = -1;
   return ::close(temp);
}

/* increments the datalen bytes too */

int WAV::writeData(char *buffer, int bytes)
{
   int temp;
   if(hfile == -1) return 0;

   temp = write(hfile, buffer, bytes);

   header.WAVE.datalen += temp;
   header.WAVElen += temp;

   lseek(hfile,0,SEEK_SET);
   write(hfile, &header, sizeof(header));

   lseek(hfile,0,SEEK_END);

   return temp;
}

int WAV::readData(char *buffer, int bytes)
{
   if(hfile == -1) return 0;
   return read(hfile, buffer, bytes);
}

int WAV::filepos()
{
   if(hfile == -1) return 0;
   return tell(hfile) - headersize;
}

int WAV::filelength()
{
   if(hfile == -1) return 0;
   return ::filelength(hfile) - headersize;
}


int WAV::jumpto(long offset)
{
   if(hfile == -1) return 0;
   return lseek(hfile, offset+headersize,SEEK_SET);
}


int WAV::readHeader()
{
   if(hfile == -1) return 0;

   lseek(hfile, 0, SEEK_SET);
   read(hfile, &header.R1,4);
   if(header.R1 == 'R' && header.I2 == 'I' && header.F3 == 'F' && header.F4 == 'F')
   {
      read(hfile, &header.WAVElen,4);
      read(hfile, &header.WAVE.W1,4);
      if(header.WAVE.W1 == 'W' && header.WAVE.A2 == 'A' && header.WAVE.V3 == 'V' && header.WAVE.E4 == 'E')
      {
         int foundfmt = 0;
         do
         {
            if(!read(hfile, &header.WAVE.f5,4)) break;
            if(!read(hfile, &header.WAVE.fmtlen,4)) break;
            if(header.WAVE.f5 == 'f' && header.WAVE.m6 == 'm' && header.WAVE.t7 == 't' &&
               header.WAVE.fmtlen >= sizeof(header.WAVE.fmt))
            {
               if(!read(hfile, &header.WAVE.fmt,sizeof(header.WAVE.fmt))) break;
               lseek(hfile,header.WAVE.fmtlen-sizeof(header.WAVE.fmt),SEEK_CUR);
               foundfmt = 1;
            }
            else
            {
               /* we skip things we don't know */
               if(lseek(hfile,header.WAVE.fmtlen,SEEK_CUR)==-1) break;
            }
         }
         while(!foundfmt);

         if(foundfmt)
         {
            char buffer[4];
            unsigned long length;
            /* if this is not a standard PCM sample, abort */
            //if(header.WAVE.fmt.FormatTag != 1) return 0;

            do
            {
               if(!read(hfile, buffer,4)) break;
               if(!read(hfile, &length,4)) break;

               if(buffer[0] == 'd' && buffer[1] == 'a' && buffer[2] == 't' && buffer[3] == 'a')
               {
                  header.WAVE.d8 = 'd';
                  header.WAVE.a9 = 'a';
                  header.WAVE.t10 = 't';
                  header.WAVE.a11 = 'a';
                  header.WAVE.datalen = length;
                  return tell(hfile); /* tada */
               }
               else
                  /* we skip things we don't know */
                  if(lseek(hfile,length,SEEK_CUR) == -1) break;
            }
            while(1);
         }
      }
   }
   return 0;
}

void WAV::initHeader(int samplerate, int channels, int bits, int format)
{
   header.R1 = 'R';
   header.I2 = 'I';
   header.F3 = 'F';
   header.F4 = 'F';
   header.WAVElen = sizeof(header.WAVE); /* + datalen */
   header.WAVE.W1 = 'W';
   header.WAVE.A2 = 'A';
   header.WAVE.V3 = 'V';
   header.WAVE.E4 = 'E';
   header.WAVE.f5 = 'f';
   header.WAVE.m6 = 'm';
   header.WAVE.t7 = 't';
   header.WAVE.space = ' ';
   header.WAVE.fmtlen = sizeof(header.WAVE.fmt);
   header.WAVE.fmt.FormatTag = format;
   header.WAVE.d8 = 'd';
   header.WAVE.a9 = 'a';
   header.WAVE.t10 = 't';
   header.WAVE.a11 = 'a';
   header.WAVE.datalen = 0; /* we don't know yet */

   header.WAVE.fmt.BitsPerSample = bits;
   header.WAVE.fmt.Channels = channels;
   header.WAVE.fmt.SamplesPerSec = samplerate;

   header.WAVE.fmt.AvgBytesPerSec =
         header.WAVE.fmt.Channels *
         header.WAVE.fmt.SamplesPerSec *
         header.WAVE.fmt.BitsPerSample / 8;
   header.WAVE.fmt.BlockAlign =
         header.WAVE.fmt.Channels *
         header.WAVE.fmt.BitsPerSample / 8;
}
