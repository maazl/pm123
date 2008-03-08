/*
 * Copyright 2007 Dmitry A.Steklenev <glass@ptv.ru>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *    1. Redistributions of source code must retain the above copyright notice,
 *       this list of conditions and the following disclaimer.
 *
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *
 *    3. The name of the author may not be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef PM123_TAGS_H
#define PM123_TAGS_H

#define DLG_FILEINFO     2022
#define NB_FILEINFO       100

#define DLG_SONGINFO     2012
#define ST_TAG_TITLE      101
#define EN_TAG_TITLE      102
#define CH_TAG_TITLE      103
#define ST_TAG_ARTIST     104
#define EN_TAG_ARTIST     105
#define CH_TAG_ARTIST     106
#define ST_TAG_ALBUM      107
#define EN_TAG_ALBUM      108
#define CH_TAG_ALBUM      109
#define ST_TAG_COMMENT    110
#define EN_TAG_COMMENT    111
#define CH_TAG_COMMENT    112
#define ST_TAG_COPYRIGHT  113
#define EN_TAG_COPYRIGHT  114
#define CH_TAG_COPYRIGHT  115
#define ST_TAG_GENRE      116
#define CB_TAG_GENRE      117
#define CH_TAG_GENRE      118
#define ST_TAG_TRACK      119
#define EN_TAG_TRACK      120
#define CH_TAG_TRACK      121
#define ST_TAG_YEAR       122
#define EN_TAG_YEAR       123
#define CH_TAG_YEAR       124
#define PB_TAG_APPLY      200
#define PB_TAG_UNDO       201
#define PB_TAG_APPLYGROUP 202

#define DLG_TECHINFO     2013
#define ST_FILENAME       101
#define EN_FILENAME       102
#define ST_DECODER        103
#define EN_DECODER        104
#define ST_INFO           105
#define EN_INFO           106
#define ST_MPEGINFO       107
#define EN_MPEGINFO       108
#define ST_SONGLENGTH     109
#define EN_SONGLENGTH     110
#define ST_FILESIZE       111
#define EN_FILESIZE       112
#define ST_STARTED        113
#define EN_STARTED        114
#define ST_ENDED          115
#define EN_ENDED          116
#define ST_TRACK_GAIN     117
#define EN_TRACK_GAIN     118
#define ST_TRACK_PEAK     119
#define EN_TRACK_PEAK     120
#define ST_ALBUM_GAIN     121
#define EN_ALBUM_GAIN     122
#define ST_ALBUM_PEAK     123
#define EN_ALBUM_PEAK     124

/* tag_edit options */
#define TAG_GROUP_OPERATIONS  0x0001

/* tag_edit returns */
#define TAG_NO_CHANGES        0
#define TAG_APPLY             1
#define TAG_APPLY_TO_GROUP    2

/* tag_apply options */
#define TAG_APPLY_CHOICED     0x0000
#define TAG_APPLY_ALL         0x0001

/* Edits a meta information of the specified file.
   Must be called from the main thread. */
int  tag_edit( HWND owner, const char* filename, char* decoder, DECODER_INFO* info, int options );
/* Sends request about applying a meta information to the specified file. */
BOOL tag_apply( const char* filename, char* decoder, DECODER_INFO* info, int options );

/* Initializes of the tagging module. */
void tag_init( void );
/* Terminates  of the tagging module. */
void tag_term( void );

#ifdef __cplusplus
}
#endif
#endif /* PM123_TAGS_H */
