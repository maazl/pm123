/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp� <rosmo@sektori.com>
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

/*
 File handling for reading and writing MP3-TAG, a format by Damaged
 Cybernetics.
 *
 Source by Thorvald Natvig
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io.h>
#include "tag.h"

/* This one includes the table of the genres. Autogenerated */
#include "genre.h"

void safecopy(char *to,char *from, int maxlen) {
   int where;
   strncpy(to,from,maxlen);
   to[maxlen]=0;
   for ( where=maxlen-1 ; ((where>=0) && (to[where]==' ')) ; where-- ) {
      to[where]=0 ;
   }
}

void spacecopy(char *to,char *from, int maxlen) {
   int where;
   strncpy(to,from,maxlen);
   for ( where=maxlen-1 ; ((where>=0) && (to[where]==0)) ; where--) {
      to[where]=' ';
   }
}

int wipetag(int fd,size_t filesize) {
   char buffer[4];
   lseek(fd,-128,SEEK_END);
   read(fd,&buffer,3);
   if (! strncmp(buffer,"TAG",3)) {
      lseek(fd,0,SEEK_SET);
      if(_chsize(fd,filesize-128)==0)
   return(1);
      else return(0);
   } else {
      return(1);
   }
}


int gettag(int fd, tune *info) {
   tag song;
   lseek(fd, -128, SEEK_END) ;
   read(fd, &song, 128);

   if (! strncmp(song.tag,"TAG",3)) {
      safecopy(info->title,song.title,30);
      safecopy(info->artist,song.artist,30);
      safecopy(info->album,song.album,30);
      safecopy(info->year,song.year,4);
      safecopy(info->comment,song.comment,30);
      info->gennum= song.genre & 0xFF;
      if (info->gennum >= GENRE_LARGEST) {
    info->gennum = GENRE_LARGEST;
      }
      strcpy(info->genre,genres[info->gennum]);
      return(1);
   } else {
      return(0);
   }
}

void jointag(tune *to, tune *from) {
   if (from->title[0]) strncpy(to->title,from->title,30);
   if (from->artist[0]) strncpy(to->artist,from->artist,30);
   if (from->album[0]) strncpy(to->album,from->album,30);
   if (from->year[0]) strncpy(to->year,from->year,4);
   if (from->comment[0]) strncpy(to->comment,from->comment,30);
   if (from->gennum != -1 ) to->gennum=from->gennum;
   if (to->gennum >= GENRE_LARGEST) {
      to->gennum=0;
   }
   strcpy(to->genre,genres[to->gennum]);
}

int settag(int fd, tune *info) {
   int temp;
   tag song;

   spacecopy(song.title,info->title,30);
   spacecopy(song.artist,info->artist,30);
   spacecopy(song.album,info->album,30);
   spacecopy(song.year,info->year,4);
   spacecopy(song.comment,info->comment,30);

   if (info->gennum > GENRE_LARGEST) {
      song.genre=0;
   } else {
      temp=info->gennum;
      song.genre=(unsigned char)temp;
   }

   lseek(fd, -128, SEEK_END) ;
   temp=read(fd, &song, 3) ;
   if (! strncmp(song.tag,"TAG",3)) {
      lseek(fd, -128, SEEK_END) ;
   } else {
      lseek(fd, 0 , SEEK_END) ;
   }
   strncpy(song.tag,"TAG",3);
   temp=write(fd,&song,128);
   if (temp==128) {
      return (1) ;
   } else {
      return (0) ;
   }
}

void printgenres() {
  int i;
  for (i=0; i<=GENRE_LARGEST;i++) {
    printf("%2i : %s\n",i,genres[i]);
  }
}

