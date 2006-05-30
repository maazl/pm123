/*
 * Copyright (C) 1996, 1997, 1998, 1999, 2000 Florian Schintke
 *
 * This is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free 
 * Software Foundation; either version 2, or (at your option) any later 
 * version. 
 *
 * This is distributed in the hope that it will be useful, but WITHOUT 
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
 * for more details. 
 * 
 * You should have received a copy of the GNU General Public License with 
 * the c2html, java2html, pas2html or perl2html source package as the 
 * file COPYING. If not, write to the Free Software Foundation, Inc., 
 * 59 Temple Place - Suite 330, Boston, MA 
 * 02111-1307, USA. 
 *
 * Documenttitle and purpose:
 *   Implementation of the UN*X wildcards in C. So they are
 *   available in a portable way and can be used whereever
 *   needed.                                                  
 *
 * Author(s):
 *   Florian Schintke (schintke@gmx.de)
 */

#include <stdio.h>
#include <ctype.h>
#include "wildcards.h"

int asterisk (char **wildcard, char **test);
/* scans an asterisk */

int wildcardfit (char *wildcard, char *test)
{
  int fit = 1;
  
  for (; ('\000' != *wildcard) && (1 == fit) && ('\000' != *test); wildcard++)
    {
      switch (*wildcard)
        {
        case '?':
          test++;
          break;
        case '*':
          fit = asterisk (&wildcard, &test);
          /* the asterisk was skipped by asterisk() but the loop will */
          /* increment by itself. So we have to decrement */
          wildcard--;
          break;
        default:
          fit = (int) (toupper(*wildcard) == toupper(*test));
          test++;
        }
    }
  while ((*wildcard == '*') && (1 == fit)) 
    /* here the teststring is empty otherwise you cannot */
    /* leave the previous loop */ 
    wildcard++;
  return (int) ((1 == fit) && ('\0' == *test) && ('\0' == *wildcard));
}

int asterisk (char **wildcard, char **test)
{
  /* Warning: uses multiple returns */
  int fit = 1;

  /* erase the leading asterisk */
  (*wildcard)++; 
  while (('\000' != (**test))
         && (('?' == **wildcard) 
             || ('*' == **wildcard)))
    {
      if ('?' == **wildcard) 
        (*test)++;
      (*wildcard)++;
    }
  /* Now it could be that test is empty and wildcard contains */
  /* aterisks. Then we delete them to get a proper state */
  while ('*' == (**wildcard))
    (*wildcard)++;

  if (('\0' == (**test)) && ('\0' != (**wildcard)))
    return (fit = 0);
  if (('\0' == (**test)) && ('\0' == (**wildcard)))
    return (fit = 1); 
  else
    {
      /* Neither test nor wildcard are empty!          */
      /* the first character of wildcard isn't in [*?] */
      if (0 == wildcardfit(*wildcard, (*test)))
        {
          do 
            {
              (*test)++;
              /* skip as much characters as possible in the teststring */
              /* stop if a character match occurs */
              while ((toupper(**wildcard) != toupper(**test)) 
                     && ('\0' != (**test)))
                (*test)++;
            }
          while ((('\0' != **test))? 
                 (0 == wildcardfit (*wildcard, (*test))) 
                 : (0 != (fit = 0)));
        }
      if (('\0' == **test) && ('\0' == **wildcard))
        fit = 1;
      return (fit);
    }
}
