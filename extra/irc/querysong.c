#define INCL_DOS
#include <os2.h>
#include <string.h>
#include <stdio.h>

int main( int argc, char *argv[] )
{
   HPIPE pipe = 0;
   ULONG action = 0, bytesread = 0;
   char buf[256], *buf2, *buf3;

   DosOpen("\\PIPE\\PM123",
           &pipe,
           &action,
           0,
           FILE_NORMAL,
           OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
           OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE | OPEN_FLAGS_FAIL_ON_ERROR,
           NULL);

   strcpy(buf, "*status tag");
   DosWrite((HFILE) pipe,
            buf,
            strlen(buf) + 1,
            &action);

   *buf = '\0';
   bytesread = 0;

   while(bytesread == 0)
      DosRead(pipe,
              buf,
              sizeof(buf),
              &bytesread);

   DosClose(pipe);
   pipe = 0;
   action = 0;

   /* display tag */
   if(strlen(buf) > 0)
   {
      printf("%s", buf);
   }
   /* if no tag, display the filename */
   else
   {
      while( DosOpen("\\PIPE\\PM123",
                     &pipe,
                     &action,
                     0,
                     FILE_NORMAL,
                     OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                     OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE | OPEN_FLAGS_FAIL_ON_ERROR,
                     NULL) != 0)
         DosSleep(1L);

      strcpy(buf, "*status file");
      DosWrite((HFILE) pipe,
               buf,
               strlen(buf) + 1,
               &action);

      *buf = '\0';
      bytesread = 0;

      while(bytesread == 0)
         DosRead(pipe,
                 buf,
                 sizeof(buf),
                 &bytesread);

      buf3 = buf;
      while(*buf3)
      {
         if(*buf3 == '\\')
         {
            *buf3 = '/';
         }
         buf3++;
      }

      buf2 = strrchr(buf, '/');
      if(buf2)
      {
         *buf2 = '\0';
         buf2++;
         buf3 = (1 + strchr(buf, '/'));
         if((buf3 - 1))
            printf("%s/%s", buf3, buf2);
         else
            printf("%s", buf2);
      }
      else
         printf("%s", buf);

      DosClose(pipe);
      pipe = 0;
      action = 0;
   }

   if(argc > 1 && stricmp(argv[1], "-v") == 0)
   {
      while(DosOpen("\\PIPE\\PM123",
                    &pipe,
                    &action,
                    0,
                    FILE_NORMAL,
                    OPEN_ACTION_FAIL_IF_NEW | OPEN_ACTION_OPEN_IF_EXISTS,
                    OPEN_SHARE_DENYREADWRITE | OPEN_ACCESS_READWRITE | OPEN_FLAGS_FAIL_ON_ERROR,
                    NULL) != 0)
         DosSleep(1L);

      strcpy(buf, "*status info");
      DosWrite((HFILE) pipe,
               buf,
               strlen(buf) + 1,
               &action);

      *buf = '\0';
      bytesread = 0;

      while(bytesread == 0)
         DosRead(pipe,
                 buf,
                 sizeof(buf),
                 &bytesread);

      printf(" (%s)", buf);
   }

   DosClose(pipe);
   pipe = 0;
   action = 0;

   printf("\n");
   return 0;
}
