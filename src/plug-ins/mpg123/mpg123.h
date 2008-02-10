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
#include <float.h>

#include <utilfct.h>
#include <decoder_plug.h>
#include <xio.h>
#include <dxhead.h>

#if defined( OS2 )

  #ifndef M_PI
  #define M_PI    3.14159265358979323846
  #endif
  #ifndef M_SQRT2
  #define M_SQRT2 1.41421356237309504880
  #endif
  #define REAL_IS_FLOAT
#endif

#if defined( REAL_IS_FLOAT )
  #define real float
#elif defined( REAL_IS_LONG_DOUBLE )
  #define real long double
#else
  #define real double
#endif

#define WRITE_SAMPLE( samples, sum, clip ) \
{                                          \
  int temp = sum;                          \
  if( temp > 32767 ) {                     \
    *samples = 0x7fff;                     \
     clip++;                               \
  } else if( temp < -32768 ) {             \
    *samples = -0x8000;                    \
     clip++;                               \
  } else {                                 \
    *samples = temp;                       \
  }                                        \
}

// For Layer 1:
// MaxFrameLengthInBytes = (12000 * MaxBitRate / MinSampleRate + MaxPadding ) * 4
// For Layer 2 & 3 (MPEG 1.0):
// MaxFrameLengthInBytes = 144000 * MaxBitRate / MinSampleRate + MaxPadding
// For Layer 3 (MPEG 2.X):
// MaxFrameLengthInBytes = 144000 * MaxBitRate / MinSampleRate / 2 + MaxPadding
//
// Maybe mpeg 2.5 and layer 1/2 is an illegal combination, but I have not
// found any trustworthy information about it.
//
// mpeg 1.0, layer 1  448   32000 =  676
// mpeg 1.0, layer 2  384   32000 = 1729
// mpeg 1.0, layer 3  320   32000 = 1441
// mpeg 2.x, layer 1  256    8000 = 1540
// mpeg 2.x, layer 2  160    8000 = 2881
// mpeg 2.x, layer 3  160    8000 = 1441

#define MAXFRAMESIZE  2881
#define HDRCMPMASK    0xFFFF0D00UL
#define MAXRESYNC     65535

// Special mask for tests with variable sample rates. 
// Yes, we have also such strange tests.
//
// #define HDRCMPMASK 0xFFFF0100UL

typedef struct _DECODER_STRUCT
{
   XFILE* file;
   char   filename[_MAX_PATH];
   FILE*  save;
   char   savename[_MAX_PATH];

   XHEADDATA     xing_header;
   unsigned char xing_TOC[100];
   FORMAT_INFO   output_format;

   HEV   play;            // For internal use to sync the decoder thread.
   HEV   ok;              // For internal use to sync the decoder thread.
   HMTX  mutex;           // For internal use to sync the decoder thread.
   int   decodertid;      // Decoder thread indentifier.
   BOOL  stop;
   BOOL  frew;
   BOOL  ffwd;
   int   jumptopos;
   ULONG status;
   HWND  hwnd;            // PM interface main frame window handle.
   int   obr;             // For VBR event.
   int   abr;             // For position calculation.
   int   started;         // Position of the beginning of the data stream.

   char* metadata_buff;
   int   metadata_size;

   void  DLLENTRYP(error_display)( const char* );
   void  DLLENTRYP(info_display )( const char* );
   int   DLLENTRYP(output_play_samples)( void* a, const FORMAT_INFO* format, const char* buf, int len, int posmarker );

   void* a;               // Only to be used with the precedent function.
   int   audio_buffersize;

} DECODER_STRUCT;

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
  int (*synth)( real*, int, unsigned char* );
  int (*synth_mono)( real*, unsigned char* );
  int stereo;
  int jsbound;
  int single;
  int II_sblimit;
  int lsf;        // 0: MPEG 1.0; 1: MPEG 2.0/2.5
  int mpeg25;
  int down_sample;
  int block_size;
  int lay;
  int (*do_layer)( DECODER_STRUCT* w, struct frame* fr );
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
  int framesize;
  int ssize;      // Size of the layer 3 frame side information.
  int filepos;    // Position from the beginning of the data stream.
};

struct frame_buffer {

  int size;
  unsigned char* main_data;
  unsigned char  reserved[512];
  unsigned char  data[MAXFRAMESIZE];
};

struct flags {

  int equalizer;
  int mp3_eq;
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

extern BOOL   read_header( DECODER_STRUCT* w, unsigned long* header );
extern BOOL   read_next_header( DECODER_STRUCT* w, unsigned long* header );
extern BOOL   read_initialize( DECODER_STRUCT* w, struct frame* fr );
extern BOOL   read_first_frame_header( DECODER_STRUCT* w, struct frame* fr, unsigned long* header );
extern BOOL   is_header_valid( unsigned long header );
extern BOOL   output_flush( DECODER_STRUCT* w, struct frame* fr );
extern BOOL   seekto_pos( DECODER_STRUCT* w, struct frame* fr, int bytes );
extern BOOL   decode_header( DECODER_STRUCT* w, int header, struct frame* fr );
extern BOOL   complete_main_data( unsigned int backstep, struct frame* fr );
extern BOOL   read_frame( DECODER_STRUCT* w, struct frame* fr );
extern void   clear_decoder( void );
extern void   clear_layer3( void );
extern int    do_layer3( DECODER_STRUCT* w, struct frame*fr );
extern int    do_layer2( DECODER_STRUCT* w, struct frame* fr );
extern int    do_layer1( DECODER_STRUCT* w, struct frame* fr );
extern void   do_equalizer( real* bandPtr, int channel );
extern void   do_mp3eq( real* bandPtr, int channel );
extern void   init_eq( int rate );
extern int    synth_1to1( real*, int, unsigned char* );
extern int    synth_1to1_8bit( real*, int, unsigned char* );
extern int    synth_2to1( real*, int, unsigned char* );
extern int    synth_2to1_8bit( real*, int, unsigned char* );
extern int    synth_4to1( real*, int, unsigned char* );
extern int    synth_4to1_8bit( real*, int, unsigned char* );
extern int    synth_1to1_mono( real*, unsigned char* );
extern int    synth_1to1_mono2stereo( real*, unsigned char* );
extern int    synth_1to1_8bit_mono( real*, unsigned char* );
extern int    synth_1to1_8bit_mono2stereo( real*, unsigned char* );
extern int    synth_2to1_mono( real*, unsigned char* );
extern int    synth_2to1_mono2stereo( real*, unsigned char* );
extern int    synth_2to1_8bit_mono( real*, unsigned char* );
extern int    synth_2to1_8bit_mono2stereo( real*, unsigned char* );
extern int    synth_4to1_mono( real*, unsigned char* );
extern int    synth_4to1_mono2stereo( real*, unsigned char* );
extern int    synth_4to1_8bit_mono( real*, unsigned char* );
extern int    synth_4to1_8bit_mono2stereo( real*, unsigned char* );
extern void   init_layer3( int );
extern void   init_layer2( int );
extern void   make_conv16to8_table( int );

extern void _Optlink make_decode_tables( long scale );
extern void _Optlink dct64_MMX( short*, short*, real* );
extern BOOL _Optlink detect_mmx( void );
extern int  _Optlink synth_1to1_MMX( real*, int, unsigned char* );
extern void _Optlink dct64( real*, real*, real* );
