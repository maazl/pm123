/*************************************************************************/
/* Name: urldcode.c (value 2hr)						 */
/* Vers: 1.2								 */
/* Prog: Andrew Smith, feedback@cisc.com				 */
/*									 */
/* Desc: Provide a non-recursive and reentrant decoding utility for	 */
/*       CGI applications.						 */
/*									 */
/* NOTE: Functions STRCHR and TOUPPER are assumed to be reentrant.  If   */
/*       this is not so you will need to find reentrant equivalents.     */
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
#include <ctype.h>
#include <string.h>

#include "cisrcl.h"	/* CIS reentrant C library header */

/******************************************************************/
/* Passed any string value, decode from URL transmission.  We are */
/* given a source, which can be safely written into as the size   */
/* must be the same or smaller.  A non-zero represents success.   */
/*                                                                */
/* RETURNS: The length of the resulting decoded "string".         */
/*								  */
/* NOTE: You should use this function on each part of the URL     */
/* after it has been separated into fields.  Otherwise you will   */
/* not be able to use the & as a field separator.		  */
/******************************************************************/
int urldcode(char *str)
{
const char *CONVERT = "0123456789ABCDEF";

int  v;		/* Converted character value as an integer */
char *s,*t;	/* Source and destination */
char *w;	/* Where character occurs in conversion string */

   /*--------------------------------------*/
   /* Try to protect against dumb mistakes */
   /*--------------------------------------*/
   if ( !str )
      return 0;

   /*-------------------------------------------------------------*/
   /* Scan through string -- destination equals source originally */
   /*-------------------------------------------------------------*/
   t=str;
   for(s=str;*s;s++,t++)
   {
      if ( *s=='%' )
      {
         /*-----------------------------------------------------------*/
         /* Verify and use sixteens... check for "illegal" end of str */
         /*-----------------------------------------------------------*/
         s++;
         if ( !*s || (w=strchr(CONVERT,toupper(*s)))==NULL )
            return 0;
         v = (int)(w-CONVERT)*16;

         /*-------------------------------------------------------*/
         /* Verify and use ones... check for "illegal" end of str */
         /*-------------------------------------------------------*/
	 s++;
         if ( !*s || (w=strchr(CONVERT,toupper(*s)))==NULL )
            return 0;
         v += (int)(w-CONVERT);

         /*---------------------------------*/
         /* Append character to destination */
         /*---------------------------------*/
         *t = (char)v;
      }
      else if ( *s=='+' )
      {
         *t = ' ';
      }
      else
      {
         *t=*s;
      }
   }
   *t='\0';

   return t-str;
}

/**********************************************/
/* Demonstrate usage of the urldcode function */
/**********************************************/
/* void main()
 * {
 * char x[80];
 * 
 *    strcpy(x,"/htbin/test.cgi?equal=%3D&amp=%26&space=%20&tilde=%7E");
 *    if ( urldcode(x) )
 *       printf("Worked: '%s'\n",x);
 *    else
 *       printf("Failed\n");
 * 
 *    if ( urldcode(0) )
 *       printf("Worked: ???\n");
 *    else
 *       printf("Failed: (null)\n");
 * 
 *    strcpy(x,"%21Blah blah blah %2");
 *    if ( urldcode(x) )
 *       printf("Worked: ???\n");
 *    else
 *       printf("Failed: '%s'\n",x);
 * }
 */

