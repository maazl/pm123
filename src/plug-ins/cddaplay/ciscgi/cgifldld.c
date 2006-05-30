/*************************************************************************/
/* Name: cgifldld.c (value 2hr)						 */
/* Vers: 1.2								 */
/* Prog: Andrew Smith, feedback@cisc.com				 */
/*									 */
/* Desc: Load a named field from a string encoded in the standard CGI	 */
/*	 manner.							 */
/*									 */
/* NOTE: Functions STRSTR and STRLEN are assumed to be reentrant.  If    */
/*    	 not you will need to find reentrant equivalents or only use	 */
/*	 this code for non-reentrant purposes.				 */
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

/*****************************************************************/
/* PARAMETERS:	SRC    - A buffer containing entire record.	 */
/*		TXT   -  Name (such as USERID) to locate.	 */
/*		DST    - A buffer to store field value in.	 */
/*								 */
/* EFFECTS:	Destination is loaded with a string if the named */
/* 		field is found, otherwise it is set to null.	 */
/*								 */
/* RETURNS:	nothing.					 */
/*								 */
/* ASSUMPTIONS: The programmer can control the size of the input */
/*		fields and hence will always supply a DST large  */
/*		enough.  Otherwise core dumps may occur.	 */
/*								 */
/* Note: This function is case sensitive, you should use a	 */
/* function such as CGIUPPER to simplify usage.			 */
/*****************************************************************/
void cgifldld(char *src, char *txt, char *dst)
{
char *s,*t,*z;

   /* Initialize pointers to destination and source */
   t=dst;
   z=src;

   /*------------------------------------------*/
   /* Search for TXT followed by an equal sign */
   /*------------------------------------------*/
   do
   {
      /*--------------*/
      /* Is it there? */
      /*--------------*/
      s=strstr(z,txt);
      z=s+1;

      /*-------------------------------*/
      /* Ensure TXT is ENTIRE data tag */
      /*-------------------------------*/
      if ( s && (s==src || *(s-1)=='&') && *(s+strlen(txt))=='=' )
         break;

   } while ( s );

   /*---------------------------------------------------*/
   /* If found, load destination with raw info from URL */
   /*---------------------------------------------------*/
   if ( s )
   {
      for(s=s+strlen(txt)+1;*s && *s!='&';s++,t++)
         *t=*s;
   }
   *t='\0';
}


/************************************************/
/* Main, for demonstration and testing purposes */
/************************************************/
/* void main()
 * {
 * char *src="Dogfood=Alpo&Catfood=Nine+Lives&userid=ax\"xxb2";
 * char dst[128];
 * char txt[32];
 * 
 *    printf("String: '%s'\n",src);
 * 
 *    strcpy(txt,"Dogfood");
 *    cgifldld(src,txt,dst);
 *    printf("Dogfood = '%s'\n",dst);
 * 
 *    strcpy(txt,"Catfood");
 *    cgifldld(src,txt,dst);
 *    printf("Catfood = '%s'\n",dst);
 * 
 *    strcpy(txt,"catfood");
 *    cgifldld(src,txt,dst);
 *    printf("catfood = '%s'\n",dst);
 * 
 *    strcpy(txt,"userid");
 *    cgifldld(src,txt,dst);
 *    printf("userid = '%s'\n",dst);
 * }
 */
