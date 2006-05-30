/*

gbmppm.c - PPM format

Credit for writing this module must go to Heiko Nitzsche.
This file is just as public domain as the rest of GBM.

Supported formats and options:
------------------------------
Pixmap : Portable Pixel-map (binary P6 type) : .PPM

Standard formats (backward compatible):
  Reads 24 bpp unpalettised RGB files.
  Reads 48 bpp unpalettised RGB files and presents them as 24 bpp.

Extended formats (not backward compatible, import option ext_bpp required):
  Reads 48 bpp unpalettised RGB files and presents them as 48 bpp.

Writes 24 and 48 bpp unpalettised RGB files.


  Input:
  ------

  Can specify image within PPM file with multiple images
    Input option: index=# (default: 0)

  Can specify that non-standard GBM color depth is exported (48 bpp)
    Input option: ext_bpp (default: bpp is downsampled to 24 bpp)


History:
--------
(Heiko Nitzsche)

19-Feb-2006: Add function to query number of images
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

/* ---------------------------------------- */

#define  GBM_ERR_PPM_BAD_M    ((GBM_ERR) 200)

/* ---------------------------------------- */

static GBMFT ppm_gbmft =
{
   GBM_FMT_DESC_SHORT_PPM,
   GBM_FMT_DESC_LONG_PPM,
   GBM_FMT_DESC_EXT_PPM,
   GBM_FT_R24 | GBM_FT_R48 |
   GBM_FT_W24 | GBM_FT_W48
};


typedef struct
{
   unsigned int max_intensity;

   /* This entry will store the options provided during first header read.
    * It will keep the options for the case the header has to be reread.
    */
   char read_options[PRIV_SIZE - sizeof(int)
                               - 8 /* space for structure element padding */ ];

} PPM_PRIV_READ;


/* ---------------------------------------- */

static void rgb_bgr(const byte *p, byte *q, int n, const unsigned int max_intensity)
{
   byte r, g, b;

   if (max_intensity < 255)
   {
      while ( n-- )
      {
         r = *p++;
         g = *p++;
         b = *p++;

         *q++ = (byte) ((b * 255U) / max_intensity);
         *q++ = (byte) ((g * 255U) / max_intensity);
         *q++ = (byte) ((r * 255U) / max_intensity);
      }
   }
   else
   {
      while ( n-- )
      {
         r = *p++;
         g = *p++;
         b = *p++;

         *q++ = b;
         *q++ = g;
         *q++ = r;
      }
   }
}

static void rgb16msb_bgr(const byte * p, byte * q, int n, const unsigned int max_intensity)
{
   word r, g, b;

   word * p16 = (word *) p;

   if (max_intensity < 65535)
   {
      while ( n-- )
      {
         r = *p16++;
         g = *p16++;
         b = *p16++;

         swab((char *) &r, (char *) &r, 2);
         swab((char *) &g, (char *) &g, 2);
         swab((char *) &b, (char *) &b, 2);

         r = (r * 65535U) / max_intensity;
         g = (g * 65535U) / max_intensity;
         b = (b * 65535U) / max_intensity;

         #define CVT(x) (((x) * 255) / ((1L << 16) - 1))

         *q++ = CVT(b);
         *q++ = CVT(g);
         *q++ = CVT(r);
      }
   }
   else
   {
      while ( n-- )
      {
         r = *p16++;
         g = *p16++;
         b = *p16++;

         swab((char *) &r, (char *) &r, 2);
         swab((char *) &g, (char *) &g, 2);
         swab((char *) &b, (char *) &b, 2);

         #define CVT(x) (((x) * 255) / ((1L << 16) - 1))

         *q++ = CVT(b);
         *q++ = CVT(g);
         *q++ = CVT(r);
      }
   }
}

static void rgb16msb_bgr16lsb(const byte *p, byte *q, int n, const unsigned int max_intensity)
{
   word r, g, b;

   word * p16 = (word *) p;
   word * q16 = (word *) q;

   if (max_intensity < 65535)
   {
      while ( n-- )
      {
         r = *p16++;
         g = *p16++;
         b = *p16++;

         swab((char *) &r, (char *) &r, 2);
         swab((char *) &g, (char *) &g, 2);
         swab((char *) &b, (char *) &b, 2);

         *q16++ = (b * 65535U) / max_intensity;
         *q16++ = (g * 65535U) / max_intensity;
         *q16++ = (r * 65535U) / max_intensity;
      }
   }
   else
   {
      while ( n-- )
      {
         r = *p16++;
         g = *p16++;
         b = *p16++;

         swab((char *) &r, (char *) &r, 2);
         swab((char *) &g, (char *) &g, 2);
         swab((char *) &b, (char *) &b, 2);

         *q16++ = b;
         *q16++ = g;
         *q16++ = r;
      }
   }
}

/* ---------------------------------------- */

static byte read_byte(int fd)
{
   byte b = 0;
   gbm_file_read(fd, (char *) &b, 1);
   return b;
}

static char read_char(int fd)
{
   char c;
   while ( (c = read_byte(fd)) == '#' )
   {
      /* Discard to end of line */
      while ( (c = read_byte(fd)) != '\n' );
   }
   return c;
}

static int read_num(int fd)
{
   char c;
   int num;

   while ( isspace(c = read_char(fd)) );

   num = c - '0';
   while ( isdigit(c = read_char(fd)) )
   {
      num = num * 10 + (c - '0');
   }
   return num;
}

/* ---------------------------------------- */
/* ---------------------------------------- */

GBM_ERR ppm_qft(GBMFT *gbmft)
{
   *gbmft = ppm_gbmft;
   return GBM_ERR_OK;
}

/* ---------------------------------------- */
/* ---------------------------------------- */

static GBM_ERR read_ppm_header(int fd, int *h1, int *h2, int *w, int *h, int *m, int *data_bytes)
{
   *h1 = read_byte(fd);
   *h2 = read_byte(fd);
   if ((*h1 != 'P') || (*h2 != '6'))
   {
      return GBM_ERR_BAD_MAGIC;
   }

   *w  = read_num(fd);
   *h  = read_num(fd);
   if ((*w <= 0) || (*h <= 0))
   {
      return GBM_ERR_BAD_SIZE;
   }

   *m  = read_num(fd);
   if (*m <= 1)
   {
      return GBM_ERR_PPM_BAD_M;
   }

   /* check whether 1 byte or 2 byte format */
   if (*m < 0x100)
   {
      *data_bytes = 3 * (*w) * (*h);
   }
   else if (*m < 0x10000)
   {
      *data_bytes = 6 * (*w) * (*h);
   }
   else
   {
      return GBM_ERR_PPM_BAD_M;
   }

   return GBM_ERR_OK;
}

/* ---------------------------------------- */

/* Read number of images in the PPM file. */
GBM_ERR ppm_rimgcnt(const char *fn, int fd, int *pimgcnt)
{
   GBM_ERR rc;
   GBM     gbm;
   int     h1, h2, m, data_bytes;

   fn=fn; /* suppress compiler warning */

   *pimgcnt = 1;

   /* read header info of first bitmap */
   rc = read_ppm_header(fd, &h1, &h2, &gbm.w, &gbm.h, &m, &data_bytes);
   if (rc != GBM_ERR_OK)
   {
      return rc;
   }

   /* find last available image index */
   {
      long image_start;

      image_start = gbm_file_lseek(fd, 0, GBM_SEEK_CUR) + data_bytes;

      /* index 0 has already been read */

      /* move file pointer to beginning of the next bitmap */
      while (gbm_file_lseek(fd, image_start, GBM_SEEK_SET) >= 0)
      {
         /* read header info of next bitmap */
         rc = read_ppm_header(fd, &h1, &h2, &gbm.w, &gbm.h, &m, &data_bytes);
         if (rc != GBM_ERR_OK)
         {
            break;
         }
         (*pimgcnt)++;

         image_start = gbm_file_lseek(fd, 0, GBM_SEEK_CUR) + data_bytes;
      }
   }

   return GBM_ERR_OK;
}

/* ---------------------------------------- */

static GBM_ERR internal_ppm_rhdr(int fd, GBM * gbm, GBM * gbm_src)
{
   GBM_ERR rc;
   int     h1, h2, m, data_bytes;
   BOOLEAN use_native_bpp;
   const char *s = NULL;

   PPM_PRIV_READ *ppm_priv = (PPM_PRIV_READ *) gbm->priv;

   /* check if extended color depths are requested */
   use_native_bpp = (gbm_find_word(ppm_priv->read_options, "ext_bpp") != NULL)
                    ? TRUE : FALSE;

   /* start at the beginning of the file */
   gbm_file_lseek(fd, 0, GBM_SEEK_SET);

   /* read header info of first bitmap */
   rc = read_ppm_header(fd, &h1, &h2, &gbm->w, &gbm->h, &m, &data_bytes);
   if (rc != GBM_ERR_OK)
   {
      return rc;
   }

   /* goto requested image index */
   if ((s = gbm_find_word_prefix(ppm_priv->read_options, "index=")) != NULL)
   {
      int  image_index_curr;
      long image_start;
      int  image_index = 0;
      if (sscanf(s + 6, "%d", &image_index) != 1)
      {
         return GBM_ERR_BAD_OPTION;
      }

      image_start = gbm_file_lseek(fd, 0, GBM_SEEK_CUR) + data_bytes;

      /* index 0 has already been read */
      image_index_curr = 0;
      while (image_index_curr < image_index)
      {
         /* move file pointer to beginning of the next bitmap */
         if (gbm_file_lseek(fd, image_start, GBM_SEEK_SET) < 0)
         {
            return GBM_ERR_READ;
         }

         /* read header info of next bitmap */
         rc = read_ppm_header(fd, &h1, &h2, &gbm->w, &gbm->h, &m, &data_bytes);
         if (rc != GBM_ERR_OK)
         {
            return rc;
         }

         image_start = gbm_file_lseek(fd, 0, GBM_SEEK_CUR) + data_bytes;
         image_index_curr++;
      }
   }

   ppm_priv->max_intensity = (unsigned int) m;

   /* check whether 1 byte or 2 byte format */
   if (ppm_priv->max_intensity < 0x100)
   {
      gbm    ->bpp = 24;
      gbm_src->bpp = 24;
   }
   else if (ppm_priv->max_intensity < 0x10000)
   {
      gbm    ->bpp = use_native_bpp ? 48 : 24;
      gbm_src->bpp = 48;
   }
   else
   {
      return GBM_ERR_PPM_BAD_M;
   }

   gbm_src->w = gbm->w;
   gbm_src->h = gbm->h;

   return GBM_ERR_OK;
}

/* ---------------------------------------- */

GBM_ERR ppm_rhdr(const char *fn, int fd, GBM *gbm, const char *opt)
{
   PPM_PRIV_READ *ppm_priv = (PPM_PRIV_READ *) gbm->priv;
   GBM gbm_src;

   fn=fn; /* Suppress 'unref arg' compiler warnings */

   /* copy possible options */
   if (strlen(opt) >= sizeof(ppm_priv->read_options))
   {
      return GBM_ERR_BAD_OPTION;
   }
   strcpy(ppm_priv->read_options, opt);

   /* read bitmap info */
   return internal_ppm_rhdr(fd, gbm, &gbm_src);
}

/* ---------------------------------------- */
/* ---------------------------------------- */

GBM_ERR ppm_rpal(int fd, GBM *gbm, GBMRGB *gbmrgb)
{
   fd=fd; gbm=gbm; gbmrgb=gbmrgb; /* Suppress 'unref arg' compiler warnings */

   return GBM_ERR_OK;
}

/* ---------------------------------------- */
/* ---------------------------------------- */

GBM_ERR ppm_rdata(int fd, GBM *gbm, byte *data)
{
   PPM_PRIV_READ *ppm_priv = (PPM_PRIV_READ *) gbm->priv;

   int    i, line_bytes;
   byte * p;

   GBM gbm_src;

   const int stride = ((gbm->w * gbm->bpp + 31)/32) * 4;

   GBM_ERR rc = internal_ppm_rhdr(fd, gbm, &gbm_src);
   if (rc != GBM_ERR_OK)
   {
      return rc;
   }

   line_bytes = gbm_src.w * (gbm_src.bpp / 8);

   p = data + ((gbm->h - 1) * stride);

   switch(gbm->bpp)
   {
      case 24:
         /* check whether 1 byte or 2 byte format */
         if (ppm_priv->max_intensity < 0x100)
         {
            for (i = gbm->h - 1; i >= 0; i--)
            {
               gbm_file_read(fd, p, line_bytes);
               rgb_bgr(p, p, gbm->w, ppm_priv->max_intensity);
               p -= stride;
            }
         }
         else
         {
            byte * src_data = (byte *) malloc(line_bytes);
            if (src_data == NULL)
            {
               return GBM_ERR_MEM;
            }
            for (i = gbm->h - 1; i >= 0; i--)
            {
               gbm_file_read(fd, src_data, line_bytes);
               rgb16msb_bgr(src_data, p, gbm->w, ppm_priv->max_intensity);
               p -= stride;
            }
            free(src_data);
         }
         break;

      case 48:
         for (i = gbm->h - 1; i >= 0; i--)
         {
            gbm_file_read(fd, p, line_bytes);
            rgb16msb_bgr16lsb(p, p, gbm->w, ppm_priv->max_intensity);
            p -= stride;
         }
         break;

      default:
         return GBM_ERR_READ;
   }

   return GBM_ERR_OK;
}

/* ---------------------------------------- */
/* ---------------------------------------- */

GBM_ERR ppm_w(const char *fn, int fd, const GBM *gbm, const GBMRGB *gbmrgb, const byte *data, const char *opt)
{
   char  s[100+1];
   int   i;
   const byte *p;
   byte *linebuf;

   const int stride     = ((gbm->w * gbm->bpp + 31)/32) * 4;
   const int line_bytes = gbm->w * (gbm->bpp / 8);

   fn=fn; gbmrgb=gbmrgb; opt=opt; /* Suppress 'unref arg' compiler warnings */

   if ((linebuf = (byte *) malloc(stride)) == NULL)
   {
      return GBM_ERR_MEM;
   }

   switch(gbm->bpp)  
   {
      case 24:
         sprintf(s, "P6\n%d %d\n255\n", gbm->w, gbm->h);
         gbm_file_write(fd, s, (int) strlen(s));

         p = data + ((gbm->h - 1) * stride);
         for (i = gbm->h - 1; i >= 0; i--)
         {
            rgb_bgr(p, linebuf, gbm->w, 0x100);
            gbm_file_write(fd, linebuf, line_bytes);
            p -= stride;
         }
         break;

      case 48:
         sprintf(s, "P6\n%d %d\n65535\n", gbm->w, gbm->h);
         gbm_file_write(fd, s, (int) strlen(s));

         p = data + ((gbm->h - 1) * stride);
         for (i = gbm->h - 1; i >= 0; i--)
         {
            rgb16msb_bgr16lsb(p, linebuf, gbm->w, 0x10000);
            gbm_file_write(fd, linebuf, line_bytes);
            p -= stride;
         }
         break;

      default:
         free(linebuf);
         return GBM_ERR_NOT_SUPP;
   }

   free(linebuf);

   return GBM_ERR_OK;
}

/* ---------------------------------------- */

const char *ppm_err(GBM_ERR rc)
{
   switch ( (int) rc )
   {
      case GBM_ERR_PPM_BAD_M:
         return "bad maximum pixel intensity";
   }
   return NULL;
}

