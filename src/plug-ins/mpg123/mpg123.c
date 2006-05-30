/*
 * Mpeg Layer audio decoder (see version.h for version number)
 * ------------------------
 * copyright (c) 1995,1996,1997 by Michael Hipp, All rights reserved.
 * See also 'README' !
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#if !defined(WIN32) && !defined(__IBMC__) && !defined(__IBMCPP__)
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
#endif

/* #define SET_PRIO */

#include "mpg123.h"

#include "version.h"

struct flags flags = { 0 , 0 };
int supported_rates = 0;

char *listname = NULL;
long outscale  = 32768;
int checkrange = FALSE;
int tryresync  = TRUE;
//int quiet      = FALSE;
int quiet      = TRUE;
int verbose    = 0;
int doublespeed= 0;
int halfspeed  = 0;
int change_always = 1;
int force_8bit = 0;
int force_frequency = -1;
int force_mono = 0;
long numframes = -1;
long startFrame= 0;
int usebuffer  = 0;
int frontend_type = 0;
int remote     = 0;
int buffer_fd[2];
int buffer_pid;
int down_sample;
int use_mmx;
int using_mmx;

#ifdef SHUFFLESUPPORT
char **shufflist= NULL;
int *shuffleord= NULL;
int shuffle    = FALSE;
#endif

int intflag = FALSE;

#include <utilfct.h>

HMODULE thisModule = 0;
char *thisModulePath = NULL;

BOOL mmx_present = FALSE;

int _CRT_init(void);
void _CRT_term(void);

BOOL _System _DLL_InitTerm(ULONG hModule, ULONG flag)
{
   if(flag == 0)
   {
      mmx_present = detect_mmx();
      if(_CRT_init() == -1)
         return FALSE;

      // getModule(&thisModule,&thisModulePath);
   }
   else
      _CRT_term();

   return TRUE;
}

static struct frame fr;
#define FRAMEBUFUNIT (18 * 64 * 4)

void set_synth (char *arg)
{
    if (*arg == '2') {
        fr.down_sample = 1;
    }
    else {
        fr.down_sample = 2;
    }
}

/*
 * play a frame read read_frame();
 * (re)initialize audio if necessary.
 */
int play_frame(DECODER_STRUCT *w,struct frame *fr)
{
   int clip;

   if (fr->error_protection) {
      getbits(16); /* crc */
   }

   clip = (fr->do_layer)(w,fr);

/*   if(clip > 0 && checkrange)
      fprintf(stderr,"%d samples clipped\n", clip); */
   if(clip == -1)
      return -1;

   return 0;
}

void set_synth_functions(struct frame *fr)
{
   typedef int (*func)(real *,int,unsigned char *);
   typedef int (*func_mono)(real *,unsigned char *);

   static func funcs[2][3] = {
      { synth_1to1, synth_2to1, synth_4to1 } ,
      { synth_1to1_8bit, synth_2to1_8bit, synth_4to1_8bit }
   };

    static func_mono funcs_mono[2][2][3] = {
        { { synth_1to1_mono2stereo ,
            synth_2to1_mono2stereo ,
            synth_4to1_mono2stereo } ,
          { synth_1to1_8bit_mono2stereo ,
            synth_2to1_8bit_mono2stereo ,
            synth_4to1_8bit_mono2stereo } } ,
        { { synth_1to1_mono ,
            synth_2to1_mono ,
            synth_4to1_mono } ,
          { synth_1to1_8bit_mono ,
            synth_2to1_8bit_mono ,
            synth_4to1_8bit_mono } }
    };

   fr->synth = funcs[force_8bit][fr->down_sample];
   fr->synth_mono = funcs_mono[force_mono][force_8bit][fr->down_sample];
   fr->block_size = 128 >> (force_mono+force_8bit+fr->down_sample);
}

/* starting the main DLL code */

void save_ini(void);
void load_ini(void);


extern int tabsel_123[2][3][16];

void clear_decoder(void)
{
   /* clear synth */
   real crap[SBLIMIT];
   char crap2[4*SBLIMIT];
   int i;

   for(i = 0; i < 16; i++)
   {
      memset(crap,0,sizeof(crap));
      synth_1to1(crap,0,crap2);
      memset(crap,0,sizeof(crap));
      synth_1to1(crap,1,crap2);
   }

   /* clear layer3 */
   clear_layer3();
}

static void _Optlink DecoderThread(void *arg)
{
   ULONG resetcount;
   int init;
   DECODER_STRUCT *w = (DECODER_STRUCT *) arg;

   for(;;)
   {
      DosWaitEventSem(w->play, -1);
      DosResetEventSem(w->play,&resetcount);

      if( w->end == TRUE ) {
        return;
      }

      w->status = DECODER_STARTING;
      w->last_length = -1;

      DosResetEventSem(w->playsem,&resetcount);
      DosPostEventSem(w->ok);

      if(!open_stream(w, w->filename,-1, w->buffersize, w->bufferwait))
      {
         WinPostMsg(w->hwnd,WM_PLAYERROR,0,0);
         w->status = DECODER_STOPPED;
         DosPostEventSem(w->playsem);
         continue;
      }

      w->jumptopos = -1;
      w->rew = w->ffwd = 0;

      read_frame_init();

      init = 1;
      for(w->stop = FALSE;!w->stop && read_frame(w,&fr);  ) {

      /* hey taneli, need that?
         if(fr.frameNum < startFrame || (doublespeed && (fr.frameNum % doublespeed))) {
            if(fr.lay == 3)
               set_pointer(512);
            continue;
         }  */

         if(init)
         {
            clear_decoder();
            w->output_format.samplerate = freqs[fr.sampling_frequency]>>(fr.down_sample);
            init_eq(w->output_format.samplerate);

            free(pcm_sample);
            pcm_sample = (unsigned char *) malloc(w->audio_buffersize);
            pcm_point = 0;

            init = 0;
            w->status = DECODER_PLAYING;
            w->last_length = decoder_length(w);
         }

         if(play_frame(w,&fr) < 0)
         {
            WinPostMsg(w->hwnd,WM_PLAYERROR,0,0);
            break;
         }

         /* samuel */
         if(w->jumptopos >= 0)
         {
            int jumptobyte;
            if(w->XingVBRmode && (w->Xingheader.flags & FRAMES_FLAG) && (w->Xingheader.flags & BYTES_FLAG ))
               jumptobyte = SeekPoint(w->XingTOC, w->Xingheader.bytes, (float) w->jumptopos * 100 / ( 1000.0 *
                            8 * 144 * w->Xingheader.frames / freqs[fr.sampling_frequency]) );
            else
               jumptobyte = (float) w->jumptopos * (tabsel_123[fr.lsf][fr.lay-1][fr.bitrate_index] * 125) / 1000;

            clear_decoder();

            seekto_pos(w, &fr, jumptobyte);

            w->jumptopos = -1;
            WinPostMsg(w->hwnd,WM_SEEKSTOP,0,0);
         }

         if(w->rew)
            /* ie.: ~1/4 second */
            back_pos(w,&fr,(tabsel_123[fr.lsf][fr.lay-1][fr.bitrate_index] * 125)/4);

         if(w->ffwd)
            forward_pos(w,&fr,(tabsel_123[fr.lsf][fr.lay-1][fr.bitrate_index] * 125)/4);
      }

      if(pcm_sample)
      {
         free(pcm_sample);
         pcm_sample = NULL;
      }

      w->status = DECODER_STOPPED;

      close_stream(w);

      if(w->metastruct.save_file != NULL)
      {
         fclose(w->metastruct.save_file);
         w->metastruct.save_file = NULL;
      }

      DosPostEventSem(w->playsem);
      WinPostMsg(w->hwnd,WM_PLAYSTOP,0,0);
      DosPostEventSem(w->ok);
   }
}


void reset_thread(DECODER_STRUCT *w)
{
   ULONG resetcount;
   w->end = FALSE;

   DosResetEventSem(w->play,&resetcount);
   DosResetEventSem(w->ok,&resetcount);
   DosKillThread(w->decodertid);
   w->decodertid = _beginthread(DecoderThread,0,64*1024,(void *) w);
}

int _System decoder_init(void **returnw)
{
   DECODER_STRUCT *w = calloc(sizeof(*w),1);
   *returnw = w;

   w->end = FALSE;

   DosCreateEventSem(NULL,&w->play,0,FALSE);
   DosCreateEventSem(NULL,&w->ok,0,FALSE);
   w->decodertid = _beginthread(DecoderThread,0,64*1024,(void *) w);

   return w->decodertid;
}

BOOL _System decoder_uninit( void* arg )
{
   DECODER_STRUCT* w = arg;
   int wait = 50;

   w->end = TRUE;
   w->stop = TRUE;
   sockfile_abort(w->filept);
   DosPostEventSem(w->play);

   while( wait-- && DosWaitThread( &w->decodertid, DCWW_NOWAIT ) == ERROR_THREAD_NOT_TERMINATED ) {
     DosSleep( 100 );
   }

   if( DosWaitThread( &w->decodertid, DCWW_NOWAIT ) == ERROR_THREAD_NOT_TERMINATED ) {
     DosKillThread( w->decodertid );
   }

   DosCloseEventSem(w->play);
   DosCloseEventSem(w->ok);

   return TRUE;
}

void setup_decoder(void);

ULONG _System decoder_command(void *arg, ULONG msg, DECODER_PARAMS *info)
{
   DECODER_STRUCT *w = arg;
   ULONG resetcount;

   switch(msg)
   {
      case DECODER_SETUP:

         setup_decoder();

         fr.synth = synth_1to1;

         w->output_format.format = WAVE_FORMAT_PCM;
         w->output_format.bits = 16;

         make_decode_tables(outscale);
         init_layer2(fr.down_sample); /* inits also shared tables with layer1 */
         init_layer3(fr.down_sample);

         if(force_mono) {
            fr.single = 3; /* left and right single mix */
            w->output_format.channels = 1;
         }
         else {
            fr.single = -1; /* both channels */
            w->output_format.channels = 2;
         }


         set_synth_functions(&fr);

         httpauth = info->httpauth;
         proxyurl = info->proxyurl;

         w->buffersize = info->buffersize;
         w->bufferwait = info->bufferwait;

         w->playsem = info->playsem;
         DosPostEventSem(w->playsem);
         w->hwnd = info->hwnd;
         w->metastruct.hwnd = info->hwnd;

         w->error_display = info->error_display;
         w->metastruct.info_display = info->info_display;
         w->output_play_samples = info->output_play_samples;
         w->a = info->a;

         w->audio_buffersize = info->audio_buffersize;

         w->metastruct.metadata_buffer = info->metadata_buffer;
         w->metastruct.metadata_buffer[0] = 0;
         w->metastruct.metadata_size = info->metadata_size;

         memset(w->metastruct.save_filename,0,sizeof(w->metastruct.save_filename));

         break;

      case DECODER_PLAY:

         if(w->status == DECODER_STOPPED)
         {
            strcpy(w->filename, info->filename);
            DosResetEventSem(w->ok,&resetcount);
            DosPostEventSem(w->play);
            if(DosWaitEventSem(w->ok, 10000) == 640)
            {
               w->status = DECODER_STOPPED;
               reset_thread(w);
            }
         }
         break;


      case DECODER_STOP:
         if(w->status != DECODER_STOPPED)
         {
            DosResetEventSem(w->ok,&resetcount);
            w->stop = TRUE;
            sockfile_abort(w->filept);
            if(DosWaitEventSem(w->ok, 10000) == 640)
            {
               w->status = DECODER_STOPPED;
               reset_thread(w);
            }
         }
         break;

      case DECODER_FFWD:
         if(info->ffwd)
         {
            nobuffermode_stream(w,TRUE);
            w->ffwd = TRUE;
         }
         else
         {
            nobuffermode_stream(w,FALSE);
            w->ffwd = FALSE;
         }
         break;

      case DECODER_REW:
         if(info->rew)
         {
            nobuffermode_stream(w,TRUE);
            w->rew = TRUE;
         }
         else
         {
            nobuffermode_stream(w,FALSE);
            w->rew = FALSE;
         }
         break;

      case DECODER_JUMPTO:
         w->jumptopos = info->jumpto;
         break;

      case DECODER_EQ:
         if((flags.mp3_eq = flags.equalizer = info->equalizer) == 1)
            memcpy(&octave_eq,info->bandgain,sizeof(octave_eq));

         if(w->status == DECODER_PLAYING)
            init_eq(w->output_format.samplerate);

         break;

      case DECODER_BUFFER:
         info->bufferstatus = bufferstatus_stream(w);
         break;

      case DECODER_SAVEDATA:
         if(info->save_filename == NULL)
            w->metastruct.save_filename[0] = 0;
         else
            strncpy(w->metastruct.save_filename, info->save_filename, sizeof(w->metastruct.save_filename)-1);
         break;

      default:
         (*w->error_display)("Unknown command."); return 1;
   }

   return 0;
}

ULONG _System decoder_status(void *arg)
{
   DECODER_STRUCT *w = arg;

   return w->status;
}

ULONG _System decoder_length(void *arg)
{
   DECODER_STRUCT *w = arg;

   _control87(EM_OVERFLOW | EM_UNDERFLOW | EM_ZERODIVIDE |
              EM_INEXACT | EM_INVALID | EM_DENORMAL, MCW_EM);

   if(w->status == DECODER_PLAYING)
   {
      if(w->XingVBRmode)
         w->last_length = (float)8 * 144 * w->Xingheader.frames * 1000 / freqs[fr.sampling_frequency];
      else
      {
         int bytepers = (tabsel_123[fr.lsf][fr.lay-1][fr.bitrate_index] * 125);
         if(bytepers == 0) bytepers = 1;
         w->last_length = (float)size_stream(w)*1000/bytepers;
      }
   }

   return w->last_length;
}

void _System dummy_error_display(char *message)
{
   ;
}

ULONG _System decoder_fileinfo(char *filename, DECODER_INFO *info)
{
  unsigned long temphead=0;
  unsigned long temphead2;
  struct frame tempfr;
  unsigned char hbuf[8];
  int framesize;
  int rc = 0;
  struct stat fi = {0};
  int i, crapssize;

  int sockmode = 0;

  XHEADDATA tempXingheader;
  int tempXingVBRmode = 0;

  char metadata[512] = "";
  META_STRUCT mtemp = {0};
  mtemp.metadata_buffer = metadata;
  mtemp.metadata_size = sizeof(metadata);
  mtemp.data_until_meta = -1; // unknown

  memset(info,0,sizeof(*info));
  info->size = sizeof(*info);
  info->numchannels = 0;

  if (!strncmp(filename, "http://", 7))
     sockmode = HTTP;

  if (!(mtemp.filept = _fopen(filename, "rb", sockmode, 0, 0))) {
/*      if(!sockmode)
      {
         sprintf(errorbuf,"%s: %s",filename, strerror(errno));
         queue_error(errorbuf);
      } */

      rc = sockfile_errno(sockmode);

//         perror (filename);
//         perror (temp);
         goto end;
  }

read_again:
   rc = 0;

   if(!head_read(&mtemp, hbuf,&temphead))
   {
      rc = 100;
      goto end;
   }

   if(!head_check(temphead) ) {
      int i;

//      fprintf(stderr,"Junk at the beginning\n");
      /* I even saw RIFF headers at the beginning of MPEG streams ;( */

         /* step in byte steps through next 128K */
         for(i=0;i<128*1024;i++) {
            if(readdata(hbuf+3,1,1,&mtemp) != 1)
            {
               rc = 100;
               goto end;
            }
            temphead <<= 8;
            temphead |= hbuf[3];
            temphead &= 0xffffffff;
            if(head_check(temphead))
               break;
         }
         if(i == 128*1024) {
//            fprintf(stderr,"Giving up searching valid MPEG header\n");
            rc = 200;
            goto end;
         }
   }

   framesize = decode_header(temphead, 0, &tempfr, &crapssize, dummy_error_display);

   if(_ftell(mtemp.filept) > 256*1024)
   {
      rc = 200;
      goto end;
   }

   if(framesize <= 0)
      goto read_again;

   if(sockmode)
   {
      int found_badheader = 0;

      for(i = 0; i < 2 && !found_badheader; i++)
      {
         char *crap = (char *) alloca(framesize);
         if(readdata(crap,1,framesize,&mtemp) == framesize &&
            head_read(&mtemp, hbuf, &temphead2))
         {
            if((temphead2 & HDRCMPMASK) != (temphead & HDRCMPMASK))
               found_badheader = 1;
            else
               framesize = decode_header(temphead2, 0, &tempfr, &crapssize, dummy_error_display);
         }
         else
         {
            rc = 100;
            goto end;
         }
      }

      if(found_badheader)
         goto read_again;
   }
   else
   {
      int found_badheader = 0;
      int seekback = 0;

      for(i = 0; i < 8 && !found_badheader; i++)
      {
         if(_fseek(mtemp.filept,framesize,SEEK_CUR) == 0 &&
            head_read(&mtemp, hbuf,&temphead2) )
         {
            seekback -= (framesize+4);
            if((temphead2 & HDRCMPMASK) != (temphead & HDRCMPMASK))
               found_badheader = 1;
            else
               framesize = decode_header(temphead2, 0, &tempfr, &crapssize, dummy_error_display);
         }
         else
         {
            rc = 100;
            goto end;
         }
      }

      _fseek(mtemp.filept,seekback,SEEK_CUR);

      if(found_badheader)
         goto read_again;
   }

   /* let's try to find a xing VBR header */
   if(tempfr.lay == 3)
   {
      char *buf = alloca(framesize+4);
      buf[0] = (temphead >> 24) & 0xFF;
      buf[1] = (temphead >> 16) & 0xFF;
      buf[2] = (temphead >> 8)  & 0xFF;
      buf[3] = (temphead)       & 0xFF;
      _fread(buf+4,1,framesize,mtemp.filept);
      tempXingheader.toc = NULL;
      if(GetXingHeader(&tempXingheader,buf))
         tempXingVBRmode = 1;
      else
         tempXingVBRmode = 0;
   }

   if(rc != 0)
      goto end;


   info->format.format = WAVE_FORMAT_PCM;
   info->format.bits = 16;

   info->mpeg = tempfr.mpeg25 ? 25 : (tempfr.lsf ? 20 : 10);
   info->layer = tempfr.lay;
   info->format.samplerate = freqs[tempfr.sampling_frequency];
   info->mode = tempfr.mode;
   info->modext = tempfr.mode_ext;
   info->bpf = framesize+4;
   info->format.channels = tempfr.stereo;
   info->bitrate = tabsel_123[tempfr.lsf][tempfr.lay-1][tempfr.bitrate_index];
   info->extention = tempfr.extension;
   info->junklength = _ftell(mtemp.filept)-4-framesize;

   if(tempXingVBRmode && tempXingheader.flags & FRAMES_FLAG)
   {
      info->songlength = (float)8 * 144 * tempXingheader.frames * 1000 / freqs[tempfr.sampling_frequency];
      if(tempXingheader.flags & BYTES_FLAG)
         info->bitrate = (float) tempXingheader.bytes / info->songlength * 1000 / 125;
   }
   else
   {
      SOCKFILE *sockfile = (SOCKFILE *) mtemp.filept;
      int length;

      if(sockmode)
      {
         HTTP_INFO http_info;
         sockfile_httpinfo(mtemp.filept,&http_info);
         length = http_info.length;
      }
      else
      {
         fstat(fileno(sockfile->stream), &fi);
         length = fi.st_size;
      }

      if(!info->bitrate)
         rc = 200;
      else
      {
         info->songlength = (float)length*1000/(info->bitrate * 125);
         rc = 0;
      }
   }

   sprintf(info->tech_info, "%u kbs, %.1f kHz, %s", info->bitrate, (info->format.samplerate/1000.0), modes(info->mode));

   if( sockmode )
   {
      HTTP_INFO http_info;
      char *title;

      sockfile_httpinfo(mtemp.filept,&http_info);

      strncpy(info->comment,http_info.icy_name,sizeof(info->comment)-1);
      strncpy(info->genre,http_info.icy_genre,sizeof(info->genre)-1);

      // title, I'd have to read at least metaint bytes to snatch this huh
      title = strstr(metadata,"StreamTitle='")+13;

      if(title != (char*)NULL+13)
      {
         char *sep = strchr(title,';');
         if(sep != NULL)
            *(sep-1) = 0; // the stupid '

         info->title[79] = 0;
         strncpy(info->title,title,sizeof(info->title)-1);
      }
   }
   else
   {
      *info->comment = '\0';
      *info->genre   = '\0';
   }

end:

   _fclose(mtemp.filept);

   return rc;
}


ULONG _System decoder_trackinfo(char *drive, int track, DECODER_INFO *info)
{
   return 200;
}

ULONG _System decoder_cdinfo(char *drive, DECODER_CDINFO *info)
{
   return 100;
}

ULONG _System decoder_support(char *ext[], int *size)
{
   if(size)
   {
      if(ext != NULL && *size >= 3)
      {
         strcpy(ext[0],"*.MP1");
         strcpy(ext[1],"*.MP2");
         strcpy(ext[2],"*.MP3");
      }
      *size = 3;
   }

   return DECODER_FILENAME | DECODER_URL;
}


void setup_decoder()
{
   fr.down_sample = down_sample;
   fr.using_mmx = use_mmx;
   using_mmx = use_mmx;
}


#include "mpg123def.h"

HWND dlghwnd = 0;

MRESULT EXPENTRY ConfigureDlgProc(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   switch(msg)
   {
      case WM_DESTROY:
         dlghwnd = 0;
         break;

      case WM_CLOSE:
         WinDestroyWindow(hwnd);
         break;

      case WM_INITDLG:
         WinSendDlgItemMsg(hwnd, CB_FORCEMONO, BM_SETCHECK, MPFROMSHORT(force_mono), 0);
         WinSendDlgItemMsg(hwnd, CB_DOWNSAMPLE, BM_SETCHECK, MPFROMSHORT(down_sample), 0);
         WinSendDlgItemMsg(hwnd, CB_USEMMX, BM_SETCHECK, MPFROMSHORT(use_mmx), 0);
         break;
      case WM_COMMAND:
         switch(SHORT1FROMMP(mp1))
         {
            case DID_OK:
               force_mono = (BOOL)WinSendDlgItemMsg(hwnd, CB_FORCEMONO, BM_QUERYCHECK, 0, 0);
               down_sample = (BOOL)WinSendDlgItemMsg(hwnd, CB_DOWNSAMPLE, BM_QUERYCHECK, 0, 0);
               use_mmx = (BOOL)WinSendDlgItemMsg(hwnd, CB_USEMMX, BM_QUERYCHECK, 0, 0);
               save_ini();
            case DID_CANCEL:
               WinDestroyWindow(hwnd);
               break;
         }
         break;
   }

   return WinDefDlgProc(hwnd, msg, mp1, mp2);
}

#define FONT1 "9.WarpSans"
#define FONT2 "8.Helv"

void _System plugin_configure(HWND hwnd, HMODULE module)
{
   if(dlghwnd == 0)
   {
      LONG fontcounter = 0;
      dlghwnd = WinLoadDlg(HWND_DESKTOP, HWND_DESKTOP, ConfigureDlgProc, module, 1, NULL);

      if(dlghwnd)
      {
         HPS hps;

         hps = WinGetPS(HWND_DESKTOP);
         if(GpiQueryFonts(hps, QF_PUBLIC,strchr(FONT1,'.')+1, &fontcounter, 0, NULL))
            WinSetPresParam(dlghwnd, PP_FONTNAMESIZE, strlen(FONT1)+1, FONT1);
         else
            WinSetPresParam(dlghwnd, PP_FONTNAMESIZE, strlen(FONT2)+1, FONT2);
         WinReleasePS(hps);

         WinSetFocus(HWND_DESKTOP,WinWindowFromID(dlghwnd,CB_FORCEMONO));
         if(!mmx_present)
            WinEnableWindow(WinWindowFromID(dlghwnd,CB_USEMMX),FALSE);
         WinShowWindow(dlghwnd, TRUE);
      }
   }
   else
      WinFocusChange(HWND_DESKTOP, dlghwnd, 0);
}

#define INIFILE "mpg123.ini"

void save_ini()
{
   HINI INIhandle;

   if((INIhandle = open_module_ini()) != NULLHANDLE)
   {
      save_ini_value(INIhandle,force_mono);
      save_ini_value(INIhandle,down_sample);
      save_ini_value(INIhandle,use_mmx);
      close_ini(INIhandle);
   }
}

void load_ini()
{
   HINI INIhandle;

   force_mono = 0;
   down_sample = 0;
   use_mmx = 0;

   if((INIhandle = open_module_ini()) != NULLHANDLE)
   {
      load_ini_value(INIhandle,force_mono);
      load_ini_value(INIhandle,down_sample);
      load_ini_value(INIhandle,use_mmx);
      close_ini(INIhandle);
   }
}

void _System plugin_query(PLUGIN_QUERYPARAM *param)
{
   param->type = PLUGIN_DECODER;
   param->author = "Samuel Audet";
   param->desc = "MP3 Decoder 1.21";
   param->configurable = TRUE;

   load_ini();
}
