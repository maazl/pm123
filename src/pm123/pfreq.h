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

#ifndef PM123_PFREQ_H
#define PM123_PFREQ_H

#define DLG_PM            48

#define PM_MAIN_MENU    1024
#define IDM_PM_APPEND   1000
#define IDM_PM_APPFILE  1600
#define IDM_PM_APPURL   1601
#define IDM_PM_APPOTHER 1602 /* reserve some ID's for several plug-ins.  */
#define IDM_PM_REMOVE   1012

#define PM_FILE_MENU    2048
#define PM_LIST_MENU    3072
#define IDM_PM_RENAME   1014
#define IDM_PM_LOAD     1013

//#define IDM_PM_L_REMOVE 3075
//#define IDM_PM_L_DELETE 3076
//#define IDM_PM_L_CALC   3077


#ifdef __cplusplus
extern "C" {
#endif

/* Creates the playlist manager presentation windowInitialize the playlist manager. */
void pm_create( void );
/* Sets the visibility state of the playlist manager presentation window. */
void pm_show( BOOL show );
/* Uninitialize the playlist manager. */
void pm_destroy( void );


#ifdef __cplusplus
}
#endif
#endif
