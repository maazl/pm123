#include <ctype.h>
#include <stdlib.h>
#include <io.h>
#include <signal.h>
#include "mpg123.h"
#include "tables.h"

/* max = 1728 */
#define MAXFRAMESIZE 1792

#define SKIP_JUNK 1

int tabsel_123[2][3][16] = {
   { {0,32,64,96,128,160,192,224,256,288,320,352,384,416,448,},
     {0,32,48,56, 64, 80, 96,112,128,160,192,224,256,320,384,},
     {0,32,40,48, 56, 64, 80, 96,112,128,160,192,224,256,320,} },

   { {0,32,48,56,64,80,96,112,128,144,160,176,192,224,256,},
     {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,},
     {0,8,16,24,32,40,48,56,64,80,96,112,128,144,160,} }
};

long freqs[9] = { 44100, 48000, 32000, 22050, 24000, 16000 , 11025 , 12000 , 8000 };

#ifdef I386_ASSEM
int bitindex;
unsigned char *wordpointer;
#else
static int bitindex;
static unsigned char *wordpointer;
#endif

static int fsize=0,fsizeold=0,ssize;
static unsigned char bsspace[2][MAXFRAMESIZE+512]; /* MAXFRAMESIZE */
static unsigned char *bsbuf=bsspace[1],*bsbufold;
static int bsnum=0;

struct ibuf {
   struct ibuf *next;
   struct ibuf *prev;
   unsigned char *buf;
   unsigned char *pnt;
   int len;
   /* skip,time stamp */
};

struct ibuf ibufs[2];
struct ibuf *cibuf;
int ibufnum=0;

unsigned char *pcm_sample = NULL;
int pcm_point = 0;
int audiobufsize = AUDIOBUFSIZE;

void get_II_stuff(struct frame *fr)
{
  static int translate[3][2][16] =
   { { { 0,2,2,2,2,2,2,0,0,0,1,1,1,1,1,0 } ,
       { 0,2,2,0,0,0,1,1,1,1,1,1,1,1,1,0 } } ,
     { { 0,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0 } ,
       { 0,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0 } } ,
     { { 0,3,3,3,3,3,3,0,0,0,1,1,1,1,1,0 } ,
       { 0,3,3,0,0,0,1,1,1,1,1,1,1,1,1,0 } } };

  int table,sblim;
  static struct al_table *tables[5] =
       { alloc_0, alloc_1, alloc_2, alloc_3 , alloc_4 };
  static int sblims[5] = { 27 , 30 , 8, 12 , 30 };
  if(fr->lsf)
    table = 4;
  else
    table = translate[fr->sampling_frequency][2-fr->stereo][fr->bitrate_index];
  sblim = sblims[table];

  fr->alloc = tables[table];
  fr->II_sblimit = sblim;
}

// returns playing position in ms... too CPU intensive?!
int decoding_pos(DECODER_STRUCT *w, struct frame *fr)
{
   _control87(EM_OVERFLOW | EM_UNDERFLOW | EM_ZERODIVIDE |
              EM_INEXACT | EM_INVALID | EM_DENORMAL, MCW_EM);

   if(w->XingVBRmode)
   {
      float tofind = (float) fr->filepos * 256 / w->Xingheader.bytes;
      float percent = 0.0;
      int i = 0;

      for(i = 1; i < 100; i++)
         if(tofind < w->XingTOC[i])
            break;

      percent = (float) (tofind - w->XingTOC[i-1]) / (w->XingTOC[i] - w->XingTOC[i-1]) + i-1;

      // 8 bit / byte and 8*144 samples / frame (?) for MP3
      return 1000 * percent * (8 * 144 * w->Xingheader.frames / freqs[fr->sampling_frequency] ) / 100;
   }
   else
   {
      int bytepers = (tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index] * 125);
      if(bytepers == 0) bytepers = 1;
      return (float)fr->filepos*1000/bytepers;
   }
}

int output_flush(DECODER_STRUCT *w, struct frame *fr)
{
   int rc = 1;
   if(pcm_point)
   {
      if(w->output_format.samplerate != freqs[fr->sampling_frequency]>>(fr->down_sample))
      {
         w->output_format.samplerate = freqs[fr->sampling_frequency]>>(fr->down_sample);
         init_eq(w->output_format.samplerate);
      }
      if( (*w->output_play_samples)(w->a,&w->output_format,pcm_sample,pcm_point,decoding_pos(w,fr)) < pcm_point)
         rc = 0;
      pcm_point = 0;
   }
   return rc;
}

size_t readdata(void *buffer, size_t size, size_t count, META_STRUCT *m)
{
   int read = 0;
   HTTP_INFO http_info;
   int metaint;
   sockfile_httpinfo(m->filept,&http_info);
   metaint = http_info.icy_metaint;

   // opening
   if(m->save_file == NULL && m->save_filename[0] != 0)
   {
      m->save_file = fopen(m->save_filename,"ab");
      if(m->save_file == NULL)
      {
         char temp[1024];
         sprintf(temp,"Could not open %s to save data.",m->save_filename);
         (*m->info_display)(temp);
         m->save_filename[0] = 0;
      }
   }
   // closing
   else if(m->save_filename[0] == 0 && m->save_file != NULL)
   {
      fclose(m->save_file);
      m->save_file = NULL;
   }


   if(metaint > 0)
   {
      int toread = size*count;
      int read2 = 0;
      char *metadata;

      if(m->data_until_meta == -1)
         m->data_until_meta = metaint;

      while(m->data_until_meta - toread < 0)
      {
         char metablocks = 0;
         int metasize = 0;
         int readmeta = 0;

         if(m->data_until_meta > 0)
         {
            read += _fread((char*)buffer+read,1,m->data_until_meta,m->filept);
            toread -= read;
         }

         readmeta += _fread(&metablocks,1,1,m->filept);
         if(readmeta == 1 && metablocks > 0)
         {
            metasize = metablocks*16;
            metadata = alloca(metasize + 1);
            metadata[metasize] = 0;

            readmeta += _fread(metadata,1,metasize,m->filept);
            if(readmeta == metasize+1 && m->metadata_buffer != NULL && m->metadata_size > 0)
            {
               if(strcmp(m->metadata_buffer,metadata) != 0)
               {
                  m->metadata_buffer[m->metadata_size-1] = 0;
                  strncpy(m->metadata_buffer,metadata,m->metadata_size-1);
                  WinPostMsg(m->hwnd,WM_METADATA,MPFROMP(m->metadata_buffer),0);
               }
            }
         }

         m->data_until_meta = metaint; // metaint excludes the actual metadata
      }
      read2 = _fread((char*)buffer+read,1,toread,m->filept);
      read += read2;
      m->data_until_meta -= read2;
   }
   else
      read = _fread(buffer,size,count,m->filept);

   if(m->save_file != NULL)
   {
      if(fwrite(buffer,1,read,m->save_file) < read)
      {
         char temp[1024];
         fclose(m->save_file);
         m->save_file = NULL;
         sprintf(temp,"Error writing to %s to save data.",m->save_filename);
         (*m->info_display)(temp);
         m->save_filename[0] = 0;
      }
   }

   return read;
}


static unsigned long oldhead = 0;
static unsigned long firsthead=0;

void read_frame_init (void)
{
   oldhead = 0;
   firsthead = 0;
}

/*
 * HACK,HACK,HACK...
 * step back <bytes> bytes
 */
int back_pos(DECODER_STRUCT *w, struct frame *fr,int bytes)
{
   unsigned char buf[4];
   unsigned long newhead;
   int donebytes = 0;

   if(w->sockmode) return 0;  /* this only works on files */

   if(_fseek(w->filept,-(bytes+2*(fsize+5)),SEEK_CUR) < 0)
   {
      donebytes = ftell(w->filept);
      _rewind(w->filept);
   }
   else
      donebytes = bytes+2*(fsize+5);

   if(_fread(buf,1,4,w->filept) != 4)
      return donebytes;

   donebytes -= 4;

   newhead = ((unsigned long) buf[0] << 24) |
             ((unsigned long) buf[1] << 16) |
             ((unsigned long) buf[2] << 8)  |
              (unsigned long) buf[3];

   while( (newhead & HDRCMPMASK) != (firsthead & HDRCMPMASK) ) {
      if(_fread(buf,1,1,w->filept) != 1)
         return donebytes;
      donebytes--;
      newhead <<= 8;
      newhead |= buf[0];
      newhead &= 0xffffffff;
   }

   if( _fseek(w->filept,-4,SEEK_CUR) < 0)
      return donebytes;

   donebytes += 4;

   read_frame(w,fr);
   read_frame(w,fr);
   donebytes -= 2*(fsize+4);

   if(fr->lay == 3)
      set_pointer(512);

   return donebytes;
}

/*
 * HACK,HACK,HACK...
 * step forward <bytes> bytes
*/
int forward_pos(DECODER_STRUCT *w, struct frame *fr,int bytes)
{
   unsigned char buf[4];
   unsigned long newhead;
   int donebytes = 0;

   if(w->sockmode) return 0;  /* this only works on files */

   if(_fseek(w->filept,bytes-2*(fsize+3),SEEK_CUR) < 0)
      return donebytes;
   else
      donebytes = bytes-2*(fsize+3);

   if(_fread(buf,1,4,w->filept) != 4)
      return donebytes;

   donebytes += 4;

   newhead = ((unsigned long) buf[0] << 24) |
             ((unsigned long) buf[1] << 16) |
             ((unsigned long) buf[2] << 8)  |
              (unsigned long) buf[3];

   while( (newhead & HDRCMPMASK) != (firsthead & HDRCMPMASK) ) {
      if(_fread(buf,1,1,w->filept) != 1)
         return donebytes;
      donebytes++;
      newhead <<= 8;
      newhead |= buf[0];
      newhead &= 0xffffffff;
   }

   if(_fseek(w->filept,-4,SEEK_CUR) < 0)
      return donebytes;

   donebytes -= 4;

   read_frame(w,fr);
   read_frame(w,fr);
   donebytes += 2*(fsize+4);

   if(fr->lay == 3)
      set_pointer(512);

   return donebytes;
}

int seekto_pos(DECODER_STRUCT *w, struct frame *fr, int bytes)
{
   unsigned char buf[4];
   unsigned long newhead;
   int donebytes = 0;
   int seektobytes = bytes-2*(fsize+3);

   if(w->sockmode) return 0;  /* this only works on files */

   if(seektobytes < 0) seektobytes = 0;

   if(_fseek(w->filept,seektobytes,SEEK_SET) < 0)
      return donebytes;
   else
      donebytes = seektobytes;

   if(_fread(buf,1,4,w->filept) != 4)
      return donebytes;

   donebytes += 4;

   newhead = ((unsigned long) buf[0] << 24) |
             ((unsigned long) buf[1] << 16) |
             ((unsigned long) buf[2] << 8)  |
              (unsigned long) buf[3];

   while( (newhead & HDRCMPMASK) != (firsthead & HDRCMPMASK) ) {
      if(_fread(buf,1,1,w->filept) != 1)
         return donebytes;
      donebytes++;
      newhead <<= 8;
      newhead |= buf[0];
      newhead &= 0xffffffff;
   }

   if(_fseek(w->filept,-4,SEEK_CUR) < 0)
      return donebytes;

   donebytes -= 4;

   read_frame(w,fr);
   read_frame(w,fr);
   donebytes += 2*(fsize+4);

   if(fr->lay == 3)
      set_pointer(512);

   return donebytes;
}

int head_read(META_STRUCT *m, unsigned char *hbuf,unsigned long *newhead)
{
   if(readdata(hbuf,1,4,m) != 4)
      return FALSE;

   *newhead = ((unsigned long) hbuf[0] << 24) |
              ((unsigned long) hbuf[1] << 16) |
              ((unsigned long) hbuf[2] << 8)  |
               (unsigned long) hbuf[3];

   return TRUE;
}


int head_check(unsigned long newhead)
{
   if( (newhead & 0xffe00000) != 0xffe00000)
      return FALSE;
   if(!((newhead>>17)&3))
      return FALSE;
   if( ((newhead>>12)&0xf) == 0xf)
      return FALSE;
   if( ((newhead>>10)&0x3) == 0x3 )
      return FALSE;

   return TRUE;
}

int decode_header(int newhead, int oldhead, struct frame *fr, int *ssize, void (* _System error_display)(char *) )
{
   int framesize;

    if( newhead & (1<<20) ) {
      fr->lsf = (newhead & (1<<19)) ? 0x0 : 0x1;
      fr->mpeg25 = 0;
    }
    else {
      fr->lsf = 1;
      fr->mpeg25 = 1;
    }

    if (!tryresync || !oldhead) {
          /* If "tryresync" is true, assume that certain
             parameters do not change within the stream! */
      fr->lay = 4-((newhead>>17)&3);
      fr->bitrate_index = ((newhead>>12)&0xf);
      if( ((newhead>>10)&0x3) == 0x3) {
          error_display("Stream error");
          return 0;
//        fprintf(stderr,"Stream error\n");
//        exit(1);
      }
      if(fr->mpeg25) {
        fr->sampling_frequency = 6 + ((newhead>>10)&0x3);
      }
      else
        fr->sampling_frequency = ((newhead>>10)&0x3) + (fr->lsf*3);
      fr->error_protection = ((newhead>>16)&0x1)^0x1;
    }

    fr->bitrate_index = ((newhead>>12)&0xf);
    fr->padding   = ((newhead>>9)&0x1);
    fr->extension = ((newhead>>8)&0x1);
    fr->mode      = ((newhead>>6)&0x3);
    fr->mode_ext  = ((newhead>>4)&0x3);
    fr->copyright = ((newhead>>3)&0x1);
    fr->original  = ((newhead>>2)&0x1);
    fr->emphasis  = newhead & 0x3;

    fr->stereo    = (fr->mode == MPG_MD_MONO) ? 1 : 2;

    if(!fr->bitrate_index)
    {
//      fprintf(stderr,"Free format not supported.\n");
        error_display("Free format not supported.");

       return (0);
    }

    if(fr->sampling_frequency >= sizeof(freqs)/sizeof(freqs[0]))
       fr->sampling_frequency = sizeof(freqs)/sizeof(freqs[0])-1;

    switch(fr->lay)
    {
      case 1:
        fr->do_layer = do_layer1;
        fr->jsbound = (fr->mode == MPG_MD_JOINT_STEREO) ?
                         (fr->mode_ext<<2)+4 : 32;
        framesize  = (long) tabsel_123[fr->lsf][0][fr->bitrate_index] * 12000;
        framesize /= freqs[fr->sampling_frequency];
        framesize  = ((framesize+fr->padding)<<2)-4;
        break;
      case 2:
        fr->do_layer = do_layer2;
        get_II_stuff(fr);
        fr->jsbound = (fr->mode == MPG_MD_JOINT_STEREO) ?
                         (fr->mode_ext<<2)+4 : fr->II_sblimit;
        framesize = (long) tabsel_123[fr->lsf][1][fr->bitrate_index] * 144000;
        framesize /= freqs[fr->sampling_frequency];
        framesize += fr->padding - 4;
        break;
      case 3:
        fr->do_layer = do_layer3;
        if(fr->lsf)
          *ssize = (fr->stereo == 1) ? 9 : 17;
        else
          *ssize = (fr->stereo == 1) ? 17 : 32;
        if(fr->error_protection)
          *ssize += 2;
          framesize  = (long) tabsel_123[fr->lsf][2][fr->bitrate_index] * 144000;
          framesize /= freqs[fr->sampling_frequency]<<(fr->lsf);
          framesize = framesize + fr->padding - 4;
        break;
      default:
//        fprintf(stderr,"Sorry, unknown layer type.\n");
//        return (0);
        framesize = 0;
    }

   return framesize;
}



int read_frame(DECODER_STRUCT *w, struct frame *fr)
{
  unsigned long newhead = 0;
  unsigned char ssave[40];
  unsigned char hbuf[8];
  static int framesize;
  int l;
  int try = 0;
  int rc = 1;

#if 0
  static int halfphase = 0;

  if (halfspeed)
    if (halfphase--) {
      bitindex = 0;
      wordpointer = (unsigned char *) bsbuf;
      if (fr->lay == 3)
        memcpy (bsbuf, ssave, ssize);
      return 1;
    }
    else
      halfphase = halfspeed - 1;
#endif

read_again:
   rc = 1;
   fr->filepos = _ftell(w->filept);

   if(!head_read(&w->metastruct, hbuf,&newhead))
      return FALSE;

  if(oldhead != newhead || !oldhead)
  {
    fr->header_change = 1;

init_resync:

   if(!firsthead && !head_check(newhead) ) {
      int i;

//      fprintf(stderr,"Junk at the beginning\n");

         /* step in byte steps through next 128K */
         for(i=0;i<128*1024;i++) {
            memmove (&hbuf[0], &hbuf[1], 3);
            if(readdata(hbuf+3,1,1,&w->metastruct) != 1)
               return 0;
            newhead <<= 8;
            newhead |= hbuf[3];
            newhead &= 0xffffffff;
            if(head_check(newhead))
               break;
         }
         if(i == 128*1024) {
//            fprintf(stderr,"Giving up searching valid MPEG header\n");
            return 0;
         }
      /*
       * should we check, whether a new frame starts at the next
       * expected position? (some kind of read ahead)
       * We could implement this easily, at least for files.
       */


   }

    if( (newhead & 0xffe00000) != 0xffe00000) {
//      if (!quiet)
//        fprintf(stderr,"Illegal Audio-MPEG-Header 0x%08lx at offset 0x%lx.\n",
//              newhead,_ftell(w->filept)-4);
      {
        char errorbuf[1024];
        sprintf(errorbuf,"Illegal Audio-MPEG-Header 0x%08lx at offset 0x%lx.",
              newhead,_ftell(w->filept)-4);
        w->error_display(errorbuf);
      }

      if (tryresync) {
            /* Read more bytes until we find something that looks
               reasonably like a valid header.  This is not a
               perfect strategy, but it should get us back on the
               track within a short time (and hopefully without
               too much distortion in the audio output).  */
        do {
          try++;
          if (readdata(&hbuf[3],1,1,&w->metastruct) != 1)
            return 0;

          /* This is faster than combining newhead from scratch */
          newhead = ((newhead << 8) | hbuf[3]) & 0xffffffff;

          if (!oldhead)
            goto init_resync;       /* "considered harmful", eh? */

        } while ((newhead & HDRCMPMASK) != (oldhead & HDRCMPMASK)
              && (newhead & HDRCMPMASK) != (firsthead & HDRCMPMASK));
//        if (!quiet)
//          fprintf (stderr, "Skipped %d bytes in input.\n", try);
        {
          char errorbuf[1024];
          sprintf (errorbuf, "Skipped %d bytes in input.", try);
          w->error_display(errorbuf);
        }
      }
      else
        return (0);
    }

   framesize = decode_header(newhead, oldhead, fr, &ssize, w->error_display);
  }
  else
    fr->header_change = 0;

  if(!firsthead)
  {
     unsigned long temphead;
     struct frame tempfr;
     int i, crapssize;

     if(_ftell(w->filept) > 256*1024) return 0; // we can't find a first header

     if(framesize <= 0)
        goto read_again;

     if(w->sockmode)
     {
        int found_badheader = 0;

        for(i = 0; i < 2 && !found_badheader; i++)
        {
           char *crap = (char *) alloca(framesize);
           if(readdata(crap,1,framesize,&w->metastruct) == framesize &&
              head_read(&w->metastruct, hbuf, &temphead))
           {
              if((temphead & HDRCMPMASK) != (newhead & HDRCMPMASK))
                 found_badheader = 1;
              else
                 framesize = decode_header(temphead, 0, &tempfr, &crapssize, w->error_display);
           }
           else
              return 0;
        }

        if(found_badheader)
           goto read_again;
     }
     else
     {
        int found_badheader = 0;
        int seekback = 0;
        int size = framesize;

        for(i = 0; i < 8 && !found_badheader; i++)
        {
           if(_fseek(w->filept,size,SEEK_CUR) == 0 &&
              head_read(&w->metastruct, hbuf,&temphead) )
           {
              seekback -= (size+4);
              if((temphead & HDRCMPMASK) != (newhead & HDRCMPMASK))
                 found_badheader = 1;
              else
                 size = decode_header(temphead, 0, &tempfr, &crapssize, w->error_display);
           }
           else
              return 0;
        }

        _fseek(w->filept,seekback,SEEK_CUR);

        if(found_badheader)
           goto read_again;
     }

     firsthead = newhead; // youhou!
     w->old_bitrate = tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index]; // for VBR event

     /* let's try to find a xing VBR header */
     if(fr->lay == 3)
     {
        char *buf = alloca(framesize+4);
        buf[0] = (firsthead >> 24) & 0xFF;
        buf[1] = (firsthead >> 16) & 0xFF;
        buf[2] = (firsthead >> 8)  & 0xFF;
        buf[3] = (firsthead)       & 0xFF;

        if(w->sockmode)
           readdata(buf+4,1,framesize,&w->metastruct);
        else
           _fread(buf+4,1,framesize,w->filept);

        w->Xingheader.toc = w->XingTOC;
        w->XingVBRmode = GetXingHeader(&w->Xingheader,buf);

        /* if we can't seek back, let's just read the next frame */
        if(_fseek(w->filept,-framesize,SEEK_CUR) != 0)
           goto read_again;
     }
  }

  if(rc == 0)
     return rc;

  if(framesize <= 0)
     goto read_again;

  oldhead = newhead;

  fsizeold=fsize; /* for Layer3 */
  bsbufold = bsbuf;
  bsbuf = bsspace[bsnum]+512;
  bsnum = (bsnum + 1) & 1;

  fsize = min(framesize,MAXFRAMESIZE);

  if( (l=readdata(bsbuf,1,fsize,&w->metastruct)) != fsize)
  {
    if(l <= 0)
      return 0;
    memset(bsbuf+l,0,fsize-l);
  }

  if (halfspeed && fr->lay == 3)
    memcpy (ssave, bsbuf, ssize);

  bitindex = 0;
  wordpointer = (unsigned char *) bsbuf;

  if(tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index] != w->old_bitrate)
  {
     WinPostMsg(w->hwnd,WM_CHANGEBR,MPFROMLONG(tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index]),0);
     w->old_bitrate = tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index];
  }

  return 1;
}


/*
 *   Split "path" into directory and filename components.
 *
 *   Return value is 0 if no directory was specified (i.e.
 *   "path" does not contain a '/'), OR if the directory
 *   is the same as on the previous call to this function.
 *
 *   Return value is 1 if a directory was specified AND it
 *   is different from the previous one (if any).
 */

int split_dir_file (const char *path, char **dname, char **fname)
{
   static char *lastdir = NULL;
   char *slashpos;

   if ((slashpos = strrchr(path, '/')) != NULL) {
      *fname = slashpos + 1;
      *dname = strdup(path); /* , 1 + slashpos - path); */
      if(!(*dname)) {
         perror("memory");
         exit(1);
      }
      (*dname)[1 + slashpos - path] = 0;
      if (lastdir && !strcmp(lastdir, *dname)) {
         /***   same as previous directory   ***/
         free (*dname);
         *dname = lastdir;
         return 0;
      }
      else {
         /***   different directory   ***/
         if (lastdir)
            free (lastdir);
         lastdir = *dname;
         return 1;
      }
   }
   else {
      /***   no directory specified   ***/
      if (lastdir) {
         free (lastdir);
         lastdir = NULL;
      };
      *dname = NULL;
      *fname = (char *)path;
      return 0;
   }
}

/* open the device to read the bit stream from it */

int open_stream(DECODER_STRUCT *w, char *bs_filenam,int fd, int buffersize, int bufferwait)
{
    w->sockmode = 0;
    w->filept_opened = 1;
    w->metastruct.data_until_meta = -1; // unknown
    w->metastruct.metadata_buffer[0] = 0;

    if (!bs_filenam) {
      if(fd < 0) {
         w->filept = w->metastruct.filept = stdin;
         w->filept_opened = 0;
      }
      else
         w->filept = fdopen(fd,"r");
    }
    else if (!strncmp(bs_filenam, "http://", 7))
       w->sockmode = HTTP;

    if (!(w->metastruct.filept = w->filept = _fopen(bs_filenam, "rb", w->sockmode, buffersize, bufferwait))) {
      char errorbuf[1024];
      if(w->sockmode)
         strcpy(errorbuf,http_strerror());
      else
      {
//         sprintf(errorbuf,"%s: %s",bs_filenam, strerror(errno));
         sprintf(errorbuf,"%s: %s",bs_filenam, strerror(sockfile_errno(w->sockmode)));
      }
      w->error_display(errorbuf);

//           perror (bs_filenam);
//           perror (temp);
       return FALSE;
    }

    return TRUE;
}

/*close the device containing the bit stream after a read process*/

void close_stream(DECODER_STRUCT *w)
{
   if (w->filept_opened)
   {
      _fclose(w->filept);
      w->filept_opened = FALSE;
   }
}

long tell_stream(DECODER_STRUCT *w)
{
   return _ftell(w->filept);
}

#if !defined(I386_ASSEM) || defined(DEBUG_GETBITS)
unsigned int getbits(int number_of_bits)
{
  unsigned long rval;

#ifdef DEBUG_GETBITS
fprintf(stderr,"g%d",number_of_bits);
#endif

  if(!number_of_bits)
    return 0;

  {
    rval = wordpointer[0];
    rval <<= 8;
    rval |= wordpointer[1];
    rval <<= 8;
    rval |= wordpointer[2];
#if 0
    rval = ((unsigned int) wordpointer[0] << 16)+((unsigned int) wordpointer[1]<<8)+
                 (unsigned int) wordpointer[2];
#endif
    rval <<= bitindex;
    rval &= 0xffffff;

    bitindex += number_of_bits;

    rval >>= (24-number_of_bits);

    wordpointer += (bitindex>>3);
    bitindex &= 7;
  }

#ifdef DEBUG_GETBITS
fprintf(stderr,":%x ",rval);
#endif
  return rval;
}

unsigned int getbits_fast(int number_of_bits)
{
  unsigned long rval;

#ifdef DEBUG_GETBITS
fprintf(stderr,"g%d",number_of_bits);
#endif

  {
    rval = wordpointer[0];
    rval <<= 8;
    rval |= wordpointer[1];
    rval <<= bitindex;
    rval &= 0xffff;
#if 0
    rval = ((unsigned int) high << (8-bitindex) )+((unsigned int) (unsigned char) wordpointer[1]);
#endif
    bitindex += number_of_bits;

    rval >>= (16-number_of_bits);

    wordpointer += (bitindex>>3);
    bitindex &= 7;
  }


#ifdef DEBUG_GETBITS
fprintf(stderr,":%x ",rval);
#endif


  return rval;
}

unsigned int get1bit(void)
{
  unsigned char rval;

#ifdef DEBUG_GETBITS
fprintf(stderr,"g%d",1);
#endif

  rval = *wordpointer << bitindex;

  bitindex++;
  wordpointer += (bitindex>>3);
  bitindex &= 7;

#ifdef DEBUG_GETBITS
fprintf(stderr,":%d ",rval>>7);
#endif

  return rval>>7;
}
#endif

void set_pointer(long backstep)
{
  wordpointer = bsbuf + ssize - backstep;
  if (backstep)
    memcpy(wordpointer,bsbufold+fsizeold-backstep,backstep);
  bitindex = 0;
}

/* samuel */
#include <sys/types.h>
#include <sys/stat.h>
int size_stream(DECODER_STRUCT *w)
{
   struct stat fi;
   if(w->filept_opened)
   {
      SOCKFILE *sockfile = (SOCKFILE *) w->filept;
      if(w->sockmode)
      {
         HTTP_INFO http_info;
         sockfile_httpinfo(w->filept,&http_info);
         return http_info.length;
      }
      else
      {
         fstat(fileno(sockfile->stream), &fi);
         return fi.st_size;
      }
   }
   return -1;
}

int bufferstatus_stream(DECODER_STRUCT *w)
{
   if(w->filept_opened)
      return sockfile_bufferstatus(w->filept);
   else
      return -1;
}

int nobuffermode_stream(DECODER_STRUCT *w, int nobuffermode)
{
   if(w->filept_opened && !w->sockmode)
      return sockfile_nobuffermode(w->filept, nobuffermode);
   else
      return FALSE;
}
