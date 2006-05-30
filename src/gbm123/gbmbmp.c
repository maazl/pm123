/*

gbmbmp.c - OS/2 1.1, 1.2, 2.0 and Windows 3.0 support

Reads and writes any OS/2 1.x bitmap.
Will also read uncompressed, RLE4, RLE8 and RLE24 OS/2 2.0 and Windows 3.x bitmaps too.
There are horrific file structure alignment considerations hence each
word,dword is read individually.
Input options: index=# (default: 0)

History:
--------
(Heiko Nitzsche)

19-Feb-2006: Add function to query number of images
21-Feb-2006: Add support for reading RLE24 encoded bitmaps
22-Feb-2006: Move format description strings to gbmdesc.h

*/

#include <stdio.h>
#include <ctype.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "gbm.h"
#include "gbmhelp.h"
#include "gbmdesc.h"

#ifndef min
  #define min(a,b)  (((a)<(b))?(a):(b))
#endif

#define  low_byte(w)    ((byte)  ((w)&0x00ff)    )
#define  high_byte(w)   ((byte) (((w)&0xff00)>>8))
#define  make_word(a,b) (((word)a) + (((word)b) << 8))

/* ------------------------------------------------- */

static BOOLEAN read_word(int fd, word *w)
{
    byte low = 0, high = 0;

    if ( gbm_file_read(fd, (char *) &low, 1) != 1 )
        return FALSE;
    if ( gbm_file_read(fd, (char *) &high, 1) != 1 )
        return FALSE;
    *w = (word) (low + ((word) high << 8));
    return TRUE;
}

static BOOLEAN read_dword(int fd, dword *d)
{
    word low, high;
    if ( !read_word(fd, &low) )
        return FALSE;
    if ( !read_word(fd, &high) )
        return FALSE;
    *d = low + ((dword) high << 16);
    return TRUE;
}

static BOOLEAN write_word(int fd, word w)
{
    byte    low  = (byte) w;
    byte    high = (byte) (w >> 8);

    gbm_file_write(fd, &low, 1);
    gbm_file_write(fd, &high, 1);
    return TRUE;
}

static BOOLEAN write_dword(int fd, dword d)
{
    write_word(fd, (word) d);
    write_word(fd, (word) (d >> 16));
    return TRUE;
}

static GBMFT bmp_gbmft =
{
    GBM_FMT_DESC_SHORT_BMP,
    GBM_FMT_DESC_LONG_BMP,
    GBM_FMT_DESC_EXT_BMP,
    GBM_FT_R1|GBM_FT_R4|GBM_FT_R8|GBM_FT_R24|
    GBM_FT_W1|GBM_FT_W4|GBM_FT_W8|GBM_FT_W24,
};

#define GBM_ERR_BMP_PLANES    ((GBM_ERR) 300)
#define GBM_ERR_BMP_BITCOUNT  ((GBM_ERR) 301)
#define GBM_ERR_BMP_CBFIX     ((GBM_ERR) 302)
#define GBM_ERR_BMP_COMP      ((GBM_ERR) 303)
#define GBM_ERR_BMP_OFFSET    ((GBM_ERR) 304)

/* ------------------------------------------------- */

typedef struct
{
    dword base;
    BOOLEAN windows;
    dword cbFix;
    dword ulCompression;
    dword cclrUsed;
    dword offBits;
    BOOLEAN inv, invb;
} BMP_PRIV;

#define BFT_BMAP          0x4d42
#define BFT_BITMAPARRAY   0x4142
#define BCA_UNCOMP        0x00000000L /* BI_RGB  */
#define BCA_RLE8          0x00000001L /* BI_RLE8 */
#define BCA_RLE4          0x00000002L /* BI_RLE4 */
#define BCA_RLE24         0x00000004L /* seems to be OS/2 only */

#if 0 /* not yet supported */
#define BCA_HUFFFMAN1D    0x00000003L /* blackwhite only, same code as BI_BITFIELDS  , take care for detection */
#define BI_BITFIELDS      0x00000003L /* color only     , same code as BCA_HUFFFMAN1D, take care for detection */
#endif

#define MSWCC_EOL         0
#define MSWCC_EOB         1
#define MSWCC_DELTA       2

static void invert(byte *buffer, unsigned count)
{
    while ( count-- )
        *buffer++ ^= (byte) 0xffU;
}

static void swap_pal(GBMRGB *gbmrgb)
{
    GBMRGB tmp = gbmrgb[0];
    gbmrgb[0] = gbmrgb[1];
    gbmrgb[1] = tmp;
}

GBM_ERR bmp_qft(GBMFT *gbmft)
{
    *gbmft = bmp_gbmft;
    return GBM_ERR_OK;
}

/* ------------------------------------------------- */

/* Read number of bitmaps in BMP file. */
GBM_ERR bmp_rimgcnt(const char *fn, int fd, int *pimgcnt)
{
    word usType;
    fn=fn; /* Suppress 'unref arg' compiler warnings */

    if ( !read_word(fd, &usType) )
    {
        return GBM_ERR_READ;
    }
    if ( usType == BFT_BITMAPARRAY )
    {
        int i;
        for(i = 1;;i++)
        {
            dword cbSize2, offNext;
            
            if ( !read_dword(fd, &cbSize2) ) break;
            if ( !read_dword(fd, &offNext) ) break;
            if ( offNext == 0L ) break;

            gbm_file_lseek(fd, (long) offNext, GBM_SEEK_SET);

            if ( !read_word(fd, &usType) ) break;
            if ( usType != BFT_BITMAPARRAY ) break;
        }
        *pimgcnt = i;
    }
    else
    {
       *pimgcnt = 1;
    }

    return GBM_ERR_OK;
}

/* ------------------------------------------------- */

GBM_ERR bmp_rhdr(const char *fn, int fd, GBM *gbm, const char *opt)
{
    word usType, xHotspot, yHotspot;
    dword cbSize, offBits, cbFix;
    BMP_PRIV *bmp_priv = (BMP_PRIV *) gbm->priv;
    bmp_priv->inv  = ( gbm_find_word(opt, "inv" ) != NULL );
    bmp_priv->invb = ( gbm_find_word(opt, "invb") != NULL );

    fn=fn; /* Suppress 'unref arg' compiler warnings */

    if ( !read_word(fd, &usType) )
        return GBM_ERR_READ;
    if ( usType == BFT_BITMAPARRAY )
    /*...shandle bitmap arrays:16:*/
    {
       const char *index;
       int i;

       if ( (index = gbm_find_word_prefix(opt, "index=")) != NULL )
          sscanf(index + 6, "%d", &i);
       else
          i = 0;

       while ( i-- > 0 )
       {
          dword cbSize2, offNext;

          if ( !read_dword(fd, &cbSize2) )
             return GBM_ERR_READ;
          if ( !read_dword(fd, &offNext) )
             return GBM_ERR_READ;
          if ( offNext == 0L )
             return GBM_ERR_BMP_OFFSET;
          gbm_file_lseek(fd, (long) offNext, GBM_SEEK_SET);
          if ( !read_word(fd, &usType) )
             return GBM_ERR_READ;
          if ( usType != BFT_BITMAPARRAY )
             return GBM_ERR_BAD_MAGIC;
       }
       gbm_file_lseek(fd, 4L + 4L + 2L + 2L, GBM_SEEK_CUR);
       if ( !read_word(fd, &usType) )
          return GBM_ERR_READ;
    }

    if ( usType != BFT_BMAP )
        return GBM_ERR_BAD_MAGIC;

    bmp_priv->base = (dword) ( gbm_file_lseek(fd, 0L, GBM_SEEK_CUR) - 2L );

    if ( !read_dword(fd, &cbSize) )
        return GBM_ERR_READ;
    if ( !read_word(fd, &xHotspot) )
        return GBM_ERR_READ;
    if ( !read_word(fd, &yHotspot) )
        return GBM_ERR_READ;
    if ( !read_dword(fd, &offBits) )
        return GBM_ERR_READ;
    if ( !read_dword(fd, &cbFix) )
        return GBM_ERR_READ;

    bmp_priv->offBits = offBits;

    if ( cbFix == 12 )
    /* OS/2 1.x uncompressed bitmap */
    {
       word cx, cy, cPlanes, cBitCount;

       if ( !read_word(fd, &cx) )
          return GBM_ERR_READ;
       if ( !read_word(fd, &cy) )
          return GBM_ERR_READ;
       if ( !read_word(fd, &cPlanes) )
          return GBM_ERR_READ;
       if ( !read_word(fd, &cBitCount) )
          return GBM_ERR_READ;

       if ( cx == 0 || cy == 0 )
          return GBM_ERR_BAD_SIZE;
       if ( cPlanes != 1 )
          return GBM_ERR_BMP_PLANES;
       if ( cBitCount != 1 && cBitCount != 4 && cBitCount != 8 && cBitCount != 24 )
          return GBM_ERR_BMP_BITCOUNT;

       gbm->w   = (int) cx;
       gbm->h   = (int) cy;
       gbm->bpp = (int) cBitCount;

       bmp_priv->windows = FALSE;
    }
    else if ( cbFix >= 16 && cbFix <= 64 &&
              ((cbFix & 3) == 0 || cbFix == 42 || cbFix == 46) )
    /*...sOS\47\2 2\46\0 and Windows 3\46\0:16:*/
    {
       word cPlanes, cBitCount, usUnits, usReserved, usRecording, usRendering;
       dword ulWidth, ulHeight, ulCompression;
       dword ulSizeImage, ulXPelsPerMeter, ulYPelsPerMeter;
       dword cclrUsed, cclrImportant, cSize1, cSize2, ulColorEncoding, ulIdentifier;
       BOOLEAN ok;

       ok  = read_dword(fd, &ulWidth);
       ok &= read_dword(fd, &ulHeight);
       ok &= read_word(fd, &cPlanes);
       ok &= read_word(fd, &cBitCount);
       if ( cbFix > 16 )
          ok &= read_dword(fd, &ulCompression);
       else
          ulCompression = BCA_UNCOMP;
       if ( cbFix > 20 )
          ok &= read_dword(fd, &ulSizeImage);
       if ( cbFix > 24 )
          ok &= read_dword(fd, &ulXPelsPerMeter);
       if ( cbFix > 28 )
          ok &= read_dword(fd, &ulYPelsPerMeter);
       if ( cbFix > 32 )
          ok &= read_dword(fd, &cclrUsed);
       else
          cclrUsed = ( (dword)1 << cBitCount );
       if ( cBitCount != 24 && cclrUsed == 0 )
          cclrUsed = ( (dword)1 << cBitCount );

       /* Protect against badly written bitmaps! */
       if ( cclrUsed > ( (dword)1 << cBitCount ) )
          cclrUsed = ( (dword)1 << cBitCount );

       if ( cbFix > 36 )
          ok &= read_dword(fd, &cclrImportant);
       if ( cbFix > 40 )
          ok &= read_word(fd, &usUnits);
       if ( cbFix > 42 )
          ok &= read_word(fd, &usReserved);
       if ( cbFix > 44 )
          ok &= read_word(fd, &usRecording);
       if ( cbFix > 46 )
          ok &= read_word(fd, &usRendering);
       if ( cbFix > 48 )
          ok &= read_dword(fd, &cSize1);
       if ( cbFix > 52 )
          ok &= read_dword(fd, &cSize2);
       if ( cbFix > 56 )
          ok &= read_dword(fd, &ulColorEncoding);
       if ( cbFix > 60 )
          ok &= read_dword(fd, &ulIdentifier);

       if ( !ok )
          return GBM_ERR_READ;

       if ( ulWidth == 0L || ulHeight == 0L )
          return GBM_ERR_BAD_SIZE;
       if ( cPlanes != 1 )
          return GBM_ERR_BMP_PLANES;
       if ( cBitCount != 1 && cBitCount != 4 && cBitCount != 8 && cBitCount != 24 )
          return GBM_ERR_BMP_BITCOUNT;

       gbm->w   = (int) ulWidth;
       gbm->h   = (int) ulHeight;
       gbm->bpp = (int) cBitCount;

       bmp_priv->windows       = TRUE;
       bmp_priv->cbFix         = cbFix;
       bmp_priv->ulCompression = ulCompression;
       bmp_priv->cclrUsed      = cclrUsed;
    }
    else
       return GBM_ERR_BMP_CBFIX;

    return GBM_ERR_OK;
}

/* ------------------------------------------------- */

GBM_ERR bmp_rpal(int fd, GBM *gbm, GBMRGB *gbmrgb)
{
    BMP_PRIV *bmp_priv = (BMP_PRIV *) gbm->priv;

    if ( gbm->bpp != 24 )
    {
        int i;
        byte b[4];

        if ( bmp_priv->windows )
        {
           gbm_file_lseek(fd, (long) (bmp_priv->base + 14L + bmp_priv->cbFix), GBM_SEEK_SET);
           for ( i = 0; i < (1 << gbm->bpp); i++ )
              /* Used to use bmp_priv->cclrUsed, but bitmaps have been found which have
                 their used colours not at the start of the palette. */
           {
              gbm_file_read(fd, b, 4);
              gbmrgb[i].b = b[0];
              gbmrgb[i].g = b[1];
              gbmrgb[i].r = b[2];
           }
        }
        else
        {
           gbm_file_lseek(fd, (long) (bmp_priv->base + 26L), GBM_SEEK_SET);
           for ( i = 0; i < (1 << gbm->bpp); i++ )
           {
              gbm_file_read(fd, b, 3);
              gbmrgb[i].b = b[0];
              gbmrgb[i].g = b[1];
              gbmrgb[i].r = b[2];
           }
        }
    }

    if ( gbm->bpp == 1 && !bmp_priv->inv )
        swap_pal(gbmrgb);

    return GBM_ERR_OK;
}

/* ------------------------------------------------- */

GBM_ERR bmp_rdata(int fd, GBM *gbm, byte *data)
{
    BMP_PRIV *bmp_priv = (BMP_PRIV *) gbm->priv;
    const int cLinesWorth = ((gbm->bpp * gbm->w + 31) / 32) * 4;

    if ( bmp_priv->windows )
    /* OS/2 2.0 and Windows 3.0 */
    {
       gbm_file_lseek(fd, (long)bmp_priv->offBits, GBM_SEEK_SET);

       switch ( (int) bmp_priv->ulCompression )
       {
          /* BCA_UNCOMP */
          case BCA_UNCOMP:
             gbm_file_read(fd, data, gbm->h * cLinesWorth);
             break;

          /* ---------------------------------------------------- */

          /* BCA_RLE24 */
          case BCA_RLE24:
          {
             AHEAD *ahead;
             int x = 0, y = 0;
             BOOLEAN eof24 = FALSE;

             if ( (ahead = gbm_create_ahead(fd)) == NULL )
                return GBM_ERR_MEM;

             while ( !eof24 )
             {
                const byte c = (byte) gbm_read_ahead(ahead);
                const byte d = (byte) gbm_read_ahead(ahead);
                /* fprintf(stderr, "(%d,%d) c=%d,d=%d\n", x, y, c, d); */

                if ( c )
                {
                   /* encoded run */
                   const byte g = (byte) gbm_read_ahead(ahead);
                   const byte r = (byte) gbm_read_ahead(ahead);

                   int i;
                   for (i = 0; i < (int) c; i++)
                   {
                      *data++ = d; /* b */
                      *data++ = g;
                      *data++ = r;
                      x += 3;
                   }
                }
                else
                {
                   /* unencoded run */
                   switch ( d )
                   {
                      /* MSWCC_EOL */
                      case MSWCC_EOL:
                      {
                         const int to_eol = cLinesWorth - x;
                         memset(data, 0, (size_t) to_eol);
                         data += to_eol;
                         x = 0;
                         if ( ++y == gbm->h )
                         {
                            eof24 = TRUE;
                         }
                      }
                      break;

                      /* MSWCC_EOB */
                      case MSWCC_EOB:
                         if ( y < gbm->h )
                         {
                            const int to_eol = cLinesWorth - x;

                            memset(data, 0, (size_t) to_eol);
                            x = 0; y++;
                            data += to_eol;
                            while ( y < gbm->h )
                            {
                               memset(data, 0, (size_t) cLinesWorth);
                               data += cLinesWorth;
                               y++;
                            }
                         }
                         eof24 = TRUE;
                         break;

                      /* MSWCC_DELTA */
                      case MSWCC_DELTA:
                      {
                         const byte dx  = (byte) gbm_read_ahead(ahead);
                         const byte dy  = (byte) gbm_read_ahead(ahead);
                         const int fill = (dx * 3) + (dy * cLinesWorth);
                         memset(data, 0, (size_t) fill);
                         data += fill;
                         x += dx * 3; y += dy;
                         if ( y == gbm->h )
                         {
                            eof24 = TRUE;
                         }
                      }
                      break;

                      /* default */
                      default:
                      {
                         int n = (int) d;

                         while ( n-- > 0 )
                         {
                            *data++ = (byte) gbm_read_ahead(ahead);
                            *data++ = (byte) gbm_read_ahead(ahead);
                            *data++ = (byte) gbm_read_ahead(ahead);
                            x += 3;
                         }
                         if ( x > (gbm->w * 3) ) { x = 0; ++y; } /* @@@AK */
                         if ( d & 1 )
                         {
                            gbm_read_ahead(ahead); /* Align */
                         }
                      }
                      break;
                   }
                }
             }

             gbm_destroy_ahead(ahead);
          }
          break;

          /* ---------------------------------------------------- */

          /* BCA_RLE8 */
          case BCA_RLE8:
          {
             AHEAD *ahead;
             int x = 0, y = 0;
             BOOLEAN eof8 = FALSE;

             if ( (ahead = gbm_create_ahead(fd)) == NULL )
                return GBM_ERR_MEM;

             while ( !eof8 )
             {
                const byte c = (byte) gbm_read_ahead(ahead);
                const byte d = (byte) gbm_read_ahead(ahead);
                /* fprintf(stderr, "(%d,%d) c=%d,d=%d\n", x, y, c, d); debug*/

                if ( c )
                {
                   memset(data, d, c);
                   x += c;
                   data += c;
                }
                else
                   switch ( d )
                   {
                      /* MSWCC_EOL */
                      case MSWCC_EOL:
                      {
                         const int to_eol = cLinesWorth - x;

                         memset(data, 0, (size_t) to_eol);
                         data += to_eol;
                         x = 0;
                         if ( ++y == gbm->h )
                            eof8 = TRUE;
                      }
                      break;

                      /* MSWCC_EOB */
                      case MSWCC_EOB:
                         if ( y < gbm->h )
                         {
                            const int to_eol = cLinesWorth - x;

                            memset(data, 0, (size_t) to_eol);
                            x = 0; y++;
                            data += to_eol;
                            while ( y < gbm->h )
                            {
                               memset(data, 0, (size_t) cLinesWorth);
                               data += cLinesWorth;
                               y++;
                            }
                         }
                         eof8 = TRUE;
                         break;

                      /* MSWCC_DELTA */
                      case MSWCC_DELTA:
                      {
                         const byte dx  = (byte) gbm_read_ahead(ahead);
                         const byte dy  = (byte) gbm_read_ahead(ahead);
                         const int fill = dx + dy * cLinesWorth;
                         memset(data, 0, (size_t) fill);
                         data += fill;
                         x += dx; y += dy;
                         if ( y == gbm->h )
                            eof8 = TRUE;
                      }
                      break;

                      /* default */
                      default:
                      {
                         int n = (int) d;

                         while ( n-- > 0 )
                            *data++ = (byte) gbm_read_ahead(ahead);
                         x += d;
                         if ( x > gbm->w ) { x = 0; ++y; } /* @@@AK */
                         if ( d & 1 )
                            gbm_read_ahead(ahead); /* Align */
                      }
                      break;
                   }
             }

             gbm_destroy_ahead(ahead);
          }
          break;

          /* ---------------------------------------------------- */

          /* BCA_RLE4 */
          case BCA_RLE4:
          {
             AHEAD *ahead;
             int x = 0, y = 0;
             BOOLEAN eof4 = FALSE;
             int inx = 0;

             if ( (ahead = gbm_create_ahead(fd)) == NULL )
                return GBM_ERR_MEM;

             memset(data, 0, (size_t) (gbm->h * cLinesWorth));

             while ( !eof4 )
             {
                const byte c = (byte) gbm_read_ahead(ahead);
                const byte d = (byte) gbm_read_ahead(ahead);

                if ( c )
                {
                   byte h, l;
                   int i;
                   if ( x & 1 )
                      { h = (byte) (d >> 4); l = (byte) (d << 4); }
                   else
                      { h = (byte) (d&0xf0); l = (byte) (d&0x0f); }

                   for ( i = 0; i < (int) c; i++, x++ )
                   {
                      if ( x & 1U )
                         data[inx++] |= l;
                      else
                         data[inx]   |= h;
                   }
                }
                else
                   switch ( d )
                   {
                      /* MSWCC_EOL */
                      case MSWCC_EOL:
                         x = 0;
                         if ( ++y == gbm->h )
                            eof4 = TRUE;
                         inx = cLinesWorth * y;
                         break;

                      /* MSWCC_EOB */
                      case MSWCC_EOB:
                         eof4 = TRUE;
                         break;

                      /* MSWCC_DELTA */   
                      case MSWCC_DELTA:
                      {
                         const byte dx = (byte) gbm_read_ahead(ahead);
                         const byte dy = (byte) gbm_read_ahead(ahead);

                         x += dx; y += dy;
                         inx = y * cLinesWorth + (x/2);
        
                         if ( y == gbm->h )
                            eof4 = TRUE;
                      }
                      break;

                      /* default */
                      default:
                      {
                         int i, nr = 0;

                         if ( x & 1 )
                         {
                            for ( i = 0; i+2 <= (int) d; i += 2 )
                            {
                               const byte b = (byte) gbm_read_ahead(ahead);
                               data[inx++] |= (b >> 4);
                               data[inx  ] |= (b << 4);
                               nr++;
                            }
                            if ( i < (int) d )
                            {
                               data[inx++] |= ((byte) gbm_read_ahead(ahead) >> 4);
                               nr++;
                            }
                         }
                         else
                         {
                            for ( i = 0; i+2 <= (int) d; i += 2 )
                            {
                               data[inx++] = (byte) gbm_read_ahead(ahead);
                               nr++;
                            }
                            if ( i < (int) d )
                            {
                               data[inx] = (byte) gbm_read_ahead(ahead);
                               nr++;
                            }
                         }
                         x += d;

                         if ( nr & 1 )
                            gbm_read_ahead(ahead); /* Align input stream to next word */
                      }
                      break;
                   }
             }

             gbm_destroy_ahead(ahead);
          }
          break;

          /* ---------------------------------------------------- */

          default:
             return GBM_ERR_BMP_COMP;
       }
    }
    else
    /* OS/2 1.1, 1.2 */
    {
       gbm_file_lseek(fd, (long) bmp_priv->offBits, GBM_SEEK_SET);
       gbm_file_read(fd, data, cLinesWorth * gbm->h);
    }

    if ( bmp_priv->invb )
        invert(data, (unsigned) (cLinesWorth * gbm->h));

    return GBM_ERR_OK;
}

/* ------------------------------------------------- */

static int bright(const GBMRGB *gbmrgb)
{
   return gbmrgb->r*30+gbmrgb->g*60+gbmrgb->b*10;
}

/* ------------------------------------------------- */

static int write_inv(int fd, const byte *buffer, int count)
{
    byte small_buf[1024];
    int so_far = 0, this_go, written;

    while ( so_far < count )
    {
        this_go = min(count - so_far, 1024);
        memcpy(small_buf, buffer + so_far, (size_t) this_go);
        invert(small_buf, (unsigned) this_go);
        if ( (written = gbm_file_write(fd, small_buf, this_go)) != this_go )
            return so_far + written;
        so_far += written;
    }

    return so_far;
}

/* ------------------------------------------------- */

GBM_ERR bmp_w(const char *fn, int fd, const GBM *gbm, const GBMRGB *gbmrgb, const byte *data, const char *opt)
{
    const BOOLEAN    pm11    = ( gbm_find_word(opt, "1.1"    ) != NULL );
    const BOOLEAN    win     = ( gbm_find_word(opt, "win"    ) != NULL ||
                                 gbm_find_word(opt, "2.0"    ) != NULL );
    const BOOLEAN inv        = ( gbm_find_word(opt, "inv"    ) != NULL );
          BOOLEAN invb       = ( gbm_find_word(opt, "invb"   ) != NULL );
    const BOOLEAN darkfg     = ( gbm_find_word(opt, "darkfg" ) != NULL );
    const BOOLEAN lightfg    = ( gbm_find_word(opt, "lightfg") != NULL );
    int cRGB;
    GBMRGB gbmrgb_1bpp[2];

    if ( pm11 && win )
        return GBM_ERR_BAD_OPTION;

    fn=fn; /* Suppress 'unref arg' compiler warning */

    cRGB = ( (1 << gbm->bpp) & 0x1ff );
        /* 1->2, 4->16, 8->256, 24->0 */

    if ( cRGB == 2 )
    /*...shandle messy 1bpp case:16:*/
    {
       /*
          The palette entries inside a 1bpp PM bitmap are not honored, or handled
          correctly by most programs. Current thinking is that they have no actual
          meaning. Under OS/2 PM, bitmap 1's re fg and 0's are bg, and it is the job of
          the displayer to pick fg and bg. We will pick fg=black, bg=white in the bitmap
          file we save. If we do not write black and white, we find that most programs
          will incorrectly honor these entries giving unpredicatable (and often black on
          a black background!) results.
       */

       gbmrgb_1bpp[0].r = gbmrgb_1bpp[0].g = gbmrgb_1bpp[0].b = 0xff;
       gbmrgb_1bpp[1].r = gbmrgb_1bpp[1].g = gbmrgb_1bpp[1].b = 0x00;

       /*
          We observe these values must be the wrong way around to keep most PM
          programs happy, such as WorkPlace Shell WPFolder backgrounds.
       */

       if ( !inv )
          swap_pal(gbmrgb_1bpp);

       /*
          If the user has picked the darkfg option, then they intend that the darkest
          colour in the image is to be the foreground. This is a very sensible option
          because the foreground will appear to be black when the image is reloaded.
          To acheive this we must invert the bitmap bits, if the palette dictates.
       */

       if ( darkfg && bright(&gbmrgb[0]) < bright(&gbmrgb[1]) )
          invb = !invb;
       if ( lightfg && bright(&gbmrgb[0]) >= bright(&gbmrgb[1]) )
          invb = !invb;

       gbmrgb = gbmrgb_1bpp;
    }
    /*...e*/

    if ( pm11 )
    /*...sOS\47\2 1\46\1:16:*/
    {
       const word usType     = BFT_BMAP;
       const word xHotspot   = 0;
       const word yHotspot   = 0;
       const dword cbFix     = (dword) 12;
       const word cx         = (word) gbm->w;
       const word cy         = (word) gbm->h;
       const word cPlanes    = (word) 1;
       const word cBitCount  = (word) gbm->bpp;
       const int cLinesWorth = (((cBitCount * cx + 31) / 32) * cPlanes) * 4;
       const dword offBits   = (dword) 26 + cRGB * (dword) 3;
       const dword cbSize    = offBits + (dword) cy * (dword) cLinesWorth;
       int i, total, actual;

       write_word(fd, usType);
       write_dword(fd, cbSize);
       write_word(fd, xHotspot);
       write_word(fd, yHotspot);
       write_dword(fd, offBits);
       write_dword(fd, cbFix);
       write_word(fd, cx);
       write_word(fd, cy);
       write_word(fd, cPlanes);
       write_word(fd, cBitCount);

       for ( i = 0; i < cRGB; i++ )
       {
          byte b[3];

          b[0] = gbmrgb[i].b;
          b[1] = gbmrgb[i].g;
          b[2] = gbmrgb[i].r;
          if ( gbm_file_write(fd, b, 3) != 3 )
             return GBM_ERR_WRITE;
       }

       total = gbm->h * cLinesWorth;
       if ( invb )
          actual = write_inv(fd, data, total);
       else
          actual = gbm_file_write(fd, data, total);
       if ( actual != total )
          return GBM_ERR_WRITE;
    }
    /*...e*/
    else
    /*...sOS\47\2 2\46\0 and Windows 3\46\0:16:*/
    {
       const word usType         = BFT_BMAP;
       const word xHotspot       = 0;
       const word yHotspot       = 0;
       const dword cbFix         = (dword) 40;
       const dword cx            = (dword) gbm->w;
       const dword cy            = (dword) gbm->h;
       const word cPlanes        = (word) 1;
       const word cBitCount      = (word) gbm->bpp;
       const int cLinesWorth     = (((cBitCount * (int) cx + 31) / 32) * cPlanes) * 4;
       const dword offBits       = (dword) 54 + cRGB * (dword) 4;
       const dword cbSize        = offBits + (dword) cy * (dword) cLinesWorth;
       const dword ulCompression = BCA_UNCOMP;
       const dword cbImage       = (dword) cLinesWorth * (dword) gbm->h;
       const dword cxResolution  = 0;
       const dword cyResolution  = 0;
       const dword cclrUsed      = 0;
       const dword cclrImportant = 0;
       int i, total, actual;

       write_word(fd, usType);
       write_dword(fd, cbSize);
       write_word(fd, xHotspot);
       write_word(fd, yHotspot);
       write_dword(fd, offBits);

       write_dword(fd, cbFix);
       write_dword(fd, cx);
       write_dword(fd, cy);
       write_word(fd, cPlanes);
       write_word(fd, cBitCount);
       write_dword(fd, ulCompression);
       write_dword(fd, cbImage);
       write_dword(fd, cxResolution);
       write_dword(fd, cyResolution);
       write_dword(fd, cclrUsed);
       write_dword(fd, cclrImportant);

       for ( i = 0; i < cRGB; i++ )
       {
          byte b[4];

          b[0] = gbmrgb[i].b;
          b[1] = gbmrgb[i].g;
          b[2] = gbmrgb[i].r;
          b[3] = 0;
          if ( gbm_file_write(fd, b, 4) != 4 )
             return GBM_ERR_WRITE;
       }

       total = gbm->h * cLinesWorth;
       if ( invb )
          actual = write_inv(fd, data, total);
       else
          actual = gbm_file_write(fd, data, total);
       if ( actual != total )
          return GBM_ERR_WRITE;
    }
    /*...e*/

    return GBM_ERR_OK;
}

/* ------------------------------------------------- */

const char *bmp_err(GBM_ERR rc)
{
    switch ( (int) rc )
    {
        case GBM_ERR_BMP_PLANES:
            return "number of bitmap planes is not 1";
        case GBM_ERR_BMP_BITCOUNT:
            return "bit count not 1, 4, 8 or 24";
        case GBM_ERR_BMP_CBFIX:
            return "cbFix bad";
        case GBM_ERR_BMP_COMP:
            return "compression type not uncompressed, RLE4, RLE8 or RLE24";
        case GBM_ERR_BMP_OFFSET:
            return "less bitmaps in file than index requested";
    }
    return NULL;
}

