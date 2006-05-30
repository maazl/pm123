/*
 * Copyright 1997-2003 Samuel Audet <guardia@step.polymtl.ca>
 *                     Taneli Lepp„ <rosmo@sektori.com>
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

#ifndef _CDDARC_H
#define _CDDARC_H

#define DLG_CDDA            400
#define ST_CDDBSERVERS      401
#define LB_CDDBSERVERS      402
#define PB_ADD1             405
#define PB_DELETE1          406
#define ST_HTTPCDDBSERVERS  403
#define LB_HTTPCDDBSERVERS  404
#define PB_ADD2             407
#define PB_DELETE2          408
#define PB_UPDATE           409
#define ST_NEWSERVER        410
#define EF_NEWSERVER        411
#define CB_USEHTTP          412
#define ST_PROXY            413
#define EF_PROXY            414
#define CB_TRYALL           415
#define ST_EMAIL            418
#define EF_EMAIL            419
#define PB_NETWIN           422
#define PB_OFFDB            423
#define CB_USECDDB          424

#define DLG_MATCH           500
#define LB_MATCHES          501

#define DLG_NETWIN          600
#define MLE_NETLOG          FID_CLIENT

#define DLG_OFFDB           700
#define ST_CDINFO           701
#define LB_CDINFO           702
#define PB_DELETE           703

#endif /* _CDDARC_H */
