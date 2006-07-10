/*
 * mpg123 defines
 * used source: musicout.h from mpegaudio package
 */

#define  INCL_PM
#define  INCL_DOS
#define  INCL_ERRORS
#include <os2.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#include <utilfct.h>
#include <httpget.h>
#include <sockfile.h>
#include <decoder_plug.h>
#include <plugin.h>

#if !defined(WIN32) && !defined(OS2)
#include <sys/signal.h>
#include <unistd.h>
#endif

#include "dxhead.h"

#if defined( OS2 )
  #include <float.h>
  #ifndef M_PI
    #define M_PI    3.14159265358979323846
  #endif
  #ifndef M_SQRT2
    #define M_SQRT2 1.41421356237309504880
  #endif
  #define REAL_IS_FLOAT
#endif

#if defined( HPUX )
  #define random  rand
  #define srandom srand
#endif

#if defined( _WIN32 )
  /* Win32 Additions By Tony Million */
  #undef  WIN32
  #define WIN32

  #define M_PI       3.14159265358979323846
  #define M_SQRT2    1.41421356237309504880
  #define REAL_IS_FLOAT
  #define NEW_DCT9

  #define random  rand
  #define srandom srand
#endif

#ifdef SUNOS
#define memmove(dst,src,size) bcopy(src,dst,size)
#endif

#if defined( REAL_IS_FLOAT )
  #define real float
#elif defined( REAL_IS_LONG_DOUBLE )
  #define real long double
#else
  #define real double
#endif

#if defined( __GNUC__ )
   #define INLINE inline
#elif defined( __IBMC__ ) || defined( __IBMCPP__ )
   #define INLINE _Inline
#else
   #define INLINE
#endif

#if !defined( NAS )
  #if defined( LINUX ) || defined( __FreeBSD__ ) || defined( __bsdi__ )
    #define VOXWARE
  #endif
#endif

#define WRITE_SAMPLE(samples,sum,clip) \
{                                      \
  int temp = sum;                      \
  if( temp > 32767 ) {                 \
    *samples = 0x7fff;                 \
     clip++;                           \
  } else if( temp < -32768 ) {         \
    *samples = -0x8000;                \
     clip++;                           \
  } else {                             \
    *samples = temp;                   \
  }                                    \
}

#define HDRCMPMASK 0xFFFF0D00UL

typedef struct
{
   FILE* filept;
   HWND  hwnd;

   int   data_until_meta;
   char* metadata_buffer;
   int   metadata_size;

   char  save_filename[512];
   FILE* save_file;

   void (PM123_ENTRYP info_display)(char*);

} META_STRUCT;

int    head_read ( META_STRUCT* m, unsigned long* newhead );
int    head_check( unsigned long newhead );
size_t readdata  ( void* buffer, size_t size, size_t count, META_STRUCT* m );

typedef struct
{
   FILE* filept;
   int   filept_opened;
   int   sockmode;

   XHEADDATA     XingHeader;
   unsigned char XingTOC[100];

   HEV   play;          // For internal use to sync the decoder thread.
   HEV   ok;            // For internal use to sync the decoder thread.
   ULONG decodertid;
   char  filename[1024];
   int   stop;
   int   rew;
   int   ffwd;
   int   jumptopos;
   ULONG status;
   HEV   playsem;       // Is for external use.
   HWND  hwnd;          // PM interface main frame.
   int   buffersize;
   int   bufferwait;
   int   last_length;
   BOOL  end;           // To get out of the infinite loop.
   int   old_bitrate;
   int   avg_bitrate;

   FORMAT_INFO output_format;
   META_STRUCT metastruct;

   void (PM123_ENTRYP error_display)(char*);
   int  (PM123_ENTRYP output_play_samples)(void* a, FORMAT_INFO* format, char* buf, int len, int posmarker );
   void* a;             // Only to be used with the precedent function.
   int   audio_buffersize;

} DECODER_STRUCT;

#define FALSE                  0
#define TRUE                   1

#define SBLIMIT               32
#define SCALE_BLOCK           12
#define SSLIMIT               18

#define MPG_MD_STEREO          0
#define MPG_MD_JOINT_STEREO    1
#define MPG_MD_DUAL_CHANNEL    2
#define MPG_MD_MONO            3

struct al_table
{
  short bits;
  short d;
};

struct frame {

    struct al_table *alloc;
    int (*synth)(real *,int,unsigned char *);
    int (*synth_mono)(real *,unsigned char *);
    int stereo;
    int jsbound;
    int single;
    int II_sblimit;
    int lsf;
    int mpeg25;
    int down_sample;
    int header_change;
    int block_size;
    int lay;
    int (*do_layer)(DECODER_STRUCT *w, struct frame *fr);
    int error_protection;
    int bitrate_index;
    int sampling_frequency;
    int padding;
    int extension;
    int mode;
    int mode_ext;
    int copyright;
    int original;
    int emphasis;
    int filepos;    /* bytes */
};

struct flags {

   int equalizer;
   int mp3_eq;
   int aggressive;  /* Renice to max. priority. */
};

extern int mmx_enable;
extern int mmx_use;
extern int pcm_point;

extern unsigned char* pcm_sample;
extern unsigned char* conv16to8;

extern long  freqs[9];
extern real  muls[27][64];
extern real  decwin[512+32];
extern real* pnts[5];
extern real  octave_eq[2][10];

extern struct flags flags;

struct gr_info_s {

  int scfsi;
  unsigned part2_3_length;
  unsigned big_values;
  unsigned scalefac_compress;
  unsigned block_type;
  unsigned mixed_block_flag;
  unsigned table_select[3];
  unsigned subblock_gain[3];
  unsigned maxband[3];
  unsigned maxbandl;
  unsigned maxb;
  unsigned region1start;
  unsigned region2start;
  unsigned preflag;
  unsigned scalefac_scale;
  unsigned count1table_select;
  real* full_gain[3];
  real* pow2gain;
};

struct III_sideinfo {

  unsigned main_data_begin;
  unsigned private_bits;

  struct {
    struct gr_info_s gr[2];
  } ch[2];
};

extern unsigned int get1bit( void );
extern unsigned int getbits( int );
extern unsigned int getbits_fast( int );

extern void set_pointer( long );
extern int  bufferstatus_stream( DECODER_STRUCT* w );
extern int  nobuffermode_stream( DECODER_STRUCT* w, int nobuffermode );
extern int  output_flush( DECODER_STRUCT* w, struct frame* fr );
extern int  open_stream( DECODER_STRUCT* w, char* bs_filenam, int fd, int buffersize, int bufferwait );
extern void close_stream( DECODER_STRUCT* w );
extern long tell_stream( DECODER_STRUCT* w );
extern void read_frame_init( void );
extern int  read_frame( DECODER_STRUCT* w, struct frame* fr );
extern int  back_pos( DECODER_STRUCT* w, struct frame* fr, int bytes );
extern int  forward_pos( DECODER_STRUCT* w, struct frame* fr,int bytes );
extern int  decode_header( int newhead, int oldhead, struct frame* fr, int* ssize, void (PM123_ENTRYP error_display)(char*));
extern int  seekto_pos( DECODER_STRUCT* w, struct frame* fr, int bytes );
extern int  do_layer3( DECODER_STRUCT* w, struct frame*fr );
extern void clear_layer3( void );
extern int  do_layer2( DECODER_STRUCT* w, struct frame* fr );
extern int  do_layer1( DECODER_STRUCT* w, struct frame* fr );
extern void do_equalizer( real* bandPtr, int channel );
extern void do_mp3eq( real* bandPtr, int channel );
extern void init_eq( int rate );
extern int  synth_1to1( real*, int, unsigned char* );
extern int  synth_1to1_8bit( real*, int, unsigned char* );
extern int  synth_2to1( real*, int, unsigned char* );
extern int  synth_2to1_8bit( real*, int, unsigned char* );
extern int  synth_4to1( real*, int, unsigned char* );
extern int  synth_4to1_8bit( real*, int, unsigned char* );
extern int  synth_1to1_mono( real*, unsigned char* );
extern int  synth_1to1_mono2stereo( real*, unsigned char* );
extern int  synth_1to1_8bit_mono( real*, unsigned char* );
extern int  synth_1to1_8bit_mono2stereo( real*, unsigned char* );
extern int  synth_2to1_mono( real*, unsigned char* );
extern int  synth_2to1_mono2stereo( real*, unsigned char* );
extern int  synth_2to1_8bit_mono( real*, unsigned char* );
extern int  synth_2to1_8bit_mono2stereo( real*, unsigned char* );
extern int  synth_4to1_mono( real*, unsigned char* );
extern int  synth_4to1_mono2stereo( real*, unsigned char* );
extern int  synth_4to1_8bit_mono( real*, unsigned char* );
extern int  synth_4to1_8bit_mono2stereo( real*, unsigned char* );
extern void init_layer3( int );
extern void init_layer2( int );
extern void make_conv16to8_table( int );

extern void _Optlink make_decode_tables( long scale );
extern void _Optlink dct64_MMX( short*, short*, real* );
extern BOOL _Optlink detect_mmx( void );
extern int  _Optlink synth_1to1_MMX( real*, int, unsigned char* );
extern void _Optlink dct64( real*, real*, real* );
