/*************************************************************************/
/* Name: urlncode.c (value 2hr)						 */
/* Vers: 1.2								 */
/* Prog: Andrew Smith, feedback@cisc.com				 */
/*									 */
/* Desc: Provide a non-recursive and reentrant encoding utility for	 */
/*       CGI applications.						 */
/*									 */
/* NOTE: Function STRCHR is assumed to be reentrant.  If this is not so	 */
/*       you will need to find a reentrant equivalent.			 */
/*									 */
/* Copyright 1995,1996 Custom Innovative Solutions, Corp.                */
/*                                                                       */
/* If you are earning an income while viewing our material you should    */
/* probably be paying fees to comply with our copyright requirements.    */
/* However, if you are a student you are likely exempt.                  */
/*                                                                       */
/* Please visit our site to see our licensing policy and how to report   */
/* usage:                                                                */
/*                                                                       */
/*    http://www.cisc.com/copyright.html                                 */
/*    http://www.cisc.com/reporting.html                                 */
/*                                                                       */
/* If you find our material helpful, please link to our home page.       */
/*************************************************************************/

#include <stdio.h>
#include <string.h>

#include "cisrcl.h"	/* CIS reentrant C library header */

/******************************************************************/
/* Passed any string value, encode for URL transmission.  We are  */
/* given a source, a destination and the size of the destination. */
/* Upon completion we return a 1 for success and a 0 for failure  */
/* due to destination size being too small.                       */
/*								  */
/* NOTE: You should use this function on each part of the URL     */
/* before combining them.  Otherwise you will encode the '&' that */
/* separates each individual part.                                */
/******************************************************************/
int urlncode(char *src, char *dst, int size)
{
const char *ENCLIST = "!\"#$%&'(),:;<=>?{\\}~+";
const char *CONVERT = "0123456789ABCDEF";

char *s,*t;
int  used;

int  n;		/* Integer representation of a character */
int  a;		/* Number of 16's for integer representation */

   /*------------*/
   /* Initialize */
   /*------------*/
   t=dst;
   used=0;

   /* Try to protect against dumb mistakes */
   if ( !dst )
      return 0;

   /*-------------------------------*/
   /* Process each source character */
   /*-------------------------------*/
   for (s=src;*s;s++)
   {
      /* Don't cause a core dump if we can avoid it */
      if ( used+1 >= size )
      {
         *t='\0';
         return 0;
      }

      /*---------------------------------------*/
      /* Test for and handle simple characters */
      /*---------------------------------------*/
      if ( *s==' ' )
      {
         *t++ = '+';
         used++;
      }
      else if ( !strchr(ENCLIST,*s) )
      {
         *t++ = *s;
         used++;
      }
      else
      {
         /* Don't produce a core dump if we can avoid it */
         if ( used+4 >= size )
         {
            *t='\0';
            return 0;
         }

         /*------------------------------*/
         /* Encode the current character */
         /*------------------------------*/

         /* Count number of 16s and get remainder */
         *t++ = '%';
         a=0;
         n=(int)*s;
         while (n>=16)
         {
            n -= 16;
            a++;
         }

         /* Add 16's and remainder to encoding */
         *t++ = CONVERT[a];
         *t++ = CONVERT[n];

         /* We used three more for encoding */
         used += 3;
      }
   }

   /* We didn't run out of space... return success (space used) */
   *t='\0';
   return used;
}

/**********************************************/
/* Demonstrate usage of the urlncode function */
/**********************************************/
/* void main()
 * {
 * char x[80];
 * char y[80];
 * int  n=40;
 * 
 *    strcpy(x,"This! is a URL string & its far too long.");
 *    if ( urlncode(x,y,n) )
 *       printf("  Worked: '%s'\n",y);
 *    else
 *       printf("Too Long: '%s'\n",y);
 * 
 *    strcpy(x,"This URL will %WORK%"); 
 *    if ( urlncode(x,y,n) )
 *       printf("  Worked: '%s'\n",y);
 *    else
 *       printf("Too Long: '%s'\n",y);
 * }
 */

