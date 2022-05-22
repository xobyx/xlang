#pragma once
#include "xlang_main.h"

typedef struct find
{
	bool isFind;
	char* bn;
	int size;
}find;
find c={0,0,0};
#define OVECCOUNT 30

inline char** get_lines_array(char* subject)
{
	
	pcre* re;
	const char* error;


	int erroffset;


	int ovector[OVECCOUNT];
	int subject_length = (int)strlen(subject);
	int rc;

	char* pattern = "^(.*)";//"^(.+)$";
	re = pcre_compile(
		pattern, /* the pattern */
		PCRE_MULTILINE, /* default options */
		&error, /* for error message */
		&erroffset, /* for error offset */
		NULL); /* use default character tables */

	/* Compilation failed: print the error message and exit */

	if (re == NULL)
	{
		printf("PCRE compilation failed at offset %d: %s\n", erroffset, error);
		return NULL;
	}


	int options = 0; /* Normally no options */
	int start_offset = ovector[1] = 0;
	char** r = (char**)malloc(sizeof(char**)*5000);
	memset(r, 0, 5000 - 1);
	/* Loop for second and subsequent matches */
	int ti = 0;
	for (;;)
	{
		/* Start at end of previous match */

		/* If the previous match was for an empty string, we are finished if we are
		at the end of the subject. Otherwise, arrange to run another match at the
		same point to see if a non-empty match can be found. */
		int start_offset = ovector[1];
		if (ovector[0] == ovector[1])
		{
			if (ovector[0] == subject_length) break;
			options = PCRE_NOTEMPTY | PCRE_ANCHORED;
		}

		/* Run the next matching operation */

		rc = pcre_exec(
			re, /* the compiled pattern */
			NULL, /* no extra data - we .type_idn't. study the pattern */
			subject, /* the subject string */
			subject_length, /* the length of the subject */
			start_offset, /* starting offset in the subject */
			options, /* options */
			ovector, /* output vector for substring information */
			OVECCOUNT); /* number of elements in the output vector */

		/* This time, a result of NOMATCH isn't an error. If the value in "options"
		is zero, it just means we have found all possible matches, so the loop ends.
		Otherwise, it means we have failed to find a non-empty-string match at a
		point where there was a previous empty-string match. In this case, we do what
		Perl does: advance the matching position by one, and continue. We do this by
		setting the "end of previous match" offset, because that is picked up at the
		top of the loop as the point at which to start again. */

		if (rc == PCRE_ERROR_NOMATCH)
		{
			if (options == 0) break;
			ovector[1] = start_offset + 1;
			continue; /* Go round the loop again */
		}

		/* Other matching errors are not recoverable. */

		if (rc < 0)
		{
			printf("Matching error %d\n", rc);
			pcre_free(re); /* Release memory used for the compiled pattern */
			return NULL;
		}

		/* Match succeded */

		//printf("\nMatch succeeded again at offset %d\n", ovector[0]);

		/* The match succeeded, but the output vector wasn't big enough. */

		if (rc == 0)
		{
			rc = OVECCOUNT / 3;
			printf("ovector only has room for %d captured substrings\n", rc - 1);
		}

		/* As before, show substrings stored in the output vector by number, and then
		also any named substrings. */
		//i======groub

		char* substring_start = subject + ovector[0];
		int substring_length = ovector[1] - ovector[0];
		r[ti] =(char*) malloc(sizeof(char)*substring_length);
		sprintf(r[ti], "%.*s", substring_length, substring_start);
		ti++;
		//parse_var(i,substring_start,substring_length);

		//l++;
	}
	pcre_free(re);
	return r;
}
find* isMatchF(char* pattern, char* subject, int flags)
{
	pcre* re;
	const char* error;

//	unsigned char* name_table;
	int erroffset;
	int find_all;
//	int namecount;
//	int name_entry_size;
	int ovector[OVECCOUNT];
	int subject_length;
	int rc, i;


	/**************************************************************************
	* First, sort out the command line. There is only one possible option at  *
	* the moment, "-g" to request repeated matching to find all occurrences,  *
	* like Perl's /g option. We set the variable find_all to a non-zero value *
	* if the -g option is present. Apart from that, there must be exactly two *
	* arguments.                                                              *
	**************************************************************************/

	find_all = 1;


	subject_length = (int)strlen(subject);


	/*************************************************************************
	* Now we are going to compile the regular expression pattern, and handle *
	* and errors that are detected.                                          *
	*************************************************************************/

	re = pcre_compile(
		pattern, /* the pattern */
		flags, /* default options */
		&error, /* for error message */
		&erroffset, /* for error offset */
		NULL); /* use default character tables */

	/* Compilation failed: print the error message and exit */

	if (re == NULL)
	{
		char* rb = (char*) malloc(sizeof(char)*300);
		sprintf(rb, "PCRE compilation failed at offset %d: %s\n", erroffset, error);
		find* x = (find*)malloc(sizeof(find));
		x->isFind=false;
		x->bn=rb;
		//	new find {false,rb};
		//return x;
	}


	/*************************************************************************
	* If the compilation succeeded, we call PCRE again, in order to do a     *
	* pattern match against the subject string. This does just ONE match. If *
	* further matching is needed, it will be done below.                     *
	*************************************************************************/

	rc = pcre_exec(
		re, /* the compiled pattern */
		NULL, /* no extra data - we .type_idn't study the pattern */
		subject, /* the subject string */
		subject_length, /* the length of the subject */
		0, /* start at offset 0 in the subject */
		0, /* default options */
		ovector, /* output vector for substring information */
		OVECCOUNT); /* number of elements in the output vector */

	/* Matching failed: handle error cases */
	find* xv=(find*)malloc(sizeof(find));
	if (rc < 0)
	{
		switch (rc)
		{
		case PCRE_ERROR_NOMATCH:
			xv->bn = "No match\n";
			xv->isFind = false;
			break;
			/*
			Handle other special cases if you like
			*/
		default:
		{
			char* v = (char*) malloc(sizeof(char)*100);
			sprintf(v, "Matching error %d\n", rc);

			xv->isFind = false;
			xv->bn = v;
			break;
		}
		}

		pcre_free(re);
		return xv;/* Release memory used for the compiled pattern */
	}

	/* Match succeded */

	//printf("\nMatch succeeded at offset %d\n", ovector[0]);


	/*************************************************************************
	* We have found the first match within the subject string. If the output *
	* vector wasn't big enough, say so. Then output any substrings that were *
	* captured.                                                              *
	*************************************************************************/

	/* The output vector wasn't big enough */

	if (rc == 0)
	{
		rc = OVECCOUNT / 3;
		printf("ovector only has room for %d captured substrings\n", rc - 1);
	}

	/* Show substrings stored in the output vector by number. Obviously, in a real
	application you might want to do things other than print them. */


	char* tx = NULL;
	for (i = 1; i < 2; i++)
	{
		char* substring_start = subject + ovector[2 * i];
		int substring_length = ovector[2 * i + 1] - ovector[2 * i];

		tx = (char*)malloc(substring_length + 1);
		memset(tx, 0, substring_length + 1);
		sprintf(tx, "%.*s", substring_length, substring_start);

		xv->bn = tx;
		xv->size = substring_length;
		xv->isFind = true;
		pcre_free(re);
		return xv;
	}
	return NULL;
}

find* match(char* pattern, char* subject)
{
	return isMatchF(pattern, subject, 0);
}
