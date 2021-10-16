/*****************************************************************************
 * _tgetopt.h: Unicode-enabled version of getopt,based on getopt.c by Mark K.Kim
 *           Modifications by DilDog (dildog@l0pht.com)
 *  
 * getopt.c - competent and free getopt library.
 * Mark K. Kim (mark@cbreak.org)
 * http://www.cbreak.org/
 *
 * This is a free software you can use, modify, and include in your own
 * program(s).  However, by using this software in any aforementioned manner,
 * you agree to relieve the author of this software (me, Mark K. Kim,)
 * from any liability.  Please take a special note that I do not claim
 * this software to be fit for any purpose, though such meaning may be
 * implied in the rest of the software.
 *
 */
#ifndef __INC_TGETOPT_H
#define __INC_TGETOPT_H

namespace _TGETOPT
{
	extern TCHAR * optarg;
	extern int optind;
	extern int opterr;
	extern int optopt;

	int _tgetopt(int argc, TCHAR ** argv, const TCHAR * optstr);
}


#endif /* TGETOPT_H_ */


/* vim:ts=3
*/
