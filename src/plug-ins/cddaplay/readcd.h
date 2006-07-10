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
   UCHAR frame;
   UCHAR second;
   UCHAR minute;
   UCHAR empty;
} MSF;

#define MODE_LBA 0
#define MODE_MSF 1

typedef struct
{
   UCHAR signature[4];
   UCHAR addressingMode; /* 0 = LBA, 1 = MSF */
   MSF start; /* can be LBA */
   MSF end;
} CDPLAYAUDIOPARAM;

typedef struct
{
   UCHAR firstTrack;
   UCHAR lastTrack;
   MSF leadOutAddress;
} CDAUDIODISKINFODATA;

typedef struct
{
   UCHAR signature[4];
   UCHAR trackNum;
} CDAUDIOTRACKINFOPARAM;

typedef struct
{
   MSF address;
   UCHAR info;
   #if 0
      bit 7  4 or 2 channels
      bit 6  data or audio track
      bit 5  copy ok or not ok
      bit 4  with or without preemp
      bit 3-0 ADR stuff?? whatever that is
   #endif
} CDAUDIOTRACKINFODATA;

typedef struct
{
   MSF start;
   MSF end;
   MSF length;
   ULONG size;
   BOOL data;
   USHORT channels;
   USHORT number;
} CDTRACKINFO;

#pragma pack(1)
typedef struct
{
   UCHAR signature[4];
   UCHAR addressingMode; /* 0 = LBA, 1 = MSF */
   USHORT numberSectors;
   ULONG startSector;
   UCHAR reserved;
   UCHAR interleaveSize;
   UCHAR interleaveSkipFactor;
} CDREADLONGPARAM;
#pragma pack()

typedef struct
{
   UCHAR sync[12];
   UCHAR header[4];
   UCHAR data[2048];
   UCHAR EDC_ECC[288];
} CDREADLONGDATA;

typedef struct
{
   UCHAR ControlADR;
   UCHAR UPC[7];
   UCHAR Reserved;
   UCHAR Frame;
} CDGETUPCDATA;


class CD_drive
{
   public:
      CD_drive();
      ~CD_drive();

      BOOL open(char *drive);
      BOOL close();
      BOOL readCDInfo();
      BOOL fillTrackInfo();
      BOOL play(char track);
      BOOL stop();
      BOOL readSectors(CDREADLONGDATA data[], ULONG number, ULONG start);

      char getCount() { return cdInfo.lastTrack - cdInfo.firstTrack + 1; };
      CDTRACKINFO *getTrackInfo(char track)
         {
           if(trackInfo == NULL) return NULL;
           else if(track > cdInfo.lastTrack || track < cdInfo.firstTrack) return NULL;
           else return &trackInfo[track-cdInfo.firstTrack]; };
      CDAUDIODISKINFODATA *getCDInfo()
         { return &cdInfo; };

      USHORT getTime()
         { return (cdInfo.leadOutAddress.minute*60 + cdInfo.leadOutAddress.second) -
                  (trackInfo[0].start.minute*60 + trackInfo[0].start.second); };


      static ULONG getLBA(MSF input)
      {
         return (input.minute * 4500 + input.second * 75 + input.frame - 150);
      };

      static MSF getMSF(ULONG input)
      {
         MSF output;

         input += 150;
         output.frame = input % 75;
         input /= 75;
         output.second = input % 60;
         output.minute = input / 60;

         return output;
      };

      BOOL getUPC(char UPC[7]);
      char *getDriveLetter() { return driveLetter; }

      void (PM123_ENTRYP error_display)(char *);

   protected:
      BOOL opened; /* hCDDrive cannot be negative and 0 is a valid handle
                      so I keep another variable around */
      HFILE hCDDrive;
      CDAUDIODISKINFODATA cdInfo;
      CDTRACKINFO *trackInfo;
      char *driveLetter;

      void updateError(char *fmt, ...);

      BOOL readTrackInfo(char track, CDTRACKINFO *trackInfo);
};

