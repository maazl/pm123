#define INCL_PM
#define INCL_DOS
#define INCL_DOSERRORS
#include <os2.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "slider.h"

char *pipe1 = "\\PIPE\\PM123",
     *pipe2 = "\\PIPE\\PM123_2";

void set_volume(int volume, char *pipename)
{
   HPIPE pipe;
   ULONG action, written, rc;
   char buf[256];
   int i = 0;

   do
   {
      rc = DosOpen(pipename,
                   &pipe,
                   &action,
                   0,
                   FILE_NORMAL,
                   OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                   OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE | OPEN_FLAGS_FAIL_ON_ERROR,
                   NULL);

      // if pipe busy, wait between 0 and 100 ms for retry
      if(rc == ERROR_PIPE_BUSY) 
         DosSleep(rand()*100/RAND_MAX); // Ethernet power :)

      i++;
   }
   while(rc == ERROR_PIPE_BUSY && i < 100);

   if(rc != 0)
   {
      sprintf( buf,"Could not open pipe %s, rc = %ld.", pipename, rc );
      WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, buf, "Error", 0, MB_ERROR | MB_OK);
      return;
   }

   sprintf(buf, "*volume %d", volume);
   rc = DosWrite(pipe,
                 buf,
                 strlen(buf) + 1,
                 &written);

   if(rc != 0)
   {
      sprintf( buf,"Error writing to pipe %s, rc = %ld.", pipename, rc );
      WinMessageBox(HWND_DESKTOP, HWND_DESKTOP, buf, "Error", 0, MB_ERROR | MB_OK);
      DosClose(pipe);
      return;
   }

   DosClose(pipe);
}

MRESULT EXPENTRY wpSlider(HWND hwnd, ULONG msg, MPARAM mp1, MPARAM mp2)
{
   switch(msg)
   {
      case WM_INITDLG:
         break;

      case WM_CONTROL:
         switch(SHORT1FROMMP(mp1))
         {
            case SL_VOLUME:
               switch(SHORT2FROMMP(mp1))
               {
                  case SLN_CHANGE:
                  case SLN_SLIDERTRACK:
                  {
                     MRESULT rangevalue = WinSendDlgItemMsg(hwnd, SHORT1FROMMP(mp1),
                             SLM_QUERYSLIDERINFO, MPFROM2SHORT(SMA_SLIDERARMPOSITION, SMA_RANGEVALUE), 0);
                     short value = SHORT1FROMMR(rangevalue);
                     short range = SHORT2FROMMR(rangevalue);

                     set_volume(value*100/range, pipe1);
                     set_volume(100-value*100/range, pipe2);
                     break;
                  }
               }
            break;
         }
      break;

      case WM_CLOSE:
         WinPostMsg(hwnd, WM_QUIT, 0, 0);
         break;
   }
   return WinDefDlgProc(hwnd, msg, mp1, mp2);
}

int main(int argc, char *argv[])
{
   HAB hab = 0;
   HMQ hmq = 0;
   QMSG qmsg;

   setvbuf(stdout,NULL,_IONBF, 0);
   srand(time(NULL));

   if(argc > 1)
      pipe1 = argv[1];

   if(argc > 2)
      pipe2 = argv[2];

   hab = WinInitialize(0);
   if(hab)
     hmq = WinCreateMsgQueue(hab, 0);
   if(!hmq)
     return 1;

   WinLoadDlg(HWND_DESKTOP, 0, wpSlider, 0, DLG_SLIDER, NULL);

   while( WinGetMsg( hab, &qmsg, 0L, 0, 0 ) )
      WinDispatchMsg( hab, &qmsg );

   WinDestroyMsgQueue( hmq );
   WinTerminate( hab );

   return 0;
}
