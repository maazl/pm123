/*

gbmrect.c - Subrectangle Transfer

*/

#ifndef GBMRECT_H
#define	GBMRECT_H

GBMEXPORT void GBMENTRY gbm_subrectangle(
	const GBM *gbm,
	int x, int y, int w, int h,
	const byte *data_src, byte *data_dst
	);

GBMEXPORT void GBMENTRY gbm_blit(
	const byte *s, int sw, int sx, int sy,
	      byte *d, int dw, int dx, int dy,
	int w, int h,
	int bpp
	);

#endif
