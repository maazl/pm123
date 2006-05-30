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

#define INCL_DOS
#define INCL_PM
#include <os2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "utilfct.h"

APIRET APIENTRY DosQueryModFromEIP ( HMODULE *phMod, ULONG *pObjNum, ULONG BuffLen,
         PCHAR pBuff, ULONG *pOffset, ULONG Address ) ;

unsigned long get_eip(void);

void getModule(HMODULE *thisModule, char *thisModuleName, int size)
{
   if(thisModuleName != NULL && size > 0)
   {
      ULONG ObjNum = 0, Offset = 0;
      DosQueryModFromEIP(thisModule,&ObjNum,size,thisModuleName,&Offset,get_eip());
      DosQueryModuleName(*thisModule,size,thisModuleName); // full path
   }
}

void getExeName(char *buf, int size)
{
   if(buf && size > 0)
   {
      PPIB ppib;
      PTIB ptib;

      DosGetInfoBlocks(&ptib, &ppib);
      DosQueryModuleName(ppib->pib_hmte, size, buf);
   }
}

HINI open_ini(char *filename)
{
   return PrfOpenProfile(WinQueryAnchorBlock(HWND_DESKTOP),filename);
}

// opens an ini file by the name of the module in the EXE directory
HINI open_module_ini()
{
   HMODULE module;
   char moduleName[CCHMAXPATH], exePath[CCHMAXPATH];
   char inifilename[2*CCHMAXPATH];
   char *dot;

   getModule(&module,moduleName,CCHMAXPATH);
   getExeName(exePath,CCHMAXPATH);

   strcpy(inifilename, exePath);
   *strrchr(inifilename, '\\') = 0;
   strcat(inifilename,strrchr(moduleName,'\\'));

   dot = strrchr(inifilename,'.');
   if(dot != NULL)
   {
      strcpy(dot,".INI");
      return open_ini(inifilename);
   }
   return NULLHANDLE;
}

BOOL close_ini(HINI hini)
{
   return PrfCloseProfile(hini);
}

char *blank_strip(char *string)
{
   int i = 0;
   char *pos = string;
   while (*pos == ' ' || *pos == '\t' ||
          *pos == '\n' || *pos == '\r') pos++;
   i = strlen(pos)+1;

   if(pos != string)
      memmove(string,pos,i);

   i-=2;
   while (string[i] == ' ' || string[i] == '\t' ||
          string[i] == '\n' || string[i] == '\r') { string[i] = 0; i--; }

   return string;
}

void percent_encode(char *string, int size, char *partialset)
{
   if(string == NULL || size == 0 || partialset == NULL)
      return;
   else
   {

   int i,j;
   int setlength = strlen(partialset)+1;
   int length = strlen(string);
   int strpos = 0;                  // pos in dest string
   char *src = alloca(length+1);    // unmodified source string
   char *set = alloca(setlength+1); // partialset + '%'

   memcpy(src,string,length+1);
   set[0] = '%';
   memcpy(set+1,partialset,setlength);

   for(i = 0; i < length; i++)
   {
      for(j = 0; j < setlength; j++)
      {
         if(src[i] == set[j])
         {
            if(strpos+2 >= size)
            {
               string[size-1] = 0;
               return;
            }
            else
            {
               string[strpos++] = '%';
               string[strpos] = 0;
               string[strpos+1] = 0;
               _itoa(src[i],string+strpos,16);
               strpos += 2;
            }
            goto doneparse;
         }
      }

      if(strpos >= size)
      {
         string[size-1] = 0;
         return;
      }
      else
      {
         string[strpos++] = src[i];
      }

doneparse: ;

   }
   string[strpos++] = 0;

   }
}

void percent_decode(char *string)
{
   if(string == NULL)
      return;
   else
   {

   int i;
   int length = strlen(string);
   int strpos = 0;                  // pos in dest string
   char *src = alloca(length+1);    // unmodified source string

   memcpy(src,string,length+1);

   for(i = 0; i < length; i++)
   {
      if(src[i] == '%')
      {
         char number[4] = { src[++i], src[++i], 0 };
         char character = strtol(number,NULL,16);
         string[strpos++] = character;
      }
      else
      {
         string[strpos++] = src[i];
      }
   }

   string[strpos++] = 0;

   }
}

/* removes comments starting with # */
char *uncomment(char *something)
{
   int source = 0;
   BOOL inquotes = FALSE;

   while(something[source])
   {
      if(something[source] == '\"')
         inquotes = !inquotes;
      else if(something[source] == '#' && !inquotes)
      {
         something[source] = 0;
         break;
      }
      source++;
   }

   return something;
}

/* remove leading and trailing spaces and quotes */
char *quote_strip(char *something)
{
   int i,e;
   char *pos = something;

   while (*pos == ' ' || *pos == '\t' || *pos == '\n') pos++;
   i = strlen(pos) - 1;
   for (;i>0;i--) if (pos[i] != ' ' && pos[i] != '\t' && pos[i] != '\n') break;
   if(*pos == '\"' && pos[i] == '\"' )
   {
      pos++;
      i -= 2;
   }
   for(e = 0; e < i+1; e++)
      something[e] = pos[e];
   something[e] = 0;
   return(something);
}

char *translateChar(char *string, char to[], char from[])
{
   int i;
   for(i = 0; from[i]; i++)
   {
      char *pos = string;
      while(*pos)
      {
         if(*pos == from[i])
            *pos = to[i];
         pos++;
      }
   }
   return string;
}

char *LFN2SFN(char *LFN, char *SFN)
{
   char leftpart[12];
   char rightpart[4];
   char *period;

   period = strrchr(LFN,'.');
   if(!period)
   {
      strncpy(leftpart,LFN,8);
      leftpart[8] = 0;
      rightpart[0] = 0;
   }
   else if(period-LFN > 8)
   {
      strncpy(leftpart,LFN,8);
      leftpart[8] = 0;
      strncpy(rightpart,period+1,3);
      rightpart[3] = 0;
   }
   else
   {
      strncpy(leftpart,LFN,period-LFN);
      leftpart[period-LFN] = 0;
      strncpy(rightpart,period+1,3);
      rightpart[3] = 0;
   }

   /* let's remove chars that are illegal in 8.3 filenames */
   translateChar(rightpart, "__-()!-__" , ". +[];=,~");
   translateChar(leftpart, "__-()!-__" , ". +[];=,~");

   strcpy(SFN,leftpart);
   if(rightpart[0])
   {
      strcat(SFN,".");
      strcat(SFN,rightpart);
   }

   return SFN;
}

/* very common PM stuff which I couldn't care less to remember by heart */

BOOL initPM( HAB *hab, HMQ *hmq )
{
   *hmq = *hab = 0;

   *hab = WinInitialize(0);
   if(*hab)
     *hmq = WinCreateMsgQueue(*hab, 0);
   if(*hmq)
     return TRUE;

   return FALSE;
}

void runPM( HAB hab )
{
   QMSG qmsg;

   while( WinGetMsg( hab, &qmsg, 0L, 0, 0 ) )
      WinDispatchMsg( hab, &qmsg );
}

void closePM( HAB hab, HMQ hmq )
{
   WinDestroyMsgQueue( hmq );
   WinTerminate( hab );
}

HWND loadDlg( HWND parent, PFNWP winproc, ULONG id )
{
   return WinLoadDlg(parent, parent, winproc, 0, id, NULL);
}

/* check buttons */

USHORT getCheck( HWND hwnd, LONG id )
{
   if(id)
      return SHORT1FROMMR(WinSendDlgItemMsg(hwnd, id, BM_QUERYCHECK, 0, 0));
   else
      return SHORT1FROMMR(WinSendMsg(hwnd, BM_QUERYCHECK, 0, 0));
}

USHORT setCheck( HWND hwnd, LONG id, USHORT state )
{
   if(id)
      return SHORT1FROMMR(WinSendDlgItemMsg(hwnd, id, BM_SETCHECK, MPFROMSHORT(state), 0));
   else
      return SHORT1FROMMR(WinSendMsg(hwnd, BM_SETCHECK, MPFROMSHORT(state), 0));
}

/* general window */

ULONG getText( HWND hwnd, LONG id, char *buffer, LONG size )
{
   if(id)
      return WinQueryDlgItemText(hwnd,id,size,buffer);
   else
      return WinQueryWindowText(hwnd,size,buffer);
}

BOOL setText( HWND hwnd, LONG id, char *buffer )
{
   if(id)
      return WinSetDlgItemText(hwnd, id, buffer);
   else
      return WinSetWindowText(hwnd, buffer);
}

BOOL enable(HWND hwnd, LONG id )
{
   if(id)
      return WinEnableWindow(WinWindowFromID(hwnd,id), TRUE);
   else
      return WinEnableWindow(hwnd, TRUE);
}

BOOL disable( HWND hwnd, LONG id )
{
   if(id)
      return WinEnableWindow(WinWindowFromID(hwnd,id), FALSE);
   else
      return WinEnableWindow(hwnd, FALSE);
}

/* listbox */

SHORT getItemCount( HWND hwnd, LONG id )
{
   if(id)
      return SHORT1FROMMR(WinSendDlgItemMsg(hwnd,id,LM_QUERYITEMCOUNT,0,0));
   else
      return SHORT1FROMMR(WinSendMsg(hwnd,LM_QUERYITEMCOUNT,0,0));
}

SHORT getItemText( HWND hwnd, LONG id, SHORT item, char *buffer, LONG size )
{
   if(id)
      return SHORT1FROMMR(WinSendDlgItemMsg(hwnd,id,LM_QUERYITEMTEXT,MPFROM2SHORT(item,size),MPFROMP(buffer)));
   else
      return SHORT1FROMMR(WinSendMsg(hwnd,LM_QUERYITEMTEXT,MPFROM2SHORT(item,size),MPFROMP(buffer)));
}

SHORT getItemTextSize( HWND hwnd, LONG id, SHORT item )
{
   if(id)
      return SHORT1FROMMR(WinSendDlgItemMsg(hwnd,id,LM_QUERYITEMTEXTLENGTH,MPFROMSHORT(item),0));
   else
      return SHORT1FROMMR(WinSendMsg(hwnd,LM_QUERYITEMTEXTLENGTH,MPFROMSHORT(item),0));
}

SHORT insertItemText( HWND hwnd, LONG id, SHORT item, char *buffer )
{
   if(id)
      return SHORT1FROMMR(WinSendDlgItemMsg(hwnd, id, LM_INSERTITEM, MPFROMSHORT(item), MPFROMP(buffer)));
   else
      return SHORT1FROMMR(WinSendMsg(hwnd, LM_INSERTITEM, MPFROMSHORT(item), MPFROMP(buffer)));
}

BOOL setItemHandle( HWND hwnd, LONG id, SHORT item, void *pointer )
{
   if(id)
      return LONGFROMMR(WinSendDlgItemMsg(hwnd, id, LM_SETITEMHANDLE, MPFROMSHORT(item), MPFROMP(pointer)));
   else
      return LONGFROMMR(WinSendMsg(hwnd, LM_SETITEMHANDLE, MPFROMSHORT(item), MPFROMP(pointer)));
}

void *getItemHandle( HWND hwnd, LONG id, SHORT item )
{
   if(id)
      return PVOIDFROMMR(WinSendDlgItemMsg(hwnd, id, LM_QUERYITEMHANDLE, MPFROMSHORT(item), 0));
   else
      return PVOIDFROMMR(WinSendMsg(hwnd, LM_QUERYITEMHANDLE, MPFROMSHORT(item), 0));
}

SHORT deleteItem( HWND hwnd, LONG id, SHORT item )
{
   if(id)
      return SHORT1FROMMR(WinSendDlgItemMsg(hwnd,id, LM_DELETEITEM, MPFROMSHORT(item),0));
   else
      return SHORT1FROMMR(WinSendMsg(hwnd, LM_DELETEITEM, MPFROMSHORT(item),0));
}

SHORT deleteAllItems( HWND hwnd, LONG id )
{
   if(id)
      return SHORT1FROMMR(WinSendDlgItemMsg(hwnd,id, LM_DELETEALL, 0,0));
   else
      return SHORT1FROMMR(WinSendMsg(hwnd, LM_DELETEALL, 0,0));
}

SHORT selectItem( HWND hwnd, LONG id, SHORT item )
{
   if(id)
      return SHORT1FROMMR(WinSendDlgItemMsg(hwnd,id, LM_SELECTITEM, MPFROMSHORT(item),MPFROMLONG(TRUE)));
   else
      return SHORT1FROMMR(WinSendMsg(hwnd, LM_SELECTITEM, MPFROMSHORT(item),MPFROMLONG(TRUE)));
}

SHORT deSelectItem( HWND hwnd, LONG id, SHORT item )
{
   if(id)
      return SHORT1FROMMR(WinSendDlgItemMsg(hwnd,id, LM_SELECTITEM, MPFROMSHORT(item),MPFROMLONG(FALSE)));
   else
      return SHORT1FROMMR(WinSendMsg(hwnd, LM_SELECTITEM, MPFROMSHORT(item),MPFROMLONG(FALSE)));
}

SHORT getSelectItem( HWND hwnd, LONG id, SHORT startitem )
{
   if(id)
      return SHORT1FROMMR(WinSendDlgItemMsg(hwnd,id, LM_QUERYSELECTION, MPFROMSHORT(startitem),0));
   else
      return SHORT1FROMMR(WinSendMsg(hwnd, LM_QUERYSELECTION, MPFROMSHORT(startitem),0));
}

SHORT searchItemText( HWND hwnd, LONG id, SHORT startitem, char *string )
{
   if(id)
      return SHORT1FROMMR(WinSendDlgItemMsg(hwnd,id, LM_SEARCHSTRING, MPFROM2SHORT(0,startitem),MPFROMP(string)));
   else
      return SHORT1FROMMR(WinSendMsg(hwnd, LM_SEARCHSTRING, MPFROM2SHORT(0,startitem),MPFROMP(string)));
}

/* containers */

RECORDCORE *allocaRecords( HWND hwnd, LONG id, USHORT count, USHORT custom )
{
   if(id)
      return (RECORDCORE *) PVOIDFROMMR(WinSendDlgItemMsg(hwnd,id, CM_ALLOCRECORD, MPFROMSHORT(custom), MPFROMSHORT(count)));
   else
      return (RECORDCORE *) PVOIDFROMMR(WinSendMsg(hwnd, CM_ALLOCRECORD, MPFROMSHORT(custom), MPFROMSHORT(count)));
}

BOOL freeRecords( HWND hwnd, LONG id, RECORDCORE *records, ULONG count )
{
   if(id)
      return LONGFROMMR(WinSendDlgItemMsg(hwnd,id, CM_FREERECORD, MPFROMP(records), MPFROMLONG(count)));
   else
      return LONGFROMMR(WinSendMsg(hwnd, CM_FREERECORD, MPFROMP(records), MPFROMLONG(count)));
}

ULONG insertRecords( HWND hwnd, LONG id, RECORDCORE *records, RECORDINSERT *info )
{
   if(id)
      return LONGFROMMR(WinSendDlgItemMsg(hwnd,id, CM_INSERTRECORD, MPFROMP(records), MPFROMP(info)));
   else
      return LONGFROMMR(WinSendMsg(hwnd, CM_INSERTRECORD, MPFROMP(records), MPFROMP(info)));
}

RECORDCORE *searchRecords( HWND hwnd, LONG id, RECORDCORE *record, USHORT emphasis )
{
   if(id)
      return (RECORDCORE *) PVOIDFROMMR(WinSendDlgItemMsg(hwnd,id, CM_QUERYRECORDEMPHASIS, MPFROMP(record), MPFROMLONG(emphasis)));
   else
      return (RECORDCORE *) PVOIDFROMMR(WinSendMsg(hwnd, CM_QUERYRECORDEMPHASIS, MPFROMP(record), MPFROMLONG(emphasis)));
}

RECORDCORE *enumRecords( HWND hwnd, LONG id, RECORDCORE *record, USHORT cmd )
{
   if( (ULONG) record == CMA_FIRST) cmd = CMA_FIRST;
   else if( (ULONG) record == CMA_LAST) cmd = CMA_LAST;

   if(id)
      return (RECORDCORE *) PVOIDFROMMR(WinSendDlgItemMsg(hwnd,id, CM_QUERYRECORD, MPFROMP(record), MPFROM2SHORT(cmd,CMA_ITEMORDER)));
   else
      return (RECORDCORE *) PVOIDFROMMR(WinSendMsg(hwnd, CM_QUERYRECORD, MPFROMP(record), MPFROM2SHORT(cmd,CMA_ITEMORDER)));
}

ULONG removeRecords( HWND hwnd, LONG id, RECORDCORE *records[], ULONG count )
{
   if(id)
      return LONGFROMMR(WinSendDlgItemMsg(hwnd,id, CM_REMOVERECORD,
      MPFROMP(records), MPFROM2SHORT(count, CMA_INVALIDATE | CMA_FREE)));
   else
      return LONGFROMMR(WinSendMsg(hwnd, CM_REMOVERECORD,
      MPFROMP(records), MPFROM2SHORT(count, CMA_INVALIDATE | CMA_FREE)));
}

BOOL getRecordPosition( HWND hwnd, LONG id, RECORDCORE *record, RECTL *pos, ULONG fsExtent )
{
   QUERYRECORDRECT query;

   query.cb = sizeof(query);
   query.pRecord = record;
   query.fRightSplitWindow = FALSE;
// query.fsExtent = CMA_ICON | CMA_TEXT;
   query.fsExtent = fsExtent;

   if(id)
      return LONGFROMMR(WinSendDlgItemMsg(hwnd,id, CM_QUERYRECORDRECT, MPFROMP(pos), MPFROMP(&query)));
   else
      return LONGFROMMR(WinSendMsg(hwnd, CM_QUERYRECORDRECT, MPFROMP(pos), MPFROMP(&query)));
}

FIELDINFO *allocaFieldInfo( HWND hwnd, LONG id, USHORT count )
{
   if(id)
      return (FIELDINFO *) PVOIDFROMMR(WinSendDlgItemMsg(hwnd, id, CM_ALLOCDETAILFIELDINFO, MPFROMSHORT(count), 0));
   else
      return (FIELDINFO *) PVOIDFROMMR(WinSendMsg(hwnd, CM_ALLOCDETAILFIELDINFO, MPFROMSHORT(count), 0));
}

USHORT insertFieldInfo( HWND hwnd, LONG id, FIELDINFO *records, FIELDINFOINSERT *info )
{
   if(id)
      return SHORT1FROMMR(WinSendDlgItemMsg(hwnd, id, CM_INSERTDETAILFIELDINFO, MPFROMP(records), MPFROMP(info)));
   else
      return SHORT1FROMMR(WinSendMsg(hwnd, CM_INSERTDETAILFIELDINFO, MPFROMP(records), MPFROMP(info)));
}

SHORT removeFieldInfo( HWND hwnd, LONG id, FIELDINFO *fields[], USHORT count )
{
   if(id)
      return SHORT1FROMMR(WinSendDlgItemMsg(hwnd, id, CM_REMOVEDETAILFIELDINFO, MPFROMP(fields), MPFROM2SHORT(count,CMA_FREE)));
   else
      return SHORT1FROMMR(WinSendMsg(hwnd, CM_REMOVEDETAILFIELDINFO, MPFROMP(fields), MPFROM2SHORT(count,CMA_FREE)));
}


BOOL setRecordSource( HWND hwnd, LONG id, RECORDCORE *record, BOOL on )
{
   if(id)
      return LONGFROMMR(WinSendDlgItemMsg(hwnd, id, CM_SETRECORDEMPHASIS, MPFROMP(record), MPFROM2SHORT(on, CRA_SOURCE)));
   else
      return LONGFROMMR(WinSendMsg(hwnd, CM_SETRECORDEMPHASIS, MPFROMP(record), MPFROM2SHORT(on, CRA_SOURCE)));
}

BOOL selectRecord( HWND hwnd, LONG id, RECORDCORE *record, BOOL on )
{
   if(id)
      return LONGFROMMR(WinSendDlgItemMsg(hwnd, id, CM_SETRECORDEMPHASIS, MPFROMP(record), MPFROM2SHORT(on, CRA_SELECTED)));
   else
      return LONGFROMMR(WinSendMsg(hwnd, CM_SETRECORDEMPHASIS, MPFROMP(record), MPFROM2SHORT(on, CRA_SELECTED)));
}

BOOL selectAllRecords( HWND hwnd, LONG id, BOOL on )
{
   BOOL returnBool = FALSE;
   RECORDCORE *record;
   record = enumRecords(hwnd, id, NULL, CMA_FIRST);
   while(record && record != (RECORDCORE *) -1)
   {
      returnBool = selectRecord(hwnd, id, record, on);
      record = enumRecords(hwnd, id, record, CMA_NEXT);
   }
   return returnBool;
}

BOOL removeTitleFromContainer( HWND hwnd, LONG id, char *title )
{
   RECORDCORE *record[1];

   *record = enumRecords(hwnd, id, NULL, CMA_FIRST);

   while(*record && *record != (RECORDCORE *) -1)
   {
      if(!strcmp(title,(*record)->pszIcon))
      {
         removeRecords(hwnd, id, record, 1);
         return TRUE;
      }
      *record = enumRecords(hwnd, id, *record, CMA_NEXT);
   }
   return FALSE;
}

