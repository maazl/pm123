/*

gbmmir.h - Interface to Mirror Image of General Bitmap

*/

#ifndef GBMMIR_H
#define	GBMMIR_H

extern BOOLEAN gbm_ref_vert(const GBM *gbm, byte *data);
extern BOOLEAN gbm_ref_horz(const GBM *gbm, byte *data);
extern void gbm_transpose(const GBM *gbm, const byte *data, byte *data_t);

#endif
