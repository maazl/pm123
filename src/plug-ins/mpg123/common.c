#include <ctype.h>
#include <stdlib.h>
#include <malloc.h>
#include <io.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mpg123.h"
#include "tables.h"
#include "debuglog.h"

#define  IS_4CH_TAG( ul, c1, c2, c3, c4 ) \
         ( ul == (((c1<<24)|(c2<<16)|(c3<<8)|(c4)) & 0xFFFFFFFFUL ))
#define  IS_3CH_TAG( ul, c1, c2, c3 ) \
         (( ul & 0xFFFFFF00UL ) == (((c1<<24)|(c2<<16)|(c3<<8)) & 0xFFFFFF00UL ))

int tabsel_123[2][3][16] = {
   { { 0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448 },
     { 0, 32, 48, 56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320, 384 },
     { 0, 32, 40, 48,  56,  64,  80,  96, 112, 128, 160, 192, 224, 256, 320 }},

   { { 0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256 },
     { 0,  8, 16, 24, 32, 40, 48,  56,  64,  80, 96,  112, 128, 144, 160 },
     { 0,  8, 16, 24, 32, 40, 48,  56,  64,  80, 96,  112, 128, 144, 160 }}
};

long freqs[9] = { 44100, 48000, 32000, 22050, 24000, 16000, 11025, 12000, 8000 };

static int bitindex;
static unsigned char* wordpointer;
static struct frame_buffer bsbuf[2];
static int bsnum = 0;

unsigned char* pcm_sample = NULL;
int pcm_point = 0;

static unsigned long firsthead = 0;

static void
get_II_stuff( struct frame* fr )
{
  static int translate[3][2][16] =
   { { { 0,2,2,2,2,2,2,0,0,0,1,1,1,1,1,0 } ,
       { 0,2,2,0,0,0,1,1,1,1,1,1,1,1,1,0 } } ,
     { { 0,2,2,2,2,2,2,0,0,0,0,0,0,0,0,0 } ,
       { 0,2,2,0,0,0,0,0,0,0,0,0,0,0,0,0 } } ,
     { { 0,3,3,3,3,3,3,0,0,0,1,1,1,1,1,0 } ,
       { 0,3,3,0,0,0,1,1,1,1,1,1,1,1,1,0 } } };

  int table, sblim;
  static struct al_table *tables[5] =
       { alloc_0, alloc_1, alloc_2, alloc_3 , alloc_4 };
  static int sblims[5] = { 27 , 30 , 8, 12 , 30 };

  if( fr->lsf ) {
    table = 4;
  } else {
    table = translate[fr->sampling_frequency][2-fr->stereo][fr->bitrate_index];
  }
  sblim = sblims[table];

  fr->alloc = tables[table];
  fr->II_sblimit = sblim;
}

/* returns playing position in ms... too CPU intensive?! */
static int
decoding_pos( DECODER_STRUCT* w, struct frame* fr )
{
  _control87( EM_OVERFLOW | EM_UNDERFLOW | EM_ZERODIVIDE |
              EM_INEXACT  | EM_INVALID   | EM_DENORMAL, MCW_EM );

  if(( w->xing_header.flags & FRAMES_FLAG ) &&
     ( w->xing_header.flags & TOC_FLAG    ) &&
     ( w->xing_header.flags & BYTES_FLAG  ))
  {
    float tofind  = (float)fr->filepos * 256 / w->xing_header.bytes;
    float percent = 0.0;
    int   i       = 0;
    float fa, fb;

    for( i = 1; i < 100; i++ ) {
      if( tofind < w->xing_TOC[i] ) {
        break;
      }
    }

    fa = w->xing_TOC[i-1];

    if( i == 100 ) {
      fb = 256;
    } else {
      fb = w->xing_TOC[i];
    }

    percent = ( tofind - fa ) / ( fb - fa ) + i - 1;

    // 8 bit / byte and 8*144 samples / frame (?) for MP3
    return 1000 * percent * ( 8 * 144 * w->xing_header.frames / freqs[fr->sampling_frequency] ) / 100;
  }
  else if(( w->xing_header.flags & FRAMES_FLAG ) &&
          ( w->xing_header.flags & BYTES_FLAG  ))
  {
    int songlength = 8.0 * 144 * w->xing_header.frames * 1000 / freqs[fr->sampling_frequency];
    return (float)fr->filepos * songlength / w->xing_header.bytes;
  } else {
    return (float)fr->filepos * 1000 / w->abr / 125;
  }
}

BOOL
output_flush( DECODER_STRUCT* w, struct frame* fr )
{
  int rc = TRUE;

  if( pcm_point ) {
    if( w->output_format.samplerate != freqs[fr->sampling_frequency] >> fr->down_sample )
    {
      w->output_format.samplerate = freqs[fr->sampling_frequency] >> fr->down_sample;
      init_eq( w->output_format.samplerate );
    }
    if( w->output_play_samples( w->a, &w->output_format, pcm_sample, pcm_point, decoding_pos( w, fr )) < pcm_point ) {
      rc = FALSE;
    }
    pcm_point = 0;
  }
  return rc;
}

/* Writes up to count items, each of size bytes in length, from buffer
   to the output file. Returns TRUE if it successfully writes the data,
   or FALSE if any errors were detected. */
static BOOL
save_data( void* buffer, size_t size, size_t count, DECODER_STRUCT* w )
{
  char errorbuf[1024];

  if( !w->save && *w->savename ) {
    // Opening a save stream.
    if(!( w->save = fopen( w->savename, "ab" ))) {
      strlcpy( errorbuf, "Could not open file to save data:\n", sizeof( errorbuf ));
      strlcat( errorbuf, w->savename, sizeof( errorbuf ));
      strlcat( errorbuf, "\n", sizeof( errorbuf ));
      strlcat( errorbuf, strerror( errno ), sizeof( errorbuf ));
      w->info_display( errorbuf );
      w->savename[0] = 0;
      return FALSE;
    }
  } else if( !*w->savename && w->save ) {
    // Closing a save stream.
    fclose( w->save );
    w->save = NULL;
  }

  if( w->save ) {
    if( fwrite( buffer, size, count, w->save ) < count ) {
      fclose( w->save );
      w->save = NULL;
      strlcpy( errorbuf, "Error writing to stream save file:\n", sizeof( errorbuf ));
      strlcat( errorbuf, w->savename, sizeof( errorbuf ));
      strlcat( errorbuf, "\n", sizeof( errorbuf ));
      strlcat( errorbuf, strerror( errno ), sizeof( errorbuf ));
      w->info_display( errorbuf );
      w->savename[0] = 0;
      return FALSE;
    }
  }
  return TRUE;
}

/* Writes the specified MPEG header to the output file. Returns TRUE
   if it successfully writes the header, or FALSE if any errors were
   detected. */
static BOOL
save_header( unsigned long header, DECODER_STRUCT* w )
{
  unsigned char buf[4];

  buf[0] = ( header >> 24 ) & 0xFF;
  buf[1] = ( header >> 16 ) & 0xFF;
  buf[2] = ( header >>  8 ) & 0xFF;
  buf[3] = ( header       ) & 0xFF;

  return save_data( buf, 4, 1, w );
}

/* Reads a MPEG header from the input file. Returns TRUE if it
   successfully reads a header, or FALSE if any errors were detected. */
BOOL
read_header( DECODER_STRUCT* w, unsigned long* header )
{
  unsigned char buffer[4];

  if( xio_fread( buffer, 1, 4, w->file ) != 4 ) {
    return FALSE;
  }

  *header = ((unsigned long)buffer[0] << 24) |
            ((unsigned long)buffer[1] << 16) |
            ((unsigned long)buffer[2] <<  8) |
             (unsigned long)buffer[3];

  return TRUE;
}


BOOL
read_next_header( DECODER_STRUCT* w, unsigned long* header )
{
  unsigned char byte;

  if( !xio_fread( &byte, 1, 1, w->file )) {
    return FALSE;
  }

  *header = (( *header << 8 ) | byte ) & 0xFFFFFFFFUL;
  return TRUE;
}


/* Returns TRUE if a specified header is the valid MPEG header. */
BOOL
is_header_valid( unsigned long header )
{
  // First 11 bits are set to 1 for frame sync.
  if(( header & 0xFFE00000 ) != 0xFFE00000 ) {
    return FALSE;
  }
  // Layer: 01,10,11 is 1,2,3; 00 is reserved.
  if(!(( header >> 17 ) & 0x3 )) {
    return FALSE;
  }
  // 1111 means bad bitrate.
  if( (( header >> 12 ) & 0xF ) == 0xF ) {
    return FALSE;
  }
  // 0000 means free format.
  if( (( header >> 12 ) & 0xF ) == 0x0 ) {
    return FALSE;
  }
  // Sampling freq: 11 is reserved.
  if( (( header >> 10 ) & 0x3 ) == 0x3 ) {
    return FALSE;
  }
  return TRUE;
}

/* Decodes a header and writes the decoded information
   into the frame structure. Returns TRUE if it successfully
   decodes a header, or FALSE if any errors were detected. */
BOOL
decode_header( DECODER_STRUCT* w, int header, struct frame* fr )
{
  if( !is_header_valid( header )) {
    return FALSE;
  }

  if( header & ( 1 << 20 )) {
    fr->lsf    = ( header & ( 1 << 19 )) ? 0x0 : 0x1;
    fr->mpeg25 = 0;
  } else {
    fr->lsf    = 1;
    fr->mpeg25 = 1;
  }

  fr->lay = 4 - (( header >> 17 ) & 3 );
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
  fr->stereo           = ( fr->mode == MPG_MD_MONO ) ? 1 : 2;

  if( !fr->bitrate_index ) {
    if( w->error_display ) {
      w->error_display( "Free format is not supported." );
    }
    return FALSE;
  }

  switch( fr->lay )
  {
    case 1:
      fr->do_layer   = do_layer1;

      fr->jsbound    = fr->mode == MPG_MD_JOINT_STEREO ? ( fr->mode_ext << 2 ) + 4 : 32;
      fr->framesize  = (long)tabsel_123[fr->lsf][0][fr->bitrate_index] * 12000;
      fr->framesize /= freqs[fr->sampling_frequency];
      fr->framesize  = (( fr->framesize + fr->padding ) << 2 ) - 4;
      fr->ssize      = 0;
      break;

    case 2:
      fr->do_layer   = do_layer2;

      get_II_stuff( fr );

      fr->jsbound    = fr->mode == MPG_MD_JOINT_STEREO ? ( fr->mode_ext << 2 ) + 4 : fr->II_sblimit;
      fr->framesize  = (long)tabsel_123[fr->lsf][1][fr->bitrate_index] * 144000;
      fr->framesize /= freqs[fr->sampling_frequency];
      fr->framesize  = fr->framesize + fr->padding - 4;
      fr->ssize      = 0;
      break;

    case 3:
      fr->do_layer = do_layer3;

      if( fr->lsf ) {
        fr->ssize = (fr->stereo == 1) ?  9 : 17;
      } else {
        fr->ssize = (fr->stereo == 1) ? 17 : 32;
      }

      if( fr->error_protection ) {
        fr->ssize += 2;
      }
      fr->framesize  = (long)tabsel_123[fr->lsf][2][fr->bitrate_index] * 144000;
      fr->framesize /= (freqs[fr->sampling_frequency] << (fr->lsf));
      fr->framesize  = fr->framesize + fr->padding - 4;
      break;

    default:
      fr->framesize = 0;
  }

  if( fr->framesize > MAXFRAMESIZE ) {
    return FALSE;
  }

  return TRUE;
}

/* This function must be called after opening a new played file
   and before first call of the read_frame function. */
BOOL
read_initialize( DECODER_STRUCT* w, struct frame* fr ) {
  return read_first_frame_header( w, fr, &firsthead );
}

/* Skips a ID3v2 tag's data and reads a next MPEG header from the
   input file. Returns TRUE if it successfully reads a header, or
   FALSE if any errors were detected. */
static BOOL
skip_id3v2_tag( DECODER_STRUCT* w, unsigned long* header )
{
  unsigned long size;
  char* buffer;

  if( !read_next_header( w, header )) {
    return FALSE;
  }

  // Version or revision will never be 0xFF.
  if((( *header & 0x0000FF00 ) == 0x0000FF00 ) ||
     (( *header & 0x000000FF ) == 0x000000FF ))
  {
    return TRUE;
  }

  // Skips flags.
  if( !read_next_header( w, header )) {
    return FALSE;
  }

  if( !read_header( w, header )) {
    return FALSE;
  }

  // Must be a 32 bit synchsafe integer.
  if( *header & 0x80808080UL ) {
    return TRUE;
  }

  size = (( *header & 0xFF000000UL ) >> 3 ) |
         (( *header & 0x00FF0000UL ) >> 2 ) |
         (( *header & 0x0000FF00UL ) >> 1 ) |
          ( *header & 0x000000FFUL );

  if(!( buffer = malloc( size ))) {
    return FALSE;
  }

  if( xio_fread( buffer, 1, size, w->file ) != size ) {
    free( buffer );
    return FALSE;
  }

  free( buffer );
  DEBUGLOG(( "mpg123: skipped %d bytes of the ID3v2 tag up to %d.\n", size + 10, xio_ftell( w->file )));
  return read_header( w, header );
}

/* Searches a first frame header and decodes it. After completion of
   this function a stream pointer can point to header of the
   following frame. Do not use the received results for reading
   the frame! */
BOOL
read_first_frame_header( DECODER_STRUCT* w, struct  frame* fr,
                                            unsigned long* header )
{
  int read_ahead = xio_can_seek( w->file ) < 2 ? 2 : 32;
  int done_ahead;
  int back_ahead;
  int resync = 0;

  if( !read_header( w, header )) {
    return FALSE;
  }

  // I even saw RIFF headers at the beginning of MPEG streams ;(
  if( IS_4CH_TAG( *header, 'R','I','F','F' )) {
    while( !IS_4CH_TAG( *header, 'd','a','t','a' )) {
      if( !read_next_header( w, header )) {
        return FALSE;
      }
      if( ++resync > MAXRESYNC ) {
        // Giving up searching valid MPEG header.
        errno = 200;
        return FALSE;
      }
    }
    if( !read_header( w, header )) {
      return FALSE;
    }
    DEBUGLOG(( "mpg123: skipped RIFF header up to %d.\n", xio_ftell( w->file )));
  }

  while( !decode_header( w, *header, fr ))
  {

read_again:

    // Step in byte steps through next data.
    while( ++resync < MAXRESYNC ) {
      if( IS_3CH_TAG( *header, 'I','D','3' )) {
        if( !skip_id3v2_tag( w, header )) {
          return FALSE;
        } else {
          if( is_header_valid( *header )) {
            break;
          }
        }
      }
      if( !read_next_header( w, header )) {
        return FALSE;
      }
      if( is_header_valid( *header )) {
        break;
      }
    }
    if( resync > MAXRESYNC ) {
      // Giving up searching valid MPEG header.
      DEBUGLOG(( "mpg123: giving up searching valid MPEG header...\n" ));
      errno = 200;
      return FALSE;
    }
  }

  back_ahead = 0;
  done_ahead = 0;
  w->xing_header.flags = 0;

  if( fr->lay == 3 )
  {
    // Let's try to find a XING VBR header.
    char buf[MAXFRAMESIZE];

    buf[0] = ( *header >> 24 ) & 0xFF;
    buf[1] = ( *header >> 16 ) & 0xFF;
    buf[2] = ( *header >>  8 ) & 0xFF;
    buf[3] = ( *header       ) & 0xFF;

    if( xio_fread( buf + 4, 1, fr->framesize, w->file ) != fr->framesize ) {
      return FALSE;
    }

    back_ahead -= fr->framesize;
    done_ahead++;

    if( GetXingHeader( &w->xing_header, buf )) {
      if( w->xing_header.flags && !( w->xing_header.flags & BYTES_FLAG ))
      {
        w->xing_header.bytes  = xio_fsize( w->file ) - xio_ftell( w->file ) - back_ahead + 4;
        w->xing_header.flags |= BYTES_FLAG;
      }
      // If there was a XING VBR header, it means precisely a mp3 file and
      // we do not need to look ahead.
      DEBUGLOG(( "mpg123: found a valid XING header.\n" ));
      goto done;
    } else {
      unsigned long next;

      // To pass a first step of a look ahead.
      if( !read_header( w, &next )) {
        return FALSE;
      }

      back_ahead -= 4;

      if(( next & HDRCMPMASK ) != ( *header & HDRCMPMASK ) ||
          !decode_header( w, next, fr ))
      {
        xio_fseek( w->file, back_ahead, XIO_SEEK_CUR );
        goto read_again;
      }
    }
  }

  for( ; done_ahead < read_ahead; done_ahead++ )
  {
    unsigned long next;
    char data[MAXFRAMESIZE];

    if( xio_fread( data, 1, fr->framesize, w->file ) == fr->framesize &&
        read_header( w, &next ))
    {
      back_ahead -= ( fr->framesize + 4 );

      if(( next & HDRCMPMASK ) != ( *header & HDRCMPMASK ) ||
          !decode_header( w, next, fr ))
      {
        xio_fseek( w->file, back_ahead, XIO_SEEK_CUR );
        goto read_again;
      }
    } else {
      return FALSE;
    }
  }

done:

  w->started = xio_ftell( w->file ) + back_ahead - 4;
  // xio_fseek( w->file, back_ahead - 4, XIO_SEEK_CUR );
  xio_fseek( w->file, w->started, XIO_SEEK_SET );
  // Restore a first frame broken by previous tests.
  decode_header( w, *header, fr );

  // For VBR event.
  w->obr = tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index];
  // For position calculation.
  w->abr = tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index];

  DEBUGLOG(( "mpg123: data stream is started at %d.\n", xio_ftell( w->file )));
  return TRUE;
}

/* Reads a next valid header and decodes it. Returns TRUE if it
   successfully reads a header, or FALSE if any errors were detected. */
static BOOL
read_synchronize( DECODER_STRUCT* w, struct  frame* fr,
                                     unsigned long* header )
{
  int resync = 0;

  if( !read_header( w, header )) {
    return FALSE;
  }

  while(( *header & HDRCMPMASK ) != ( firsthead & HDRCMPMASK )
       || !decode_header( w, *header, fr ))
  {
    if( IS_3CH_TAG( *header, 'T','A','G' )
        && ( xio_fsize( w->file ) - xio_ftell( w->file ) == 124 ))
    {
      // Found ID3v1 tag.
      DEBUGLOG(( "mpg123: skipped ID3v1 tag.\n" ));
      return FALSE;
    }
    if( IS_3CH_TAG( *header, 'I','D','3' )) {
      if( !skip_id3v2_tag( w, header )) {
        return FALSE;
      } else {
        continue;
      }
    }
    if( !read_next_header( w, header )) {
      return FALSE;
    }
    if( ++resync > MAXRESYNC ) {
      return FALSE;
    }
  }

  if( resync ) {
    bsbuf[0].size = 0;
    bsbuf[1].size = 0;
    DEBUGLOG(( "mpg123: stream is resynced after %d attempts.\n", resync ));
  }

  return TRUE;
}

/* Reads a next valid frame and decodes it. Returns TRUE if it
   successfully reads a frame, or FALSE if any errors were detected. */
BOOL
read_frame( DECODER_STRUCT* w, struct frame* fr )
{
  unsigned long header;
  int done;

  if( !read_synchronize( w, fr, &header )) {
    return FALSE;
  }

  // Position from the beginning of the data stream.
  fr->filepos = xio_ftell( w->file ) - 4 - w->started;

  bsnum ^= 1;

  if(( done = xio_fread( bsbuf[bsnum].data, 1, fr->framesize, w->file )) != fr->framesize ) {
    if( done <= 0 ) {
      return FALSE;
    }

    memset( bsbuf[bsnum].data + done, 0, fr->framesize - done );
  }

  save_header( header, w );
  save_data( bsbuf[bsnum].data, 1, fr->framesize, w );

  bitindex = 0;
  wordpointer = bsbuf[bsnum].data;
  bsbuf[bsnum].main_data = bsbuf[bsnum].data + fr->ssize;
  bsbuf[bsnum].size = fr->framesize - fr->ssize;

  if( tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index] != w->obr )
  {
     w->obr = tabsel_123[fr->lsf][fr->lay-1][fr->bitrate_index];
     WinPostMsg( w->hwnd, WM_CHANGEBR, MPFROMLONG( w->obr ), 0 );
  }

  return TRUE;
}

/* Changes the current file position to a new location within the file
   and synchronize the data stream. Returns TRUE if it successfully
   moves the pointer. */
BOOL
seekto_pos( DECODER_STRUCT* w, struct frame* fr, int seektobytes )
{
  unsigned long header;

  if( !xio_can_seek( w->file )) {
    // This only works on files.
    return FALSE;
  }

  if( fr->lay == 3 ) {
    seektobytes -= 2 * fr->framesize + 8;
    seektobytes  = max( w->started, seektobytes );
  }

  if( xio_fseek( w->file, seektobytes, XIO_SEEK_SET ) < 0 ) {
    return FALSE;
  }

  if( read_synchronize( w, fr, &header )) {
    if( xio_fseek( w->file, -4, XIO_SEEK_CUR ) == 0 )
    {
      // Clears the frames buffer.
      bsbuf[0].size = 0;
      bsbuf[1].size = 0;

      if( fr->lay == 3 ) {
        read_frame( w, fr );
        read_frame( w, fr );
        complete_main_data( 512, fr );
      }
      return TRUE;
    }
  }

  return FALSE;
}

unsigned int
getbits( int number_of_bits )
{
  unsigned long rval;

  #ifdef DEBUG_GETBITS
  fprintf( stderr,"g%d", number_of_bits );
  #endif

  if( !number_of_bits ) {
    return 0;
  }

  rval = wordpointer[0];
  rval <<= 8;
  rval |= wordpointer[1];
  rval <<= 8;
  rval |= wordpointer[2];
  rval <<= bitindex;
  rval &= 0xffffff;

  bitindex += number_of_bits;
  rval >>= ( 24 - number_of_bits );
  wordpointer += ( bitindex >> 3 );
  bitindex &= 7;

  #ifdef DEBUG_GETBITS
  fprintf( stderr ,":%x ", rval );
  #endif
  return rval;
}

unsigned int
getbits_fast( int number_of_bits )
{
  unsigned long rval;

  #ifdef DEBUG_GETBITS
  fprintf( stderr,"g%d", number_of_bits );
  #endif

  rval = wordpointer[0];
  rval <<= 8;
  rval |= wordpointer[1];
  rval <<= bitindex;
  rval &= 0xffff;

  bitindex += number_of_bits;
  rval >>= ( 16 - number_of_bits );
  wordpointer += ( bitindex >> 3 );
  bitindex &= 7;

  #ifdef DEBUG_GETBITS
  fprintf( stderr, ":%x ", rval );
  #endif

  return rval;
}

unsigned int
get1bit( void )
{
  unsigned char rval;

  #ifdef DEBUG_GETBITS
  fprintf( stderr, "g%d", 1 );
  #endif

  rval = *wordpointer << bitindex;

  bitindex++;
  wordpointer += ( bitindex >> 3 );
  bitindex &= 7;

  #ifdef DEBUG_GETBITS
  fprintf( stderr, ":%d ", rval >> 7 );
  #endif
  return rval >> 7;
}

BOOL
complete_main_data( unsigned int backstep, struct frame* fr )
{
  int  bsold = bsnum ^ 1;
  BOOL rc = TRUE;

  if( bsbuf[bsold].size < backstep ) {
    DEBUGLOG(( "mpg123: can't copy %d bytes of the main data from %d previous bytes\n",
                                                        backstep, bsbuf[bsold].size ));
    // If the previous frame has no enough data for a
    // main data completion then copy data as much as
    // possible.
    backstep = bsbuf[bsold].size;
    rc = FALSE;
  }

  bsbuf[bsnum].main_data -= backstep;
  bsbuf[bsnum].size      += backstep;

  if( backstep ) {
    memcpy( bsbuf[bsnum].main_data, bsbuf[bsold].main_data + bsbuf[bsold].size - backstep, backstep );
  }

  wordpointer = bsbuf[bsnum].main_data;
  bitindex = 0;
  return rc;
}

void
clear_decoder( void )
{
  // Clears synth.
  real crap [SBLIMIT  ];
  char crap2[SBLIMIT*4];
  int  i;

  for( i = 0; i < 16; i++ )
  {
    memset( crap, 0, sizeof( crap ));
    synth_1to1( crap, 0, crap2 );
    memset( crap, 0, sizeof( crap ));
    synth_1to1( crap, 1, crap2 );
  }

  clear_layer3();

  // Clears the frame buffer.
  bsbuf[0].size = 0;
  bsbuf[1].size = 0;
}

