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

#ifndef READCD_H
#define READCD_H

#include "cddb.h"


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
   /*
      bit 7  4 or 2 channels
      bit 6  data or audio track
      bit 5  copy ok or not ok
      bit 4  with or without preemp
      bit 3-0 ADR stuff?? whatever that is
   */
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


/* Class to handle CD-Audio API
 * This class is thread safe.
 */
class CD_drive
{
   private:
      char driveLetter[4];            // X:
   protected:
      CDAUDIODISKINFODATA cdInfo;
      CDTRACKINFO *trackInfo;
      CDDB_socket cddb;               // use a private instance => now thread safe
      CDDBQUERY_DATA querydata;
      
      BOOL opened; /* hCDDrive cannot be negative and 0 is a valid handle
                      so I keep another variable around */
      HFILE hCDDrive;
      char* error;                    // last error text
      
      HMTX instMtx;                   // mutex to protect *this
      int instCount;                  // reference counter
      BOOL exclusive;                 // instance in exclusive read access

   protected:
      // This class manages it's liftime alone, so we protect the constructor/destrutor.
      CD_drive(char drive);
      ~CD_drive();

   public:
      // MSF to LBA
      static ULONG getLBA(MSF input)
      {  return (input.minute * 4500 + input.second * 75 + input.frame - 150); }
      // LBA to MSF
      static MSF getMSF(ULONG input)
      {
         MSF output;

         input += 150;
         output.frame = input % 75;
         input /= 75;
         output.second = input % 60;
         output.minute = input / 60;

         return output;
      }

      // Return my drive letter.
      const char* getDriveLetter() const
      {  return driveLetter; }
      BOOL isValid() const
      {  return cdInfo.lastTrack != 0; }
      // Get the number of tracks.
      char getCount() const
      {  return cdInfo.lastTrack - cdInfo.firstTrack + 1; }
      // Get information about a track.
      const CDTRACKINFO *getTrackInfo(char track) const
      {  return trackInfo == NULL || track > cdInfo.lastTrack || track < cdInfo.firstTrack
            ? NULL
            : trackInfo + (track-cdInfo.firstTrack); }
      // Get general information about the CD.
      const CDAUDIODISKINFODATA *getCDInfo() const
      {  return &cdInfo; }
      // Get length of the CD in seconds.
      USHORT getTime() const
      {  return (cdInfo.leadOutAddress.minute*60 + cdInfo.leadOutAddress.second) -
                (trackInfo[0].start.minute*60 + trackInfo[0].start.second); }
      // Get CDDB information or NULL if not available.
      const CDDB_socket* getCDDBInfo() const
      {  return cddb.isInfoValid() ? &cddb : NULL; }

      // open CD device
      BOOL open();
      // close CD device
      BOOL close();
      // Play track.
      BOOL play(char track);
      // Stop playback.
      BOOL stop();
      // Read PCM data.
      BOOL readSectors(CDREADLONGDATA data[], ULONG number, ULONG start);
      // Get UPC code of the CD.
      BOOL getUPC(char UPC[7]);
 
      // Get the last error text or NULL if none.      
      const char* getError() const
      {  return error; }

      void clearError()
      {  free(error);
         error = NULL;
      }

   private:
      CD_drive(const CD_drive&);      // prohibit copies, unused and unimplemented
      void operator=(const CD_drive&);// prohibit assignment, unused and unimplemented

   protected:
      BOOL readCDInfo();

      BOOL fillTrackInfo();
      BOOL readTrackInfo(char track, CDTRACKINFO *trackInfo);

      BOOL loadCDDBInfo(BOOL refresh = FALSE);

      void updateError(char *fmt, ...);

   protected:
      // Factory and cache
      class Cache;
      friend class CD_drive::Cache;
      class Cache
      {
         protected:
            CD_drive* cache[26];// cache current infos
            HMTX cacheMtx;     // mutex to protect the cache
         public:
            Cache();
            ~Cache();
            CD_drive* lookup(char index)
            {  DosRequestMutexSem(cacheMtx, SEM_INDEFINITE_WAIT);
               return cache[index]; }
            void store(char index, CD_drive* value)
            {  cache[index] = value; }
            void release()
            {  DosReleaseMutexSem(cacheMtx); }
      };
      static Cache cache;

   protected:
      // get access to a CD_drive object
      static CD_drive* request(char drive, BOOL exclusive, BOOL refresh);
      void release(BOOL exclusive = FALSE);
      
   public:   
      /* Proxy class to access const functions of CD_drive objects thread safe.
       */
      class AccessInfo;
      friend class CD_drive::AccessInfo;
      class AccessInfo
      {
         public:
            AccessInfo(char drive, BOOL refresh = FALSE)
            {  instance = request(drive, FALSE, refresh);
            }
            ~AccessInfo()
            {  if (instance)
                  instance->release(FALSE);
            }
            
            BOOL isValid() const { return instance != NULL; }
            const CD_drive* operator->() { return instance; }

         private:
            CD_drive* instance;
      };

      /* Proxy class to access CD_drive objects thread safe.
       * For the time of the existance of this class the underlying CD_drive instance ist locked.
       */ 
      class AccessRead;
      friend class CD_drive::AccessRead;
      class AccessRead
      {
         public:
            AccessRead(char drive, BOOL refresh = FALSE)
            {  instance = request(drive, TRUE, refresh);
            }
            ~AccessRead()
            {  if (instance)
                  instance->release(TRUE);
            }
            
            BOOL isValid() const { return instance != NULL; }
            CD_drive* operator->() { return instance; }

         private:
            CD_drive* instance;
      };
};

#endif
