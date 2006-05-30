/*************************************************************************/
/* Name: urlvalue.c (value 1hr)						 */
/* Vers: 1.0								 */
/* Prog: Andrew Smith, feedback@cisc.com				 */
/*									 */
/* Desc: Provide a simple encoding function to display field content	 */
/*	 values.							 */
/*									 */
/* NOTE: This function is reentrant and does not use or depend upon	 */
/*	 any standard library routines.					 */
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

#include <stdio.h>	/* For optional main section */

#include "cisrcl.h"	/* CIS reentrant C library header */

/*************************************************************/
/* To display values previously entered in a form we have to */
/* ensure certain characters are converted to special tags   */
/* or they will interfere with the form field itself.	     */
/*							     */
/* USES:	SRC	The string to be converted.	     */
/*		DST	Where to store converted value.	     */
/*		SIZE	Maximum storage capacity of DST.     */
/*							     */
/* RETURNS:	0	Successful conversion.		     */
/*		1	DST too small.			     */
/*							     */
/* EFFECTS:	DST	Upon successful conversion the DST   */
/*			parameter contains a null terminated */
/*			string ready for display.	     */
/*************************************************************/
int urlvalue(char *src, char *dst, int size)
{
static char *s,*t;
static int  used;

   used=0;
   t=dst;
   /*----------------------------*/
   /* Loop through source string */
   /*----------------------------*/
   for(s=src;*s;s++)
   {
      /*-----------------------*/
      /* Handle a greater than */
      /*-----------------------*/
      if ( *s=='>' )
      {
         if ( used+4>=size )
	    return 1;
         *t++ = '&';
         *t++ = 'g';
         *t++ = 't';
         *t++ = ';';
         used += 4;
      }
      /*--------------------*/
      /* Handle a less than */
      /*--------------------*/
      else if ( *s=='<' )
      {
         if ( used+4>=size )
	    return 1;
         *t++ = '&';
         *t++ = 'l';
         *t++ = 't';
         *t++ = ';';
         used += 4;
      }
      /*-----------------------*/
      /* Handle a double quote */
      /*-----------------------*/
      else if ( *s=='"' )
      {
         if ( used+6>=size )
	    return 1;
         *t++ = '&';
         *t++ = 'q';
         *t++ = 'u';
         *t++ = 'o';
         *t++ = 't';
         *t++ = ';';
         used += 6;
      }
      /*---------------------------*/
      /* Handle a normal character */
      /*---------------------------*/
      else
      {
         if ( used+1>=size )
            return 1;
         *t++ = *s;
         used++;
      }
   }
   *t='\0';

   return CIS_NOERROR;
}

/**************************************/
/* Main function for testing purposes */
/**************************************/
/* int main(int argc, char *argv[])
 * {
 * char src[512] = "<some \"value\">";
 * char dst[512];
 * int  size=512;
 * int  retval;
 * 
 *    retval=urlvalue(src,dst,size);
 *    if ( retval==CIS_NOERROR )
 *    {
 *       printf("Src: %s\n",src);
 *       printf("Dst: %s\n",dst);
 *    }
 *    else
 *    {
 *       printf("An error has occured.\n");
 *    }
 * }
 */
