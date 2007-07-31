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


/* Code for the playlist manager */

#define INCL_BASE
#include <utilfct.h>
#include "pm123.h"
#include "pfreq.h"
#include "pfreq_base.h"
#include "url.h"

#include <debuglog.h>


// Default instance of playlist manager window, representing PM123.LST in the program folder.
static sco_ptr<PlaylistManager> DefaultPM;


/****************************************************************************
*
*  public external C interface
*
****************************************************************************/

/* Creates the playlist manager presentation window. */
void pm_create( void )
{ DEBUGLOG(("pm_create()\n"));
  url path = url::normalizeURL(startpath);
  const xstring& file = path + "PFREQ.LST";
  DEBUGLOG(("pm_create - %s\n", file.cdata()));
  DefaultPM = new PlaylistManager(file, "Playlist Manager");
  DefaultPM->SetVisible(cfg.show_plman);
}

/* Destroys the playlist manager presentation window. */
void pm_destroy( void )
{ DEBUGLOG(("pm_destroy()\n"));
  DefaultPM = NULL; // calls destructor
  DEBUGLOG(("pm_destroy() - end\n"));
}

/* Sets the visibility state of the playlist manager presentation window. */
void pm_show( BOOL show )
{ DEBUGLOG(("pm_show(%u)\n", show));
  if (DefaultPM != NULL)
    DefaultPM->SetVisible(show);
}

