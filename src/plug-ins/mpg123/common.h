/*
 * mpg123 defines
 */

#include <stdlib.h>
#include "dxhead.h"

#include <xio.h>
#include <id3v1.h>
#include <id3v2.h>

#ifndef M_PI
#define M_PI    3.14159265358979323846
#endif
#ifndef M_SQRT2
#define M_SQRT2 1.41421356237309504880
#endif

#define REAL_IS_FLOAT

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
// For Layer 3 (MPEG 2.x):
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
#define HDRCMPMASK    0xFFFE0C00UL
#define MAXRESYNC     65535

// Special mask for tests with variable sample rates.
// Yes, we have also such strange tests.
//
// #define HDRCMPMASK 0xFFFF0100UL

#define SBLIMIT               32
#define SCALE_BLOCK           12
#define SSLIMIT               18

#define MPG_MD_STEREO         0
#define MPG_MD_JOINT_STEREO   1
#define MPG_MD_DUAL_CHANNEL   2
#define MPG_MD_MONO           3

struct al_table
{
  short bits;
  short d;
};

struct gr_info_s {

  int      scfsi;
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
  real*    full_gain[3];
  real*    pow2gain;
};

struct III_sideinfo {

  unsigned main_data_begin;
  unsigned private_bits;

  struct {
    struct gr_info_s gr[2];
  } ch[2];
};

struct frame {

  struct al_table* alloc;

  int channels;
  int jsbound;
  int II_sblimit;
  int lsf;        // 0: MPEG 1.0; 1: MPEG 2.0/2.5
  int mpeg25;
  int layer;
  int error_protection;
  int bitrate_index;
  int bitrate;
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

struct synth
{
  int  bo;
  real buffs[2][2][0x110];
};

struct frame_data {

  int            size;
  unsigned char* p_main_data;
  unsigned char  backstep_data[512];
  unsigned char  data[MAXFRAMESIZE];
};

struct bit_stream {

  int            bitindex;
  unsigned char* wordptr;

  struct frame_data bsbuf[2];
  int bsnum;
};

extern real  muls[27][64];
extern real  decwin[512+32];
extern real* pnts[5];

typedef struct _MPG_FILE
{
  XFILE* file;
  char   filename[_MAX_PATH];

  XHEADDATA     xing_header;
  unsigned char xing_TOC[100];
  unsigned long first_header;
  unsigned long frame_header;

  ID3V1_TAG  tagv1;
  ID3V2_TAG* tagv2;

  struct frame      fr;
  struct bit_stream bs;
  struct synth      synth_data;

  unsigned char* pcm_samples;
  unsigned int   pcm_point;
  unsigned int   pcm_count;

  real block_data[2][2][SBLIMIT*SSLIMIT];

  int  started;     // Position of the beginning of the data stream.
  int  bitrate;     // For position calculation.
  int  songlength;  // Number of milliseconds the stream lasts.
  int  samplerate;
  int  filesize;
  int  is_stream;

  int  use_common_eq;
  int  use_layer3_eq;
  real octave_eq[2][ 10];
  real common_eq[2][ 32];
  real layer3_eq[2][576];

  int  use_mmx;

} MPG_FILE;

#define MPG_SEEK_SET 0
#define MPG_SEEK_CUR 1

extern void mpg_init( MPG_FILE*, int use_mmx );
extern int  mpg_open_file ( MPG_FILE*, const char* filename, const char* mode );
extern int  mpg_close( MPG_FILE* );
extern void mpg_abort( MPG_FILE* );
extern int  mpg_read_frame( MPG_FILE* );
extern int  mpg_decode_frame( MPG_FILE* );
extern int  mpg_save_frame( MPG_FILE*, XFILE* save );
extern int  mpg_move_sound( MPG_FILE*, unsigned char* buffer, int size );

extern int  mpg_tell_ms( MPG_FILE* );
extern int  mpg_seek_ms( MPG_FILE*, int ms, int origin );
extern void mpg_equalize( MPG_FILE*, int enable, const real* bandgain );

extern void eq_init( MPG_FILE* );
extern void do_common_eq( MPG_FILE*, real* bandPtr, int channel );
extern void do_layer3_eq( MPG_FILE*, real* bandPtr, int channel );

extern int  synth_1to1( MPG_FILE*, real*, int, unsigned char* );
extern int  synth_1to1_mono2stereo( MPG_FILE*, real*, unsigned char* );
extern int  complete_frame_main_data( struct bit_stream*, unsigned backstep, struct frame* );

extern int  do_layer3( MPG_FILE* );
extern int  do_layer2( MPG_FILE* );
extern int  do_layer1( MPG_FILE* );

extern void init_layer3( MPG_FILE* );
extern void init_layer2( MPG_FILE* );

extern void _Optlink make_decode_tables( long scale );
extern void _Optlink dct64_MMX( short*, short*, real* );
extern int  _Optlink detect_MMX( void );
extern int  _Optlink synth_1to1_MMX( struct synth*, real*, int, unsigned char* );
extern void _Optlink dct64( real*, real*, real* );

extern unsigned int get1bit( struct bit_stream* bs );
extern unsigned int getbits( struct bit_stream* bs, int );

