#ifndef _GBM_DESC_H_
#define _GBM_DESC_H_

/*

gbmdesc.h - File format description (extracted for simplified localization)

History:
--------
(Heiko Nitzsche)

23-Feb-2006: Move format description strings of all GBM formats to this file
             to simplify localization by simply replacing this file.

*/

/* ------------------------------------------------- */

/* OS/2-, Windows bitmap format */
#define GBM_FMT_DESC_SHORT_BMP  "Bitmap"
#define GBM_FMT_DESC_LONG_BMP   "OS/2 1.1, 1.2, 2.0 / Windows 3.0 bitmap"
#define GBM_FMT_DESC_EXT_BMP    "BMP VGA BGA RLE DIB RL4 RL8"

/* Portrait */
#define GBM_FMT_DESC_SHORT_CVP  "Portrait"
#define GBM_FMT_DESC_LONG_CVP   "Portrait"
#define GBM_FMT_DESC_EXT_CVP    "CVP"

/* GEM Raster */
#define GBM_FMT_DESC_SHORT_GEM  "GemRas"
#define GBM_FMT_DESC_LONG_GEM   "GEM Raster"
#define GBM_FMT_DESC_EXT_GEM    "IMG XIMG"

/* CompuServe Graphics Interchange Format */
#define GBM_FMT_DESC_SHORT_GIF  "GIF"
#define GBM_FMT_DESC_LONG_GIF   "CompuServe Graphics Interchange Format"
#define GBM_FMT_DESC_EXT_GIF    "GIF"

/* IBM Image Access eXecutive support */
#define GBM_FMT_DESC_SHORT_IAX  "IAX"
#define GBM_FMT_DESC_LONG_IAX   "IBM Image Access eXecutive"
#define GBM_FMT_DESC_EXT_IAX    "IAX"

/* JPEG File Interchange Format */
#ifdef ENABLE_IJG
  #define GBM_FMT_DESC_SHORT_JPG  "JPEG"
  #define GBM_FMT_DESC_LONG_JPG   "JPEG File Interchange Format"
  #define GBM_FMT_DESC_EXT_JPG    "JPG JPEG JPE"
#endif

/* IBM KIPS file format */
#define GBM_FMT_DESC_SHORT_KPS  "KIPS"
#define GBM_FMT_DESC_LONG_KPS   "IBM KIPS"
#define GBM_FMT_DESC_EXT_KPS    "KPS"

/* Amiga IFF / ILBM format */
#define GBM_FMT_DESC_SHORT_LBM  "ILBM"
#define GBM_FMT_DESC_LONG_LBM   "Amiga IFF / ILBM Interleaved bitmap"
#define GBM_FMT_DESC_EXT_LBM    "IFF LBM"

/* ZSoft PC Paintbrush format */
#define GBM_FMT_DESC_SHORT_PCX  "PCX"
#define GBM_FMT_DESC_LONG_PCX   "ZSoft PC Paintbrush Image format"
#define GBM_FMT_DESC_EXT_PCX    "PCX PCC"

/* Poskanzers PGM format */
#define GBM_FMT_DESC_SHORT_PGM  "Greymap"
#define GBM_FMT_DESC_LONG_PGM   "Portable Greyscale-map (binary P5 type)"
#define GBM_FMT_DESC_EXT_PGM    "PGM"

/* Portable Network Graphics Format */
#ifdef ENABLE_PNG
  #define GBM_FMT_DESC_SHORT_PNG  "PNG"
  #define GBM_FMT_DESC_LONG_PNG   "Portable Network Graphics Format"
  #define GBM_FMT_DESC_EXT_PNG    "PNG"
#endif

/* Portable Pixel-map format */
#define GBM_FMT_DESC_SHORT_PPM  "Pixmap"
#define GBM_FMT_DESC_LONG_PPM   "Portable Pixel-map (binary P6 type)"
#define GBM_FMT_DESC_EXT_PPM    "PPM"

/* IBM Printer Page Segment format */
#define GBM_FMT_DESC_SHORT_PSG  "PSEG"
#define GBM_FMT_DESC_LONG_PSG   "IBM Printer Page Segment"
#define GBM_FMT_DESC_EXT_PSG    "PSE PSEG PSEG38PP PSEG3820"

/* Archimedes Sprite from RiscOS Format */
#define GBM_FMT_DESC_SHORT_SPR  "Sprite"
#define GBM_FMT_DESC_LONG_SPR   "Archimedes Sprite from RiscOS"
#define GBM_FMT_DESC_EXT_SPR    "SPR SPRITE"

/* Truevision Targa/Vista bitmap Format */
#define GBM_FMT_DESC_SHORT_TGA  "Targa"
#define GBM_FMT_DESC_LONG_TGA   "Truevision Targa/Vista bitmap"
#define GBM_FMT_DESC_EXT_TGA    "TGA VST AFI"

/* Microsoft/Aldus Tagged Image File Format */
#ifdef ENABLE_TIF
  #define GBM_FMT_DESC_SHORT_TIF  "TIFF"
  #define GBM_FMT_DESC_LONG_TIF   "Tagged Image File Format support (TIFF 6.0)"
  #define GBM_FMT_DESC_EXT_TIF    "TIF TIFF"
#endif

/* YUV12C M-Motion Video Frame Buffer format */
#define GBM_FMT_DESC_SHORT_VID  "YUV12C"
#define GBM_FMT_DESC_LONG_VID   "YUV12C M-Motion Video Frame Buffer"
#define GBM_FMT_DESC_EXT_VID    "VID"

/* X Windows Bitmap format */
#define GBM_FMT_DESC_SHORT_XBM  "XBitmap"
#define GBM_FMT_DESC_LONG_XBM   "X Windows Bitmap"
#define GBM_FMT_DESC_EXT_XBM    "XBM"


#endif


