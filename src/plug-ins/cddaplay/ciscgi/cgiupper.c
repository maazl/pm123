/*************************************************************************/
/* Name: cgiupper.c	(value 0.5hrs)					 */
/* Vers: 1.0								 */
/* Prog: Andrew Smith, feedback@cisc.com				 */
/*                                                                       */
/* Desc: Convert only the "tags" within a URL to uppercase.		 */
/*									 */
/* NOTE: This function uses standard library routine TOUPPER.  If this	 */
/*	 routine is not reentrant you will need to find a reentrant	 */
/*	 equivalent, for use in a reentrant application.		 */
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

/*********************/
/* Standard Includes */
/*********************/
#include <stdio.h>
#include <string.h>
#include <ctype.h>

/******************/
/* Local Includes */
/******************/
#include "cisrcl.h"

/********************************************************************/
/* Convert query or content string tags to uppercase, leaving data  */
/* sections intact.  Note: This function should be used BEFORE      */
/* decoding the URL data!					    */
/*								    */
/* PARAMETERS: 	s	Text to modify.				    */
/*								    */
/* EFFECTS:	s	Tags (field names) are converted within s.  */
/********************************************************************/
void cgiupper(char *s)
{
int tag;

   tag=1;
   for (;*s;s++)
   {
      if ( tag )
         *s = toupper(*s);
      if ( *s=='=' )		/* We just got last character */
	 tag=0;
      if ( *s=='&' )		/* Next character will be first */
	 tag=1;
   }
}

/******************************************************/
/* Application entry point for demonstration purposes */
/******************************************************/
/* void main()
 * {
 * char a[]="field1=Test&FielD2=Another Test";
 * 
 *    printf("Before: '%s'\n",a);
 *    cgiupper(a);
 *    printf("After: '%s'\n",a);
 * }
 */
