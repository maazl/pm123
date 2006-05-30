#ifndef CIS_RCL
#define CIS_RCL

/*************************************************************************/
/* Name: cisrcl.h (value 2hr)						 */
/* Vers: 1.4								 */
/* Prog: Andrew Smith                                                    */
/*									 */
/* Desc: Define required structures and values.				 */
/*									 */
/* Copyright 1995,1996 Custom Innovative Solutions, Corp.		 */
/*									 */
/* If you are earning an income while viewing our material you should	 */
/* probably be paying fees to comply with our copyright requirements.	 */
/* However, if you are a student you are likely exempt.			 */
/*									 */
/* Please visit our site to see our licensing policy and how to report	 */
/* usage:								 */
/*									 */
/*    http://www.cisc.com/copyright.html				 */
/*    http://www.cisc.com/reporting.html				 */
/*									 */
/* If you find our material helpful, please link to our home page.	 */
/*************************************************************************/

/***************************/
/* Error and Status Values */
/***************************/
#define CIS_NOERROR 	0		/* No errors */
#define CIS_BADNAME	1		/* Missing, bad or invalid name */

#define CIS_BADMATDIM	100		/* Invalid matrix dimensions */

/****************/
/* LIBRARY INFO */
/****************/
#define _CIS_RCL "CISRCLv1.4"

/*********************************/
/* PARAMETER HANDLING STRUCTURES */
/*********************************/

struct parm_data
{
   char *name;		/* Name of parameter (search tag) */
   char *type;		/* User/Programmer defined parameter type */
   char *desc;		/* Description of parameter for usage help */
   char *value;		/* A string containing value as found or defaulted */

   int  found;		/* Was this parameter retrieved? */
};

/******************/
/* CGI STRUCTURES */
/******************/

struct cgi_data
{
   char *query;		/* Query string */
   char *pinfo;		/* Path info */
   char *ctent;		/* Content */
   char *rname;		/* Remote user if authenticated */
   char *sname;		/* Server name */

   int  port;		/* Server port number */
   int  auth;		/* Authenticated? */
};

/***********************/
/* GRAPHICS STRUCTURES */
/***********************/

/*---------------------------------------------*/
/* Structure to hold a graphics (small) matrix */
/*---------------------------------------------*/
struct grm44
{
   int    m,n;		/* Dimensions of the matrix */
   double el[4][4];	/* Elements of matrix */
};

/*--------------------------------------------------------*/
/* Structure to hold view coordinates (in integer values) */
/*--------------------------------------------------------*/
struct clip_view
{
   long top;
   long left;
   long bottom;
   long right;
};

/*------------------------------------------------------*/
/* Structure to hold a single point (in integer values) */
/*------------------------------------------------------*/
struct clip_point
{
   long x,y;
};

/*------------------------------------------------------*/
/* Structure to hold a line and other basic information */
/*------------------------------------------------------*/
struct clip_line
{
   struct clip_point p;
   struct clip_point q;

   double slope;	/* "m" in y=mx+b */
   double offset;	/* "b" in y=mx+b */
};

/*-----------------------------------------------*/
/* Structure exclusively for clip function usage */
/*-----------------------------------------------*/
struct clip_work
{
   short  subline;		/* Testing which subline (if any)? */
   short  vplane;		/* Which plane violated (if any)? */

   short  pleft,qleft;		/* Plane violation flags */
   short  pright,qright;
   short  ptop,qtop;
   short  pbottom,qbottom;

   double dx,dy;		/* Calculated clip point */
   long   oqx,oqy;		/* Previous endpoint */
};

#endif
