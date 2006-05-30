/*

gbmscale.h - Interface to scaling code

*/

#ifndef GBMSCALE_H
#define	GBMSCALE_H

extern GBM_ERR gbm_simple_scale(
	const byte *s, int sw, int sh,
	      byte *d, int dw, int dh,
	int bpp
	);

#endif
