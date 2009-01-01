#include <malloc.h>

#include "common.h"
#include "tables.h"
#include <debuglog.h>

#include <debuglog.h>
#include <strutils.h>
#include <minmax.h>

#define  IS_4CH_TAG( ul, c1, c2, c3, c4 ) \
         ( ul == (((c1<<24)|(c2<<16)|(c3<<8)|(c4)) & 0xFFFFFFFFUL ))
#define  IS_3CH_TAG( ul, c1, c2, c3 ) \
         (( ul & 0xFFFFFF00UL ) == (((c1<<24)|(c2<<16)|(c3<<8)) & 0xFFFFFF00UL ))

int tabsel_123[2][3][16] = {
    {{ 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448 },
     { 0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384 },
     { 0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320 }},

    {{ 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256 },
     { 0,  8, 16, 24, 32, 40, 48,  56,  64,  80, 96,  112, 128, 144, 160 },
     { 0,  8, 16, 24, 32, 40, 48,  56,  64,  80, 96,  112, 128, 144, 160 }}
};

long freqs[9] = { 44100, 48000, 32000, 22050, 24000, 16000, 11025, 12000, 8000 };

void
mpg_init( MPG_FILE* mpg, int use_mmx )
{
  mpg->use_mmx = use_mmx;
  make_decode_tables( 32768 );

  init_layer2( mpg ); // also shared tables with layer1.
  init_layer3( mpg );
}

/* Reads a MPEG header from the input file. Returns 0 if it
   successfully reads a header, or -1 if any errors were detected. */
static int
read_header( XFILE* file, unsigned long* header )
{
  unsigned char buffer[4];

  if( xio_fread( buffer, 1, 4, file ) != 4 ) {
    return -1;
  }

  *header = ((unsigned long)buffer[0] << 24) |
            ((unsigned long)buffer[1] << 16) |
            ((unsigned long)buffer[2] <<  8) |
             (unsigned long)buffer[3];
  return 0;
}

/* Reads a next MPEG header from the input file. Returns 0 if it
   successfully reads a header, or -1 if any errors were detected. */
static int
read_next_header( XFILE* file, unsigned long* header )
{
  unsigned char byte;

  if( xio_fread( &byte, 1, 1, file ) != 1 ) {
    return -1;
  }

  *header = ( *header << 8 ) | byte ;
  return 0;
}

/* Skips a ID3v2 tag's data and reads a next MPEG header from the
   input file. Returns 0 if it successfully reads a header, or
   -1 if any errors were detected. */
static int
skip_id3v2_tag( XFILE* file, unsigned long* header )
{
  unsigned long size;
  char* buffer;

  if( read_next_header( file, header ) != 0 ) {
    return -1;
  }

  // Version or revision will never be 0xFF.
  if((( *header & 0x0000FF00 ) == 0x0000FF00 ) ||
     (( *header & 0x000000FF ) == 0x000000FF ))
  {
    return -1;
  }

  // Skips flags.
  if( read_next_header( file, header ) != 0 ) {
    return -1;
  }

  if( read_header( file, header ) != 0 ) {
    return -1;
  }

  // Must be a 32 bit synchsafe integer.
  if( *header & 0x80808080UL ) {
    return  0;
  }

  size = (( *header & 0xFF000000UL ) >> 3 ) |
         (( *header & 0x00FF0000UL ) >> 2 ) |
         (( *header & 0x0000FF00UL ) >> 1 ) |
          ( *header & 0x000000FFUL );

  if(!( buffer = malloc( size ))) {
    return -1;
  }

  if( xio_fread( buffer, 1, size, file ) != size ) {
    free( buffer );
    return -1;
  }

  free( buffer );
  DEBUGLOG(( "mpg123: skipped %d bytes of the ID3v2 tag up to %d.\n", size + 10, xio_ftell( file )));
  return read_header( file, header );
}

/* Returns 1 if a specified header is the valid MPEG header. */
static int
is_header_valid( unsigned long header )
{
  // First 11 bits are set to 1 for frame sync.
  if(( header & 0xFFE00000 ) != 0xFFE00000 ) {
    return 0;
  }
  // Layer: 01,10,11 is 1,2,3; 00 is reserved.
  if(!(( header >> 17 ) & 0x3 )) {
    return 0;
  }
  // 1111 means bad bitrate.
  if( (( header >> 12 ) & 0xF ) == 0xF ) {
    return 0;
  }
  // 0000 means free format.
  if( (( header >> 12 ) & 0xF ) == 0x0 ) {
    return 0;
  }
  // Sampling freq: 11 is reserved.
  if( (( header >> 10 ) & 0x3 ) == 0x3 ) {
    return 0;
  }

  return 1;
}

static void
get_II_stuff( struct frame* fr )
{
  static int translate[3][2][16] =
   {{{ 0,2,2,2,2,2,2,0,0,0,1,1,1,1,1,0 } ,
     { 0,2,2,0,0,0,1,1,1,1,1,1,1,1,1,0 }},
    {{ 0,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0 } ,
     { 0,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0 }},
    {{ 0,3,3,3,3,3,3,0,0,0,1,1,1,1,1,0 } ,
     { 0,3,3,0,0,0,1,1,1,1,1,1,1,1,1,0 }}};

  int table, sblim;
  static struct al_table *tables[5] =
       { alloc_0, alloc_1, alloc_2, alloc_3 , alloc_4 };

  static int sblims[5] = { 27 , 30 , 8, 12 , 30 };

  if( fr->lsf ) {
    table = 4;
  } else {
    table = translate[fr->sampling_frequency][2-fr->channels][fr->bitrate_index];
  }
  sblim = sblims[table];

  fr->alloc = tables[table];
  fr->II_sblimit = sblim;
}

/* Decodes a header and writes the decoded information
   into the frame structure. Returns 0 if it successfully
   decodes a header, or -1 if any errors were detected. */
static int
decode_header( int header, struct frame* fr )
{
  if( !is_header_valid( header )) {
    return -1;
  }

  if( header & ( 1 << 20 )) {
    fr->lsf    = ( header & ( 1 << 19 )) ? 0x0 : 0x1;
    fr->mpeg25 = 0;
  } else {
    fr->lsf    = 1;
    fr->mpeg25 = 1;
  }

  fr->layer = 4 - (( header >> 17 ) & 3 );
  fr->bitrate_index = (( header >> 12 ) & 0xF );

  if( fr->mpeg25 ) {
    fr->sampling_frequency = 6 + (( header >> 10 ) & 0x3 );
  } else {
    fr->sampling_frequency = (( header >> 10 ) & 0x3 ) + ( fr->lsf * 3 );
  }

  fr->error_protection = (( header >> 16 ) & 0x1 ) ^ 0x1;
  fr->bitrate_index    = (( header >> 12 ) & 0xF );
  fr->padding          = (( header >>  9 ) & 0x1 );
  fr->extension        = (( header >>  8 ) & 0x1 );
  fr->mode             = (( header >>  6 ) & 0x3 );
  fr->mode_ext         = (( header >>  4 ) & 0x3 );
  fr->copyright        = (( header >>  3 ) & 0x1 );
  fr->original         = (( header >>  2 ) & 0x1 );
  fr->emphasis         = header & 0x3;
  fr->channels         = ( fr->mode == MPG_MD_MONO ) ? 1 : 2;
  fr->bitrate          = tabsel_123[fr->lsf][fr->layer-1][fr->bitrate_index];

  if( !fr->bitrate_index ) {
    DEBUGLOG(( "mpg123: Free format is not supported.\n" ));
    return -1;
  }

  switch( fr->layer )
  {
    case 1:
      fr->jsbound    = fr->mode == MPG_MD_JOINT_STEREO ? ( fr->mode_ext << 2 ) + 4 : 32;
      fr->framesize  = fr->bitrate * 12000;
      fr->framesize /= freqs[fr->sampling_frequency];
      fr->framesize  = (( fr->framesize + fr->padding ) << 2 ) - 4;
      fr->ssize      = 0;
      break;

    case 2:
      get_II_stuff( fr );

      fr->jsbound    = fr->mode == MPG_MD_JOINT_STEREO ? ( fr->mode_ext << 2 ) + 4 : fr->II_sblimit;
      fr->framesize  = fr->bitrate * 144000;
      fr->framesize /= freqs[fr->sampling_frequency];
      fr->framesize  = fr->framesize + fr->padding - 4;
      fr->ssize      = 0;
      break;

    case 3:
      if( fr->lsf ) {
        fr->ssize = (fr->channels == 1) ?  9 : 17;
      } else {
        fr->ssize = (fr->channels == 1) ? 17 : 32;
      }

      if( fr->error_protection ) {
        fr->ssize += 2;
      }
      fr->framesize  = fr->bitrate * 144000;
      fr->framesize /= freqs[fr->sampling_frequency] << fr->lsf;
      fr->framesize  = fr->framesize + fr->padding - 4;
      break;

    default:
      fr->framesize = 0;
  }

  if( fr->framesize > MAXFRAMESIZE ) {
    return -1;
  }

  return 0;
}

/* Searches a first frame header and decodes it. After completion of
   this function a stream pointer can point to header of the
   following frame. Do not use the received results for reading
   the frame! */
static int
read_first_frame_header( MPG_FILE* mpg, struct frame* fr, unsigned long* header, ID3V2_TAG** tagv2 )
{
  int read_ahead = xio_can_seek( mpg->file ) < 2 ? 2 : 32;
  int done_ahead;
  int back_ahead;
  int resync = 0;

  if( read_header( mpg->file, header ) != 0 ) {
    return -1;
  }

  // I even saw RIFF headers at the beginning of MPEG streams ;(
  if( IS_4CH_TAG( *header, 'R','I','F','F' )) {
    while( !IS_4CH_TAG( *header, 'd','a','t','a' )) {
      if( read_next_header( mpg->file, header ) != 0 ) {
        return -1;
      }
      if( ++resync > MAXRESYNC ) {
        DEBUGLOG(( "mpg123: giving up searching valid RIFF header...\n" ));
        return -1;
      }
    }
    if( read_header( mpg->file, header ) != 0 ) {
      return -1;
    }
    DEBUGLOG(( "mpg123: skipped RIFF header up to %d.\n", xio_ftell( mpg->file )));
  }

  while( decode_header( *header, fr ) != 0 )
  {

read_again:

    // Step in byte steps through next data.
    while( ++resync < MAXRESYNC ) {
      if( IS_3CH_TAG( *header, 'I','D','3' )) {
        if( tagv2 ) {
          xio_fseek( mpg->file, -4, XIO_SEEK_CUR );
          *tagv2 = id3v2_get_tag( mpg->file, 0 );
        } else if( skip_id3v2_tag( mpg->file, header ) != 0 ) {
          return -1;
        } else {
          if( is_header_valid( *header )) {
            break;
          }
        }
      }
      if( read_next_header( mpg->file, header ) != 0 ) {
        return -1;
      }
      if( is_header_valid( *header )) {
        break;
      }
    }
    if( resync > MAXRESYNC ) {
      DEBUGLOG(( "mpg123: giving up searching valid MPEG header...\n" ));
      return -1;
    }
  }

  back_ahead = 0;
  done_ahead = 0;
  mpg->xing_header.flags = 0;

  if( fr->layer == 3 )
  {
    // Let's try to find a XING VBR header.
    char buf[MAXFRAMESIZE];

    buf[0] = ( *header >> 24 ) & 0xFF;
    buf[1] = ( *header >> 16 ) & 0xFF;
    buf[2] = ( *header >>  8 ) & 0xFF;
    buf[3] = ( *header       ) & 0xFF;

    if( xio_fread( buf + 4, 1, fr->framesize, mpg->file ) != fr->framesize ) {
      return -1;
    }

    back_ahead -= fr->framesize;
    done_ahead++;

    if( GetXingHeader( &mpg->xing_header, buf )) {
      if( mpg->xing_header.flags && !( mpg->xing_header.flags & BYTES_FLAG ))
      {
        mpg->xing_header.bytes  = xio_fsize( mpg->file ) - xio_ftell( mpg->file ) - back_ahead + 4;
        mpg->xing_header.flags |= BYTES_FLAG;
      }
      // If there was a XING VBR header, it means precisely a mp3 file and
      // we do not need to look ahead.
      DEBUGLOG(( "mpg123: found a valid XING header.\n" ));
      goto done;
    } else {
      unsigned long next;

      if( xio_ftell( mpg->file ) == xio_fsize( mpg->file )) {
        DEBUGLOG(( "mpg123: found a very short mpeg file.\n" ));
        goto done;
      }

      // To pass a first step of a look ahead.
      if( read_header( mpg->file, &next ) != 0 ) {
        return -1;
      }

      back_ahead -= 4;

      if( IS_3CH_TAG( *header, 'T','A','G' ) &&
        ( xio_fsize( mpg->file ) - xio_ftell( mpg->file ) == 124 ))
      {
        DEBUGLOG(( "mpg123: found a very short mpeg file.\n" ));
        goto done;
      }

      if(( next & HDRCMPMASK ) != ( *header & HDRCMPMASK ) ||
           decode_header( next, fr ) != 0 )
      {
        xio_fseek( mpg->file, back_ahead, XIO_SEEK_CUR );
        goto read_again;
      }
    }
  }

  for( ; done_ahead < read_ahead; done_ahead++ )
  {
    unsigned long next;
    char data[MAXFRAMESIZE];

    if( xio_fread( data, 1, fr->framesize, mpg->file ) == fr->framesize )
    {
      back_ahead -= ( fr->framesize );

      if( xio_ftell( mpg->file ) == xio_fsize( mpg->file )) {
        DEBUGLOG(( "mpg123: found a very short mpeg file.\n" ));
        break;
      }

      // To pass a first step of a look ahead.
      if( read_header( mpg->file, &next ) != 0 ) {
        return -1;
      }

      back_ahead -= 4;

      if( IS_3CH_TAG( next, 'T','A','G' ) &&
        ( xio_fsize( mpg->file ) - xio_ftell( mpg->file ) == 124 ))
      {
        DEBUGLOG(( "mpg123: found a very short mpeg file.\n" ));
        break;
      }

      if(( next & HDRCMPMASK ) != ( *header & HDRCMPMASK ) ||
           decode_header( next, fr ) != 0 )
      {
        xio_fseek( mpg->file, back_ahead, XIO_SEEK_CUR );
        goto read_again;
      }
    } else {
      return -1;
    }
  }

done:

  mpg->started = xio_ftell( mpg->file ) + back_ahead - 4;
  // xio_fseek( w->file, back_ahead - 4, XIO_SEEK_CUR );
  xio_fseek( mpg->file, mpg->started, XIO_SEEK_SET );
  // Restore a first frame broken by previous tests.
  decode_header( *header, fr );

  DEBUGLOG(( "mpg123: data stream is started at %d.\n", mpg->started ));
  return 0;
}

/* Clears common data areas of the decoder. */
static void
clear_decoder( MPG_FILE* mpg )
{
  memset( &mpg->block_data, 0, sizeof( mpg->block_data ));
  memset( &mpg->synth_data, 0, sizeof( mpg->synth_data ));

  mpg->synth_data.bo    = 1;
  mpg->bs.bsbuf[0].size = 0;
  mpg->bs.bsbuf[1].size = 0;
}

/* Open MPEG file. Returns 0 if it successfully opens the file.
   A nonzero return value indicates an error. A -1 return value
   indicates an unsupported format of the file. */
int
mpg_open_file( MPG_FILE* mpg, const char* filename, const char* mode )
{
  strlcpy( mpg->filename, filename, sizeof( mpg->filename ));

  memset( &mpg->xing_header, 0, sizeof( mpg->xing_header ));
  memset( &mpg->xing_TOC,    0, sizeof( mpg->xing_TOC    ));
  memset( &mpg->fr,          0, sizeof( mpg->fr          ));

  mpg->xing_header.toc = mpg->xing_TOC;

  mpg->first_header = 0;
  mpg->frame_header = 0;
  mpg->started      = 0;
  mpg->bitrate      = 0;
  mpg->songlength   = 0;
  mpg->samplerate   = 0;
  mpg->filesize     = 0;
  mpg->is_stream    = 1;

  id3v1_clean_tag( &mpg->tagv1 );
  mpg->tagv2 = NULL;

  #if SCALE_BLOCK * 3 > SSLIMIT
  mpg->pcm_samples  = malloc( SCALE_BLOCK * 3 * 128 );
  #else
  mpg->pcm_samples  = malloc( SSLIMIT * 128 );
  #endif

  mpg->pcm_point    = 0;
  mpg->pcm_count    = 0;

  if( !mpg->pcm_samples ) {
    return -1;
  }

  mpg->file = xio_fopen( filename, mode );

  if( !mpg->file ) {
    free( mpg->pcm_samples );
    return xio_errno();
  }

  if( read_first_frame_header( mpg, &mpg->fr, &mpg->first_header, &mpg->tagv2 ) != 0 ) {
    mpg_close( mpg );
    return -1;
  }

  mpg->filesize   = xio_fsize( mpg->file );
  mpg->bitrate    = mpg->fr.bitrate;
  mpg->samplerate = freqs[mpg->fr.sampling_frequency];
  mpg->is_stream  = xio_can_seek( mpg->file ) != XIO_CAN_SEEK_FAST;

  if( mpg->xing_header.flags & FRAMES_FLAG ) {
    mpg->songlength = 8 * 144 * mpg->xing_header.frames * 1000.0 / ( mpg->samplerate << mpg->fr.lsf );
  } else if( mpg->xing_header.flags & BYTES_FLAG ) {
    mpg->songlength = mpg->xing_header.bytes * 1000.0 / mpg->bitrate / 125;
  } else {
    mpg->songlength = ( mpg->filesize - mpg->started ) * 1000.0 / mpg->bitrate / 125;
  }

  if( mpg->xing_header.flags & BYTES_FLAG  &&
      mpg->xing_header.flags & FRAMES_FLAG && mpg->songlength )
  {
    mpg->bitrate = mpg->xing_header.bytes / mpg->songlength * 1000.0 / 125;
  }

  if( !mpg->is_stream ) {
    // TODO: the tag should not be read unless it is required.
    // Currently also the decoder_thread reads the tag. This confuses the xio buffer.
    id3v1_get_tag( mpg->file, &mpg->tagv1 );
    xio_fseek( mpg->file, mpg->started, XIO_SEEK_SET );
  }

  clear_decoder( mpg );

  if( mpg->use_common_eq || mpg->use_layer3_eq ) {
    eq_init( mpg );
  }

  return 0;
}

/* Closes the MPEG file. */
int
mpg_close( MPG_FILE* mpg )
{
  int rc = 0;

  if( mpg->file ) {
    rc = xio_fclose( mpg->file );
    mpg->file = NULL;
  }

  if( mpg->pcm_samples ) {
    free( mpg->pcm_samples );
    mpg->pcm_samples = NULL;
  }

  if( mpg->tagv2 ) {
    id3v2_free_tag( mpg->tagv2 );
  }

  return rc;
}

/* Aborts the MPEG file. */
void
mpg_abort( MPG_FILE* mpg )
{
  if( mpg->file ) {
    xio_fabort( mpg->file );
  }
}

/* Controls the equalizer. */
void
mpg_equalize( MPG_FILE* mpg, int enable, const real* bandgain )
{
  mpg->use_common_eq = enable;
  mpg->use_layer3_eq = enable;

  if( enable ) {
    memcpy( &mpg->octave_eq, bandgain, sizeof( mpg->octave_eq ));

    if( mpg->file ) {
      eq_init( mpg );
    }
  }
}

/* Reads a next valid header and decodes it. Returns 0 if it
   successfully reads a header, or -1 if any errors were detected. */
static int
read_synchronize( MPG_FILE* mpg, unsigned long* header )
{
  int resync = 0;

  if( read_header( mpg->file, header ) != 0 ) {
    return -1;
  }

  while(( *header & HDRCMPMASK ) != ( mpg->first_header & HDRCMPMASK ) ||
           decode_header( *header, &mpg->fr ) != 0 )
  {
    if( IS_3CH_TAG( *header, 'T','A','G' )
        && ( xio_fsize( mpg->file ) - xio_ftell( mpg->file ) == 124 ))
    {
      // Found ID3v1 tag.
      DEBUGLOG(( "mpg123: skipped ID3v1 tag.\n" ));
      return -1;
    }
    if( IS_3CH_TAG( *header, 'I','D','3' )) {
      if( skip_id3v2_tag( mpg->file, header ) != 0 ) {
        return -1;
      } else {
        continue;
      }
    }
    if( read_next_header( mpg->file, header ) != 0 ) {
      return -1;
    }
    if( ++resync > MAXRESYNC ) {
      return -1;
    }
  }

  if( resync ) {
    mpg->bs.bsbuf[0].size = 0;
    mpg->bs.bsbuf[1].size = 0;
    DEBUGLOG(( "mpg123: stream is resynced after %d attempts.\n", resync ));
  }

  return 0;
}

/* returns playing position in ms... too CPU intensive?! */
int
mpg_tell_ms( MPG_FILE* mpg )
{
  if(( mpg->xing_header.flags & FRAMES_FLAG ) &&
     ( mpg->xing_header.flags & TOC_FLAG    ) &&
     ( mpg->xing_header.flags & BYTES_FLAG  ))
  {
    float tofind  = 256.0 * mpg->fr.filepos / mpg->xing_header.bytes;
    float percent = 0;
    int   i       = 0;
    float fa, fb;

    for( i = 1; i < 100; i++ ) {
      if( tofind < mpg->xing_header.toc[i] ) {
        break;
      }
    }

    fa = mpg->xing_header.toc[i-1];

    if( i == 100 ) {
      fb = 256;
    } else {
      fb = mpg->xing_header.toc[i];
    }

    percent = ( tofind - fa ) / ( fb - fa ) + i - 1;
    return percent * mpg->songlength / 100;
  }
  else if(( mpg->xing_header.flags & FRAMES_FLAG ) &&
          ( mpg->xing_header.flags & BYTES_FLAG  ))
  {
    return (float)mpg->fr.filepos * mpg->songlength / mpg->xing_header.bytes;
  } else {
    return (float)mpg->fr.filepos * 1000 / mpg->bitrate / 125;
  }
}

/* Reads a next valid frame and decodes the frame header. Returns 0 if it
   successfully reads a frame, or -1 if any errors were detected. */
static int
read_next_frame( MPG_FILE* mpg )
{
  int done;

  if( read_synchronize( mpg, &mpg->frame_header ) != 0 ) {
    return -1;
  }

  // Position from the beginning of the data stream.
  mpg->fr.filepos = xio_ftell( mpg->file ) - 4 - mpg->started;

  mpg->bs.bsnum ^= 1;
  done = xio_fread( mpg->bs.bsbuf[mpg->bs.bsnum].data, 1, mpg->fr.framesize, mpg->file );

  DEBUGLOG2(( "mpg123: read %d bytes from %d to %08X\n", done, 
                               mpg->fr.framesize, &mpg->bs.bsbuf[mpg->bs.bsnum].data ));
  if( done <= 0 ) {
    return -1;
  } else if( done != mpg->fr.framesize ) {
    memset( mpg->bs.bsbuf[mpg->bs.bsnum].data + done, 0, mpg->fr.framesize - done );
  }

  mpg->bs.bitindex = 0;
  mpg->bs.wordptr  = mpg->bs.bsbuf[mpg->bs.bsnum].data;
  mpg->bs.bsbuf[mpg->bs.bsnum].p_main_data = mpg->bs.bsbuf[mpg->bs.bsnum].data + mpg->fr.ssize;
  mpg->bs.bsbuf[mpg->bs.bsnum].size = mpg->fr.framesize - mpg->fr.ssize;

  return 0;
}

/* Reads a next valid frame and decodes it. Returns 0 if it
   successfully reads a frame, or -1 if any errors were detected. */
int
mpg_read_frame( MPG_FILE* mpg )
{
  if( read_next_frame( mpg ) != 0 ) {
    return -1;
  }

  if( mpg->fr.error_protection ) {
    getbits( &mpg->bs, 16 ); // crc
  }

  return 0;
}

/* Decodes current frame. Returns 0 if it
   successfully decodes a frame, or -1 if any errors were detected. */
int
mpg_decode_frame( MPG_FILE* mpg )
{
  switch( mpg->fr.layer )
  {
    case 1:
      do_layer1( mpg );
      break;
    case 2:
      do_layer2( mpg );
      break;
    case 3:
      do_layer3( mpg );
      break;
    default:
      return -1;
  }

  return 0;
}

/* Move a count of bytes of the decoded audio samples
   to the specified buffer. */
int
mpg_move_sound( MPG_FILE* mpg, unsigned char* buffer, int count )
{
  int copy = min( count, mpg->pcm_count );

  memcpy( buffer, mpg->pcm_samples + mpg->pcm_point, copy );
  mpg->pcm_point += copy;
  mpg->pcm_count -= copy;

  return copy;
}

/* Saves a currently loaded frame. Returns 0 if it
   successfully saves a frame, or -1 if any errors were detected. */
int
mpg_save_frame( MPG_FILE* mpg, XFILE* save )
{
  unsigned char buf[4];

  buf[0] = ( mpg->frame_header >> 24 ) & 0xFF;
  buf[1] = ( mpg->frame_header >> 16 ) & 0xFF;
  buf[2] = ( mpg->frame_header >>  8 ) & 0xFF;
  buf[3] = ( mpg->frame_header       ) & 0xFF;

  if( xio_fwrite( buf, 1, 4, save ) != 4 ||
      xio_fwrite( mpg->bs.bsbuf[mpg->bs.bsnum].data, 1, mpg->fr.framesize, save ) != mpg->fr.framesize )
  {
    return -1;
  }
   
  DEBUGLOG2(( "mpg123: save %d bytes from %08X\n", mpg->fr.framesize, 
                                                  &mpg->bs.bsbuf[mpg->bs.bsnum].data ));
  return 0;
}

int
complete_frame_main_data( struct bit_stream* bs, unsigned backstep, struct frame* fr )
{
  int bsold = bs->bsnum ^ 1;
  int rc = 0;

  if( bs->bsbuf[bsold].size < backstep ) {
    DEBUGLOG(( "mpg123: can't copy %d bytes of the main data from %d previous bytes\n",
                                                        backstep, bs->bsbuf[bsold].size ));
    // If the previous frame has no enough data for a
    // main data completion then copy data as much as
    // possible.
    backstep = bs->bsbuf[bsold].size;
    rc = -1;
  }

  if( backstep ) {
    bs->bsbuf[bs->bsnum].p_main_data -= backstep;
    bs->bsbuf[bs->bsnum].size += backstep;

    memcpy( bs->bsbuf[bs->bsnum].p_main_data, bs->bsbuf[bsold].p_main_data + bs->bsbuf[bsold].size - backstep, backstep );
  }

  bs->wordptr  = bs->bsbuf[bs->bsnum].p_main_data;
  bs->bitindex = 0;
  return rc;
}

/* Changes the current file position to a new location within the file
   and synchronize the data stream. Returns 0 if it successfully
   moves the pointer. */
static int
seek_pos( MPG_FILE* mpg, int pos )
{
  unsigned long header;

  if( !xio_can_seek( mpg->file )) {
    // This only works on files.
    return -1;
  }

  if( mpg->fr.layer == 3 ) {
    pos -= 2 * mpg->fr.framesize + 8;
    pos  = max( mpg->started, pos );
  }

  if( xio_fseek( mpg->file, pos, XIO_SEEK_SET ) != -1 ) {
    if( read_synchronize( mpg, &header ) == 0 ) {
      if( xio_fseek( mpg->file, -4, XIO_SEEK_CUR ) != -1 )
      {
        // Clears the frames buffer.
        mpg->bs.bsbuf[0].size = 0;
        mpg->bs.bsbuf[1].size = 0;

        if( mpg->fr.layer == 3 ) {
          read_next_frame( mpg );
          read_next_frame( mpg );
          complete_frame_main_data( &mpg->bs, 512, &mpg->fr );
        }
        return 0;
      }
    }
  }
  return -1;
}

/* Changes the current file position to a new time within the file.
   Returns 0 if it successfully moves the pointer. A nonzero return
   value indicates an error. On devices that cannot seek the return
   value is nonzero. */
int
mpg_seek_ms( MPG_FILE* mpg, int ms, int origin )
{
  int pos;

  switch( origin ) {
    case MPG_SEEK_SET:
      break;
    case MPG_SEEK_CUR:
      ms = mpg_tell_ms( mpg ) + ms;
      break;
    default:
      return -1;
  }

  if( ms < 0 ) {
    return -1;
  }

  if(( mpg->xing_header.flags & FRAMES_FLAG ) &&
     ( mpg->xing_header.flags & TOC_FLAG    ) &&
     ( mpg->xing_header.flags & BYTES_FLAG  ))
  {
    pos = SeekPoint( mpg->xing_header.toc, mpg->xing_header.bytes, (float)ms / mpg->songlength * 100 );
  }
  else if(( mpg->xing_header.flags & FRAMES_FLAG ) &&
          ( mpg->xing_header.flags & BYTES_FLAG  ))
  {
    pos = (float)ms * mpg->xing_header.bytes / mpg->songlength;
  } else {
    pos = (float)ms * mpg->bitrate * 125 / 1000;
  }

  pos += mpg->started;
  return seek_pos( mpg, pos );
}

unsigned int
getbits( struct bit_stream* bs, int number_of_bits )
{
  unsigned long rval;

  rval = bs->wordptr[0];
  rval <<= 8;
  rval |= bs->wordptr[1];
  rval <<= 8;
  rval |= bs->wordptr[2];
  rval <<= bs->bitindex;
  rval &= 0xffffff;

  bs->bitindex += number_of_bits;
  rval >>= ( 24 - number_of_bits );
  bs->wordptr += ( bs->bitindex >> 3 );
  bs->bitindex &= 7;

  return rval;
}

unsigned int
get1bit( struct bit_stream* bs )
{
  unsigned char rval;

  rval = *bs->wordptr << bs->bitindex;

  bs->bitindex++;
  bs->wordptr += ( bs->bitindex >> 3 );
  bs->bitindex &= 7;

  return rval >> 7;
}

