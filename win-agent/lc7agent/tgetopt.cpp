/*****************************************************************************
 * _tgetopt.c: Unicode-enabled version of getopt,based on getopt.c by Mark K.Kim
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

#define _CRT_SECURE_NO_WARNINGS 1	   /* Demicrosoftification */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <tchar.h>

namespace _TGETOPT
{

TCHAR * optarg = NULL;
int optind = 0;
int opterr = 0;
int optopt = _T('?');

static TCHAR ** prev_argv = NULL;      /* Keep a copy of argv and argc to */
static int prev_argc = 0;              /*    tell if getopt params change */
static int argv_index = 0;             /* Option we're checking */
static int argv_index2 = 0;            /* Option argument we're checking */
static int opt_offset = 0;             /* Index into compounded "-option" */
static int dashdash = 0;               /* True if "--" option reached */
static int nonopt = 0;                 /* How many nonopts we've found */

static void increment_index()
{
	/* Move onto the next option */
	if(argv_index < argv_index2)
	{
		while(prev_argv[++argv_index] && prev_argv[argv_index][0] != '-'
				&& argv_index < argv_index2+1);
	}
	else argv_index++;
	opt_offset = 1;
}


/*
* Permutes argv[] so that the argument currently being processed is moved
* to the end.
*/
static int permute_argv_once()
{
	/* Movability check */
	if(argv_index + nonopt >= prev_argc) return 1;
	/* Move the current option to the end, bring the others to front */
	else
	{
		TCHAR* tmp = prev_argv[argv_index];

		/* Move the data */
		memmove(&prev_argv[argv_index], &prev_argv[argv_index+1],
				sizeof(TCHAR**) * (prev_argc - argv_index - 1));
		prev_argv[prev_argc - 1] = tmp;

		nonopt++;
		return 0;
	}
}


int _tgetopt(int argc, TCHAR** argv, const TCHAR* optstr)
{
	int c = 0;

	/* If we have new argv, or a null optstr, which resets us - reinitialize */
	if(prev_argv != argv || prev_argc != argc || !optstr)
	{
		/* Initialize variables */
		prev_argv = argv;
		prev_argc = argc;
		argv_index = 1;
		argv_index2 = 1;
		opt_offset = 1;
		dashdash = 0;
		nonopt = 0;
	}

	if(!optstr) 
		return -1;

	/* Jump point in case we want to ignore the current argv_index */
	getopt_top:

	/* Misc. initializations */
	optarg = NULL;

	/* Dash-dash check */
	if(argv[argv_index] && !_tcscmp(argv[argv_index], _T("--")))
	{
		dashdash = 1;
		increment_index();
	}

	/* If we're at the end of argv, that's it. */
	if(argv[argv_index] == NULL)
	{
		c = -1;
	}
	/* Are we looking at a string? Single dash is also a string */
	else if(dashdash || argv[argv_index][0] != _T('-') || !_tcscmp(argv[argv_index], _T("-")))
	{
		/* If we want a string... */
		if(optstr[0] == '-')
		{
			c = 1;
			optarg = argv[argv_index];
			increment_index();
		}
		/* If we really don't want it (we're in POSIX mode), we're done */
		else if(optstr[0] == '+' || _tgetenv(_T("POSIXLY_CORRECT")))
		{
			c = -1;

			/* Everything else is a non-opt argument */
			nonopt = argc - argv_index;
		}
		/* If we mildly don't want it, then move it back */
		else
		{
			if(!permute_argv_once()) goto getopt_top;
			else c = -1;
		}
	}
	/* Otherwise we're looking at an option */
	else
	{
		const TCHAR* opt_ptr = NULL;

		/* Grab the option */
		c = argv[argv_index][opt_offset++];

		/* Is the option in the optstr? */
		if(optstr[0] == _T('-')) opt_ptr = _tcschr(optstr+1, c);
		else opt_ptr = _tcschr(optstr, c);
		/* Invalid argument */
		if(!opt_ptr)
		{
			if(opterr)
			{
				_ftprintf(stderr, _T("%s: invalid option -- %c\n\r"), argv[0], c);
			}
			optopt = c;
			c = '?';

			/* Move onto the next option */
			increment_index();
		}
		/* Option takes argument */
		else if(opt_ptr[1] == ':')
		{
			/* ie, -oARGUMENT, -xxxoARGUMENT, etc. */
			if(argv[argv_index][opt_offset] != '\0')
			{
				optarg = &argv[argv_index][opt_offset];
				increment_index();
			}
			/* ie, -o ARGUMENT (only if it's a required argument) */
			else if(opt_ptr[2] != ':')
			{
				/* One of those "you're not expected to understand this" moment */
				if(argv_index2 < argv_index) argv_index2 = argv_index;
				while(argv[++argv_index2] && argv[argv_index2][0] == '-');
				optarg = argv[argv_index2];

				/* Don't cross into the non-option argument list */
				if(argv_index2 + nonopt >= prev_argc) optarg = NULL;

				/* Move onto the next option */
				increment_index();
			}
			else
			{
				/* Move onto the next option */
				increment_index();
			}

			/* In case we got no argument for an option with required argument */
			if(optarg == NULL && opt_ptr[2] != _T(':'))
			{
				optopt = c;
				c = _T('?');

				if(opterr)
				{
					_ftprintf(stderr,_T("%s: option requires an argument -- %c\n"),
							argv[0], optopt);
				}
			}
		}
		/* Option does not take argument */
		else
		{
			/* Next argv_index */
			if(argv[argv_index][opt_offset] == _T('\0'))
			{
				increment_index();
			}
		}
	}

	/* Calculate optind */
	if(c == -1)
	{
		optind = argc - nonopt;
	}
	else
	{
		optind = argv_index;
	}

	return c;
}

}