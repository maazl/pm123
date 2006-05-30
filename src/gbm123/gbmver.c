/*

gbmver.c - Shows the library version info of GBM (Generalized Bitmap Module).
           (On OS/2 the path to the detected GBM.DLL is shown as well.)

*/

#if defined(__OS2__) || defined(OS2)
#define INCL_DOS
#include <os2.h>
#endif

#include <stdio.h>
#include "gbm.h"

/* ------------------------------ */

int main(void)
{
    int version = 0;

#if defined(__OS2__) || defined(OS2)

  PSZ      ModuleName           = "GBM.DLL";
  CHAR     FoundModuleName[300] = "";
  UCHAR    LoadError[300]       = "";
  HMODULE  ModuleHandle         = NULLHANDLE;
  PFN      ModuleAddr           = 0;
  APIRET   rc                   = 0;

  rc = DosLoadModule(LoadError,
                     sizeof(LoadError)-1,
                     ModuleName,
                     &ModuleHandle);
  if (rc)
  {
     printf("%s not found.\n", ModuleName);
     return 1;
  }
 
  /* get the full path name */
  rc = DosQueryModuleName(ModuleHandle,
                          sizeof(FoundModuleName)-1,
                          FoundModuleName);
  if (rc)
  {
     printf("Could not extract module path name.\n");
     DosFreeModule(ModuleHandle);
     return 1;
  }

  /* call Gbm_init() */
  rc = DosQueryProcAddr(ModuleHandle,
                        0L,         
                        "Gbm_init",
                        &ModuleAddr);
  if (rc)
  {
     printf("Detected %s has wrong file format.\n", ModuleName);
     DosFreeModule(ModuleHandle);
     return 1;
  }

  ModuleAddr();

  /* ------------------------ */

  /* call Gbm_version() */
  rc = DosQueryProcAddr(ModuleHandle,
                        0L,         
                        "Gbm_version",
                        &ModuleAddr);
  if (rc)
  {
     printf("Detected %s has wrong file format.\n", ModuleName);
     DosFreeModule(ModuleHandle);
     return 1;
  }
 
  version = ModuleAddr();

  /* ------------------------ */

  /* call Gbm_deinit() */
  rc = DosQueryProcAddr(ModuleHandle,
                        0L,         
                        "Gbm_deinit",
                        &ModuleAddr);
  if (rc)
  {
     printf("Detected %s has wrong file format.\n", ModuleName);
     DosFreeModule(ModuleHandle);
     return 1;
  }

  ModuleAddr();

  DosFreeModule(ModuleHandle);

  printf("%s, version %d.%d\n", FoundModuleName, version/100, version-100);

#else

    /* Standard handling for non-OS/2 operating systems */
    gbm_init();

    /* get library version */
    version = gbm_version();

    gbm_deinit();

    printf("GBM.DLL version: %d.%d\n", version/100, version-100);

#endif

    return 0;
}


