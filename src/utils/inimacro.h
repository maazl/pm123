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

#if __cplusplus
extern "C" {
#endif

HINI open_ini(char *filename);
HINI open_module_ini(void);
BOOL close_ini(HINI hini);

#define save_ini_value(INIhandle, var) PrfWriteProfileData(INIhandle, "Settings", #var, &var, sizeof(var));
#define save_ini_string(INIhandle, var) PrfWriteProfileString(INIhandle, "Settings", #var, var);
#define save_ini_data(INIhandle, var, size) PrfWriteProfileData(INIhandle, "Settings", #var, var, size);

#define load_ini_value(INIhandle, var)                                   \
{                                                                        \
   ULONG datasize;                                                       \
   PrfQueryProfileSize(INIhandle, "Settings", #var, &datasize);          \
   if (datasize == sizeof(var))                                          \
      PrfQueryProfileData(INIhandle, "Settings", #var, &var, &datasize); \
}

#define load_ini_data_size(INIhandle, var, size)                         \
{                                                                        \
   size = 0;                                                             \
   PrfQueryProfileSize(INIhandle, "Settings", #var, &size);              \
}

#define load_ini_data(INIhandle, var, size)                              \
{                                                                        \
   ULONG datasize;                                                       \
   PrfQueryProfileSize(INIhandle, "Settings", #var, &datasize);          \
   if (datasize == size)                                                 \
      PrfQueryProfileData(INIhandle, "Settings", #var, var, &datasize);  \
}

#define load_ini_string(INIhandle, var, size) PrfQueryProfileString(INIhandle, "Settings", #var, NULL, var, size);

#if __cplusplus
}
#endif

