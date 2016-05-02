/*******************************************************************************
* vfind.c
*	Search for files matching given criteria in a directory tree.
*
* Notes
*	This program is intended for Microsoft Win32.
*
*	This file can be compiled using Microsoft C with the command:
*	    cl -Ge -Ot vfind.c -I \include \lib\fpattern.obj
*
* History
*	2.0 1994-08-23, drt.
*	Second edition.
*
*	2.1 1994-08-25, drt.
*	Added the '-d', '-s', '-u', and '-g' options.
*
*	2.2 1994-08-25, drt.
*	Added the '-t' option.
*
*	2.3 1994-09-13, drt.
*	Added the '-?' option.
*
*	2.4 1995-01-08, drt.
*	Show writable (non-read-only) permission as 'w'.
*
*	2.5 1995-05-09, drt.
*	Uses '/' instead of '\' as path delimiter.
*
*	2.6 1995-07-01, drt.
*	Go back (2.5) to using '\' (this is DOS after all).
*
*	3.0 1996-01-11, drt.
*	Changed date/time format.
*	Changed date output to show 'CCYY'.
*
*	3.50 1996-02-22, drt.
*	Added support for Win32 long file names.
*
*	3.60 1996-12-31, drt.
*	Added a block byte count.
*	Added more '-t' options, and change exclusion rules.
*	Added the '-A' option.
*	Added the (undocumented) '-D' option.
*
*	3.70 1997-01-03, drt.
*	Uses "fpattern.h" filename pattern matching functions.
*	Added the '-n' summary option.
*	Added the '-v' verbose option.
*
*	3.71 1997-01-13, drt.
*	Uses fixed fpattern functions.
*	Disabled Win32 long filename handling, since it doesn't work yet.
*
*	4.0 1997-01-26, drt.
*	Fixed long filename handling.
*	Added the '-m' short name option.
*	Added the '-r' nonrecursive options.
*
*	4.1 1998-06-17, drt.
*	Changed dates to be ISO-8601 format, "CCYY-MM-DD".
*
*	5.0, 2002-05-08, David R Tribble <david@tribble.com>.
*	Converted from Win16 to Win32 compilation (uses <windows.h>).
*	Changed tabs to be 8 spaces wide from 4 spaces wide.
*	Added the '-w' option.
*
*	5.1, 2002-08-28, David R Tribble <david@tribble.com>.
*	Fixed the '-tf' option.
*
*	5.2, 2002-11-15, David R Tribble <david@tribble.com>.
*	Fixed the initialization of the search info structure in 'search()'.
*
*	5.3, 2008-09-03, David R Tribble <david@tribble.com>.
*	Allowed multiple '-d' args.
*
*	5.4, 2011-03-13, David R Tribble <david@tribble.com>.
*	Added the '-f' (file name only) option.
*
* @(#)/drt/src/cmd/vfind.c $Revision: 5.4 $ $Date: 2011-03-13 $
*
* Copyright ©1994-2011 by David R. Tribble, all rights reserved.
*/


/*==============================================================================
* Identification
*/

#define ID_C_TITLE	"vfind"
#define ID_C_DATE	"2011-03-13"
#define ID_C_VERS	"5.4"
#define ID_C_AUTH	"David R. Tribble"
#define ID_C_EMAIL	"<david@tribble.com>"
#define ID_C_HOME	"<http://david.tribble.com>"
#define ID_C_DIR	"/drt/src/cmd"
#define ID_C_FILE	__FILE__
#define ID_C_COMP	__DATE__ " " __TIME__

static const char	COPYRIGHT[] =
    "Copyright (c)1994-2011 by " ID_C_AUTH ".";
    /* Note: The copyright symbol '©' is '\251' */

static const char	ID[] =
    "@(#)$Header: " ID_C_DIR "/" ID_C_FILE " " ID_C_VERS " " ID_C_DATE " $";

static const char	ID_BUILT[] =
#if DEBUG-0 > 0
    "@(#)$Built: " ID_C_COMP " +DEBUG $";
#else
    "@(#)$Built: " ID_C_COMP " $";
#endif

static const char	ID_AUTHOR[] =
    "@(#)$Author: " ID_C_AUTH " " ID_C_EMAIL " $";

static const char	ID_COMPILE[] =
    "[" ID_C_TITLE ", " ID_C_VERS " " ID_C_DATE "] <" ID_C_COMP ">";

static const char	ID_TITLE[] =	ID_C_TITLE;
static const char	ID_DATE[] =	ID_C_DATE;
static const char	ID_VERS[] =	ID_C_VERS;
static const char	ID_AUTH[] =	ID_C_AUTH;
static const char	ID_FILE[] =	ID_C_FILE;

static const char *	prog = ID_TITLE;


/*==============================================================================
* System includes
*/

#include <ctype.h>
#include <iso646.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <dos.h>

#define WIN32_LEAN_AND_MEAN
#include <windows.h>


/*==============================================================================
* Local includes
*/

#include "fpattern.h"


/*==============================================================================
* Manifest types and constants
*/

/* Boolean constants */
#ifndef bool
 #define bool		int
#endif

#ifndef false
 #define false		0
#endif

#ifndef true
 #define true		1
#endif


#ifndef uint64_t
 typedef unsigned __int64	uint64_t;
#endif


/* DOS file attribute codes */
#define A_NORMAL	FILE_ATTRIBUTE_NORMAL
#define A_NORMAL2	0x00000000
#define A_READONLY	FILE_ATTRIBUTE_READONLY
#define A_HIDDEN	FILE_ATTRIBUTE_HIDDEN
#define A_SYSTEM	FILE_ATTRIBUTE_SYSTEM
#define A_VOLUME	0x00000008
#define A_DIRECTORY	FILE_ATTRIBUTE_DIRECTORY
#define A_ARCHIVE	FILE_ATTRIBUTE_ARCHIVE
#define A_TEMPORARY	FILE_ATTRIBUTE_TEMPORARY
#define A_COMPRESSED	FILE_ATTRIBUTE_COMPRESSED
#define A_OFFLINE	FILE_ATTRIBUTE_OFFLINE

/*+OLD
#define ATTR_DFL \
    (A_READONLY|A_HIDDEN|A_SYSTEM|A_DIRECTORY|A_ARCHIVE|A_NORMAL\
    |A_COMPRESSED|A_TEMPORARY|A_OFFLINE)
*/
#define ATTR_DFL	0x000FFFFF


/* DOS filename constants */
#define SEP_STR		"\\"
#define SEP_CHAR	'\\'
#define WILD_DOS	"*.*"
#define WILD_WIN32	"*"

#define BLOCKSIZE	512	/* DOS low-level block size		*/


/* Program exit codes */
enum ExitCodes
{
    RC_OKAY =		0,	/* Successful				*/
    RC_ERR =		1,	/* An error occurred			*/
    RC_USAGE =		255	/* Invalid usage			*/
};


/*==============================================================================
* Debug macros
*/

#ifndef DEBUG
 #define DEBUG	0
#endif

#if DEBUG-0 <= 0
 #undef  DEBUG
 #define DEBUG	0
#endif

#if DEBUG
 #define DL(e)	(opt_debug ? (void)(e) : (void)0)
#else
 #define DL(e)	((void)0)
#endif


/*==============================================================================
* Private types
*/

struct search_info
{
    HANDLE		fhandle;	/* File search handle		*/
    struct _WIN32_FIND_DATAA
			fdata;		/* Search info			*/
};


struct Opt
{
    bool		o_verbose;	/* Print verbose messages	*/
    bool		o_longlist;	/* Print long listing		*/
    bool		o_summary;	/* Print summary counts		*/
    bool		o_all;		/* Search for all entries	*/
    bool		o_almostall;	/* Search for all but "."	*/
    bool		o_nosubdirs;	/* Don't search subdirs		*/
    bool		o_dosnames;	/* Use short MS-DOS names	*/
    bool		o_nameonly;	/* Names without drive/path	*/
    const char *	o_date1;	/* Date criteria (1st)		*/
    const char *	o_date2;	/* Date criteria (2nd)		*/
    struct _FILETIME	o_date1_v;	/* Date/time stamp criteria	*/
    struct _FILETIME	o_date2_v;	/* Date/time stamp criteria	*/
    const char *	o_size;		/* Size criteria		*/
    uint64_t		o_size_v;	/* Size criteria		*/
    const char *	o_type;		/* Type criteria		*/
    const char *	o_user;		/* User-ID criteria		*/
    const char *	o_group;	/* Group-ID criteria		*/
};


struct Count
{
    long		c_dir;		/* Directories			*/
    long		c_file;		/* Files			*/
    long		c_hidden;	/* Hidden entries		*/
    uint64_t		c_bytes;	/* Bytes			*/
    uint64_t		c_blocks;	/* Blocks (512 bytes)		*/
};


/*==============================================================================
* Private variables
*/

static struct Opt	opt;
static struct Count	count;
static char		fsinfo_buf[256];


/*==============================================================================
* Public variables
*/

bool			opt_debug = false;


/*==============================================================================
* Functions
*/

/*------------------------------------------------------------------------------
* findfirst32()
*
* Returns
*	True on success, otherwise false.
*/

static bool findfirst32(const char *pat, unsigned int attr,
    struct search_info *info)
{
    /* Start the file searching */
    info->fhandle = FindFirstFileA(pat, &info->fdata);

    /* Handle DOS short filenames */
    if (opt.o_dosnames  and  info->fdata.cAlternateFileName[0] != '\0')
        memcpy(info->fdata.cFileName,
            info->fdata.cAlternateFileName,
            sizeof(info->fdata.cAlternateFileName));

    DL(printf("first: \"%s\"\n", info->fdata.cFileName));
    return (info->fhandle != NULL);
}


/*------------------------------------------------------------------------------
* findnext32()
*
* Returns
*	True on success, otherwise false.
*/

static bool findnext32(struct search_info *info)
{
    bool	found;

    /* Continue the file searching */
    found = FindNextFile(info->fhandle, &info->fdata);

    if (not found)
    {
        /* Close the search handle */
        FindClose(info->fhandle);
        info->fhandle = NULL;
    }
    else if (opt.o_dosnames  and  info->fdata.cAlternateFileName[0] != '\0')
    {
        /* Handle DOS short filenames */
        memcpy(info->fdata.cFileName,
            info->fdata.cAlternateFileName,
            sizeof(info->fdata.cAlternateFileName));
    }

    DL(printf("next: \"%s\"\n", info->fdata.cFileName));
    return (found);
}


/*------------------------------------------------------------------------------
* compare_filetimes()
*	Compare two file timestamps.
*
* Returns
*	A signed value which is:
*	- positive if date 'a' is later than date 'b',
*	- negative if date 'a' is earlier than date 'b',
*	- zero if date 'a' is the same as date 'b'.
*/

static int compare_filetimes(const struct _FILETIME *a,
    const struct _FILETIME *b)
{
    /* Compare the two file timestamps */
    if (a->dwHighDateTime > b->dwHighDateTime)
        return (+1);
    if (a->dwHighDateTime < b->dwHighDateTime)
        return (-1);
    if (a->dwLowDateTime > b->dwLowDateTime)
        return (+1);
    if (a->dwLowDateTime < b->dwLowDateTime)
        return (-1);
    return (0);
}


/*------------------------------------------------------------------------------
* include_entry()
*	Determine if file entry info 'info' matches selection specifications
*	and should be printed or not.
*
* Returns
*	True if entry info should be printed, otherwise false.
*/

static bool include_entry(struct _WIN32_FIND_DATAA *info)
{
    /* Check the first date spec */
    if (opt.o_date1 != NULL)
    {
        int	cmp;

        cmp = compare_filetimes(&info->ftLastWriteTime, &opt.o_date1_v);
        switch (opt.o_date1[0])
        {
        case '+':
            if (cmp < 0)
                goto exclude;
            break;

        case '-':
            if (cmp > 0)
                goto exclude;
            break;

        case '!':
            if (cmp == 0)
                goto exclude;
            break;

        case '=':
        default:
            if (cmp != 0)
                goto exclude;
            break;
        }
    }

    /* Check the second date spec */
    if (opt.o_date2 != NULL)
    {
        int	cmp;

        cmp = compare_filetimes(&info->ftLastWriteTime, &opt.o_date2_v);
        switch (opt.o_date2[0])
        {
        case '+':
            if (cmp < 0)
                goto exclude;
            break;

        case '-':
            if (cmp > 0)
                goto exclude;
            break;

        case '!':
            if (cmp == 0)
                goto exclude;
            break;

        case '=':
        default:
            if (cmp != 0)
                goto exclude;
            break;
        }
    }

    /* Check the size spec */
    if (opt.o_size != NULL)
    {
        uint64_t	sz;

        /* Get the file size */
        sz = ((uint64_t)info->nFileSizeHigh << 32) + info->nFileSizeLow;

        /* Check the file size criterion */
        switch (opt.o_size[0])
        {
        case '+':
            if (sz < opt.o_size_v)
                goto exclude;
            break;

        case '-':
            if (sz > opt.o_size_v)
                goto exclude;
            break;

        case '!':
            if (sz == opt.o_size_v)
                goto exclude;
            break;

        case '=':
        default:
            if (sz != opt.o_size_v)
                goto exclude;
        }
    }

    /* Check the entry type */
    if (opt.o_type != NULL)
    {
        DWORD	at;
   	bool	incl = false;
        bool	excl = false;

        at = info->dwFileAttributes;

        /* Check for or'ed criteria */
        if (strchr(opt.o_type, 'a') != NULL  and  (at & A_ARCHIVE) != 0)
            incl = true;
        if (strchr(opt.o_type, 'c') != NULL  and  (at & A_COMPRESSED) != 0)
            incl = true;
        if (strchr(opt.o_type, 'd') != NULL  and  (at & A_DIRECTORY) != 0)
            incl = true;
        if (strchr(opt.o_type, 'f') != NULL  and  (at & A_NORMAL) != 0)
            incl = true;
        if (strchr(opt.o_type, 'f') != NULL
            and  (at & (A_DIRECTORY|A_VOLUME)) == 0)
            incl = true;
        if (strchr(opt.o_type, 'h') != NULL  and  (at & A_HIDDEN) != 0)
            incl = true;
        if (strchr(opt.o_type, 'r') != NULL  and  (at & A_READONLY) != 0)
            incl = true;
        if (strchr(opt.o_type, 's') != NULL  and  (at & A_SYSTEM) != 0)
            incl = true;
        if (strchr(opt.o_type, 't') != NULL  and  (at & A_TEMPORARY) != 0)
            incl = true;
        if (strchr(opt.o_type, 'v') != NULL  and  (at & A_VOLUME) != 0)
            incl = true;
        if (strchr(opt.o_type, 'w') != NULL  and  (at & A_READONLY) == 0)
            incl = true;

#ifdef not_yet_supported
        /* Check for and'ed (required) criteria */
        if (strchr(opt.o_type, 'A') != NULL  and  (at & A_ARCHIVE) == 0)
            excl = true;
        if (strchr(opt.o_type, 'C') != NULL  and  (at & A_COMPRESSED) == 0)
            excl = true;
        if (strchr(opt.o_type, 'D') != NULL  and  (at & A_DIRECTORY) == 0)
            excl = true;
        if (strchr(opt.o_type, 'F') != NULL  and  (at & A_NORMAL) == 0)
            excl = true;
        if (strchr(opt.o_type, 'f') != NULL
            and  (at & (A_DIRECTORY|A_VOLUME)) != 0)
            excl = true;
        if (strchr(opt.o_type, 'H') != NULL  and  (at & A_HIDDEN) == 0)
            excl = true;
        if (strchr(opt.o_type, 'R') != NULL  and  (at & A_READONLY) == 0)
            excl = true;
        if (strchr(opt.o_type, 'S') != NULL  and  (at & A_SYSTEM) == 0)
            excl = true;
        if (strchr(opt.o_type, 'T') != NULL  and  (at & A_TEMPORARY) == 0)
            excl = true;
        if (strchr(opt.o_type, 'V') != NULL  and  (at & A_VOLUME) == 0)
            excl = true;
        if (strchr(opt.o_type, 'W') != NULL  and  (at & A_READONLY) != 0)
            excl = true;
#endif

        if (opt.o_type[0] != '!'  and  (not incl  or  excl))
            goto exclude;
        else if (opt.o_type[0] == '!'  and  (incl  and  not excl))
            goto exclude;
    }

    /* Check the entry name and its visibility */
    if (opt.o_all)
    {
        /* Include all entries */
    }
    else if (info->dwFileAttributes & (A_SYSTEM|A_HIDDEN))
    {
        /* Hidden filename */
        if (not opt.o_almostall  and
                (strcmp(info->cFileName, ".") == 0  or
                strcmp(info->cFileName, "..") == 0))
            goto exclude;
    }
    else if (not opt.o_almostall)
    {
        /* Check for a dotted name */
        if (strcmp(info->cFileName, ".")  == 0  or
            strcmp(info->cFileName, "..") == 0)
            goto exclude;
    }

#ifdef is_unsupported
|   /* Check the owner user name */
|   if (opt.o_user != NULL)
|   {
|       ...
|   }
#endif

#ifdef is_unsupported
|   /* Check the owner group name */
|   if (opt.o_group != NULL)
|   {
|       ...
|   }
#endif

include:
    /* Include the entry */
    DL(printf("|incl %08lX '%s'\n",
        (long)info->dwFileAttributes, info->cFileName));
    return (true);

exclude:
    /* Exclude the entry */
    DL(printf("|excl %08lX '%s'\n",
        (long)info->dwFileAttributes, info->cFileName));
    return (false);
}


/*------------------------------------------------------------------------------
* s_attrib()
*	Convert file attribute 'at' into human-readable string form.
*
* Returns
*	Pointer to static string area.
*/

static const char * s_attrib(unsigned int at, char *buf)
{
    int		i;

    /* Assemble individual bits of attribute */
    i = 0;

    buf[i++] = (at & A_DIRECTORY ?  'd' :
                (at & A_VOLUME ? 'v' :
                '-'));

    if (at & 0xFFFF0000)
        buf[i++] = '#';
/*
    if (at & 0x00080000)
        buf[i++] = '8';
    if (at & 0x00040000)
        buf[i++] = '4';
    if (at & 0x00020000)
        buf[i++] = '2';
    if (at & 0x00010000)
        buf[i++] = '1';
    if (at & 0x00000040)
        buf[i++] = '6';
*/

    buf[i++] = (at & A_COMPRESSED ? 'c' : '-');
    buf[i++] = (at & A_SYSTEM ?     's' : '-');
    buf[i++] = (at & A_HIDDEN ?     'h' : '-');
    buf[i++] = (at & A_ARCHIVE   ?  'a' : '-');
    buf[i++] = (at & A_TEMPORARY ?  't' : '-');
    buf[i++] = (at & A_READONLY ?   'r' : 'w');

    buf[i++] = '\0';

    return (buf);
}


/*------------------------------------------------------------------------------
* s_datetime()
*	Convert filestamp 'ft' into a human-readable string form containing the
*	date and time.
*
* Returns
*	Pointer to static string area containing the date and time in the format
*	"YYYY-MM-DD HH:MM:SS".
*/

static const char * s_datetime(const struct _FILETIME *ft, char *buf)
{
    struct _SYSTEMTIME	st;

    /* Convert filestamp date to broken-down "system" time */
    FileTimeToSystemTime(ft, &st);

    /* Extract and format the filestamp date and time values */
    sprintf(buf, "%04u-%02u-%02u %02u:%02u:%02u",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond);

    return (buf);
}


/*------------------------------------------------------------------------------
* s_size()
*	Convert file size 's' into human-readable string form.
*
* Returns
*	Pointer to static string area.
*/

static const char * s_size(uint64_t s, char *buf)
{
    static char		abuf[40+2] =	{ '\0' };
    int			i, j;
    uint64_t		s2;

    /* Extract digits from the number */
    abuf[40+1] = '\0';
    for (i = 40*3/4, j = 40;  i >= 0;  i--)
    {
        int	d;

        d = s % 10;
        s2 = s / 10;

        abuf[j--] = (s != 0  or  j == 40 ? d + '0' : ' ');

        s = s2;
        if (s == 0)
            break;

        if (i % 3 == 1)
            abuf[j--] = (s2 != 0 ? ',' : ' ');
    }

    strcpy(buf, abuf+j+1);
    return (abuf);
}


/*------------------------------------------------------------------------------
* print_entry()
*	Prints info about file info 'info' with drive prefix 'drive' and path
*	prefix 'pre' to stream 'out'.
*/

static void print_entry(FILE *out, const char *drive, const char *pre,
    struct _WIN32_FIND_DATAA *info)
{
    const char *	sep = SEP_STR;
    uint64_t		sz;

retry:
    /* Get path prefix and separator */
    if (pre[0] == '.'  and  pre[1] == '\0')
    {
        pre = "";
        sep = "";
    }
    else if (pre[0] == '.'  and  pre[1] == SEP_CHAR)
    {
        while (pre[0] == '.'  and  pre[1] == SEP_CHAR)
            pre += 2;
        goto retry;
    }

    /* Print info for directory entry */
    sz = ((uint64_t)info->nFileSizeHigh << 32) + info->nFileSizeLow;
    if (opt.o_longlist)
    {
        char	sbuf[30+1];
        char	abuf[30+1];
        char	dbuf[40+1];

        /* Print detailed info */
        s_size(sz, sbuf);
        s_attrib(info->dwFileAttributes, abuf),
        s_datetime(&info->ftLastWriteTime, dbuf);
        fprintf(out, "%-9s %15s %s  ",
            abuf, sbuf, dbuf);
    }

    /* Print the entry name */
    if (opt.o_nameonly)
    {
        /* Print only the file name, sans prefix */
        fprintf(out, "%s\n", info->cFileName);
    }
    else
    {
        /* Print the full file pathname */
        fprintf(out, "%s%s%s%s\n",
            drive,
            pre,
            sep,
            info->cFileName);
    }

    /* Update counters */
    if (info->dwFileAttributes & A_DIRECTORY)
        count.c_dir++;
    else if (not (info->dwFileAttributes & A_VOLUME))
        count.c_file++;

    if (info->dwFileAttributes & (A_SYSTEM|A_HIDDEN))
        count.c_hidden++;

    count.c_bytes  += sz;
    count.c_blocks += (sz + BLOCKSIZE-1)/BLOCKSIZE;
}


/*------------------------------------------------------------------------------
* search()
*	Searches for filenames that match pattern 'pat'.
*	All found entries are printed to stream 'out'.
*
* Returns
*	Number of matching filenames found.
*/

long search(FILE *out, const char *pat)
{
    char		drive[2+1];		/* Search drive prefix	*/
    char		pre[8*1024+1];		/* Search path prefix	*/
    const char *	pre2;			/* Printable path prefix */
    char		pathname[12*1024+1];	/* Working pathname pattern */
    long		count = 0;		/* Found filename count	*/
    const char *	file;			/* Filename w/ wildcards */
    struct search_info	info;			/* Search control info	*/
    const char *	ip;
    char *		jp;

    /* Separate the drive prefix from the pattern */
    if (pat[0] != '\0'  and  pat[1] == ':')
    {
        drive[0] = pat[0];
        drive[1] = ':';
        drive[2] = '\0';
        pat += 2;
    }
    else
        drive[0] = '\0';

    /* Separate the directory prefix from the pattern */
    ip = strrchr(pat, '/');
    jp = strrchr(pat, '\\');
    ip = (ip > jp ? ip : jp);

    if (ip == NULL)
    {
        /* No '/' or '\' in pat */
        file = pat;
        strcpy(pre, ".");

        pre2 = pre;
    }
    else
    {
        /* Separate the path prefix and filename in the pattern */
        file = ip+1;
        if (ip-pat > 0)
        {
            memcpy(pre, pat, ip-pat);
            pre[ip-pat] = '\0';
            pre2 = pre;
        }
        else
        {
            strcpy(pre, SEP_STR);
            pre2 = "";
        }

        for (jp = pre;  *jp != '\0';  jp++)
            if (*jp == '\\'  or  *jp == '/')
                *jp = SEP_CHAR;
    }

    DL(printf("|drv=[%s] pre=[%s] pre2=[%s] file=[%s]\n",
        drive, pre, pre2, file));

    /* Verify the file pattern */
    if (not fpattern_isvalid(file))
    {
        fprintf(stderr, "%s: ill-formed filename pattern '%s'\n", prog, file);
        return (0);
    }

    /* Build the working search pattern */
    strcpy(pathname, drive);
    strcat(pathname, pre);
    if (pre[0] != SEP_CHAR  or  pre[1] != '\0')
        strcat(pathname, SEP_STR);
#if OLD /* pre-3.61 */
    strcat(pathname, file);
#else
    strcat(pathname, WILD_WIN32);
#endif

    DL(printf("|search=[%s]\n", pathname));

    /* Search for the first matching entry */
    memset(&info, '\0', sizeof(info));
    if (opt.o_verbose)
        fprintf(out, "Searching %s%s\n", drive, pre);

    if (not findfirst32(pathname, ATTR_DFL, &info))
    {
        /* First match not found */
        DL(printf("|%s: <none>\n", pathname));
    }
    else
    {
        /* Print matches */
        DL(printf("|%s: first '%s': [%s]\n", pathname, info.fdata.cFileName));
        do
        {
            /* Found next match, print it */
            if (fpattern_matchn(file, info.fdata.cFileName)  and
                include_entry(&info.fdata))
            {
                print_entry(out, drive, pre2, &info.fdata);
                count++;
            }
        } while (findnext32(&info));
    }

    /* Search for subdirs */
    DL(printf("-------------------------------------\n"));
    if (not opt.o_nosubdirs)
    {
        /* Build the working search pattern */
        strcpy(pathname, drive);
        strcat(pathname, pre);
        if (pre[0] != SEP_CHAR  or  pre[1] != '\0')
            strcat(pathname, SEP_STR);
        strcat(pathname, WILD_WIN32);

        DL(printf("|subsearch=[%s]\n", pathname));

        /* Search for the first subdir entry */
        if (findfirst32(pathname, A_DIRECTORY, &info))
        {
            /* Recurse on any subdirs */
            DL(printf("|%s: first sub: [%s]\n", info.fdata.cFileName));
            do
            {
                /* Next subdir found, recurse on it */
                if ((info.fdata.dwFileAttributes & A_DIRECTORY) != 0  and
                    strcmp(info.fdata.cFileName, ".") != 0  and
                    strcmp(info.fdata.cFileName, "..") != 0)
                {
                    /* Recurse on the next subdir */
                    strcpy(pathname, drive);
                    strcat(pathname, pre);
                    if (pre[0] != SEP_CHAR  or  pre[1] != '\0')
                        strcat(pathname, SEP_STR);
                    strcat(pathname, info.fdata.cFileName);
                    strcat(pathname, SEP_STR);
                    strcat(pathname, file);

                    DL(printf("|recurse=[%s]\n", pathname));
                    count += search(out, pathname);	/* Recurse	*/
                }
            } while (findnext32(&info));
        }
    }
    else
    {
        DL(printf("Do not recurse on subdirs\n"));
    }

    return (count);
}


/*------------------------------------------------------------------------------
* usage()
*	Print a command usage message, then punt.
*/

static const char *	usage_m[] =
{
    "Options:",
    "    -?          Show information about this program.",
    "    -a          Print all matching entries.",
    "    -A          Print all matching entries except \".\" and \"..\".",
    "    -d[+|-|!]D  Find files modified after/before/not date D.",
#ifdef is_undocumented
    "    -D          Enable debugging trace output.",
#endif
    "    -f          Show filenames without drive or path prefixes.",
    "    -l          Long listing.",
    "    -m          Show short DOS names.",
    "    -n          Show list summary.",
    "    -r          Do not recursively search subdirectories.",
    "    -s[+|-|!]N  File size [more/less/not] N bytes.",
#ifdef is_unsupported
|   "    -g[!]name   Owner group is [not] name.",
|   "    -g[!]num    Owner group is [not] group-ID.",
#endif
    "    -t[!]T...   Find entried [not] of type T, which is one of more of",
    "                these criteria combined (or-ed) together:",
    "                    a   Archive",
    "                    c   Compressed",
    "                    d   Directory",
    "                    f   File",
    "                    h   Hidden",
    "                    r   Read only",
    "                    s   System",
    "                    v   Volume label",
    "                    w   Writable",
#ifdef not_yet_supported
|   "                Lowercase letters are or-ed together, uppercase letters"
|   "                are and-ed together.",
#endif
#ifdef is_unsupported
|   "    -u[!]name   Owner user is [not] name.",
|   "    -u[!]num    Owner user is [not] user-ID.",
#endif
    "    -v          Verbose output.",
/*
+   "    -w[+|-]n[T] Find files modified before/after the last n time periods,",
+   "                where T is the optional time period type, one of:",
+   "                    h   Hours",
+   "                    d   Days (default)",
+   "                    y   Years",
*/
    "",
    "Filenames can contain wildcard characters:",
    "    ?       Matches any single character (including '.').",
    "    *       Matches zero or more characters (including '.').",
    "    [abc]   Matches 'a', 'b', or 'c'.",
    "    [a-z]   Matches 'a' through 'z'.",
    "    [!a-z]  Matches any character except 'a' thru 'z'.",
    "    `X      Matches X exactly (which may be a wildcard character).",
    "    !X      Matches any filename except X.",
    "",
    "Date 'D' is of the form \"[YY]YY[-MM[-DD]][:HH[:MM[:SS]]]\".",
    NULL
};

static void usage(void)
{
    int		i;

    fprintf(stderr, "[%s, %s %s]\n\n",
        ID_TITLE, ID_VERS, ID_DATE);

    fprintf(stderr, "Find matching filenames in a directory tree.\n\n");

    fprintf(stderr, "usage:  %s [option...] [path" SEP_STR "]file...\n\n",
        prog);

    for (i = 0;  usage_m[i] != NULL;  i++)
        fprintf(stderr, "%s\n", usage_m[i]);

    exit(RC_USAGE);
}


/*------------------------------------------------------------------------------
* about()
*	Print an 'about' message, then punt.
*/

static const char *	about_m[] =
{
    ID_COMPILE,
#if DEBUG
    "(with debugs)",
#endif
    "",
    COPYRIGHT,
    "",
    "This software may be freely distributed.  However, duplication and/or",
    "distribution of this software for financial gain without prior written",
    "consent of the author is prohibited.",
    "",
    "    David R. Tribble",
    "    " ID_C_EMAIL,
    "    " ID_C_HOME,
    NULL
};

static void about(void)
{
    int		i;

    for (i = 0;  about_m[i] != NULL;  i++)
        fprintf(stderr, "%s\n", about_m[i]);

    exit(RC_OKAY);
}


/*------------------------------------------------------------------------------
* parse_date()
*	Parses date/time specification 'arg'.
*	Unspecified parts are assumed to be the earliest unit.
*
* Lexicon
*	[CC] YY [[-] MM [[-] DD]] [[:] HH [[:] MM [[:] SS]]]
*
* Returns
*	True if specification was of correct syntax, otherwise false.
*/

static bool parse_date(const char *arg, struct _FILETIME *ft)
{
    int			j;
    long		v;
    time_t		tnow;
    struct tm		now;
    struct _SYSTEMTIME	st;

    /* Get the default time (which is now) */
    time(&tnow);
    now = *localtime(&tnow);

    DL(printf("|now:  %04d-%02d-%02d %02d:%02d:%02d\n",
        now.tm_year+1900,
        now.tm_mon+1,
        now.tm_mday,
        now.tm_hour,
        now.tm_min,
        now.tm_sec));

    /* Parse the date spec */

    /* Get year '[CC]YY' part */
    memset(&st, 0, sizeof(st));
    while (arg[0] != '\0'  and  not isdigit(arg[0]))
        arg++;
    if (isdigit(arg[0]))
    {
        for (v = j = 0;  j < 4  and  isdigit(arg[j]);  j++)
            v = v*10 + arg[j]-'0';
        arg += j;
        v = (v > 100 ? v : v+1900);

        st.wYear =   v;
        st.wMonth =  1;
        st.wDay =    1;
        st.wHour =   0;
        st.wMinute = 0;
        st.wSecond = 0;
    }
    else
        st.wYear = now.tm_year+1900;

    /* Get the month 'MM' part */
    while (arg[0] != '\0'  and  not isdigit(arg[0]))
        arg++;
    if (isdigit(arg[0]))
    {
        for (v = j = 0;  j < 2  and  isdigit(arg[j]);  j++)
            v = v*10 + arg[j]-'0';
        arg += j;

        st.wMonth = v;
        st.wDay =   1;
    }

    /* Get the day 'DD' part */
    while (arg[0] != '\0'  and  not isdigit(arg[0]))
        arg++;
    if (isdigit(arg[0]))
    {
        for (v = j = 0;  j < 2  and  isdigit(arg[j]);  j++)
            v = v*10 + arg[j]-'0';
        arg += j;

        st.wDay = v;
    }

    /* Get the hour 'HH' part */
    while (arg[0] != '\0'  and  not isdigit(arg[0]))
        arg++;
    if (isdigit(arg[0]))
    {
        for (v = j = 0;  j < 2  and  isdigit(arg[j]);  j++)
            v = v*10 + arg[j]-'0';
        arg += j;

        st.wHour =   v;
        st.wMinute = 0;
        st.wSecond = 0;
    }

    /* Get the minute 'MM' part */
    while (arg[0] != '\0'  and  not isdigit(arg[0]))
        arg++;
    if (isdigit(arg[0]))
    {
        for (v = j = 0;  j < 2  and  isdigit(arg[j]);  j++)
            v = v*10 + arg[j]-'0';
        arg += j;

        st.wMinute = v;
        st.wSecond = 0;
    }

    /* Get the second 'SS' part */
    while (arg[0] != '\0'  and  not isdigit(arg[0]))
        arg++;
    if (isdigit(arg[0]))
    {
        for (v = j = 0;  j < 2  and  isdigit(arg[j]);  j++)
            v = v*10 + arg[j]-'0';
        arg += j;

        st.wSecond = v;
    }

    DL(printf("|date: %04d-%02d-%02d %02d:%02d:%02d\n",
        st.wYear, st.wMonth, st.wDay,
        st.wHour, st.wMinute, st.wSecond));

    /* Convert broken-down "system" date/time into a filestamp time */
    if (not SystemTimeToFileTime(&st, ft))
        return (false);
    return (true);
}


/*------------------------------------------------------------------------------
* parse_size()
*	Parses file size specification 'arg'.
*
* Returns
*	True if specification was of correct syntax, otherwise false.
*/

static bool parse_size(const char *arg, uint64_t *sz)
{
    int		j;
    uint64_t	v;

    /* Parse the size spec */
    while (arg[0] != '\0'  and  not isdigit(arg[0]))
        arg++;

    v = 0;
    for (j = 0;  arg[j] != '\0';  j++)
    {
        if (isdigit(arg[j]))
        {
            uint64_t	p;

            p = v;
            v = v*10 + arg[j]-'0';

            if (p > v)
                return (false);
        }
    }

    DL(printf("|size: %I64u\n", v));

    *sz = v;
    return (true);
}


/*------------------------------------------------------------------------------
* parse_opts()
*	Parse the command line options.
*
* Returns
*	The number of args consumed.
*/

static int parse_opts(int argc, char **argv)
{
    int		i = 0;
    int		nexti;
    int		optch;
    char *	optarg;

#if DEBUG
    /* Print the command line args */
    for (i = 0;  i < argc;  i++)
        DL(printf("|argv[%2d]='%s'\n", i, argv[i]));
#endif

    /* Parse the command line options */
    for (i = 1;  i < argc  and  argv[i][0] == '-';  i++)
    {
        nexti = i+1;

        for ( ;  argv[i][1] != '\0';  argv[i]++)
        {
            optch = argv[i][1];

            if (argv[i][2] != '\0')
                optarg = &argv[i][2];
            else if (argv[i+1] == NULL)
                optarg = "";
            else
            {
                optarg = argv[i+1];
                nexti = i+2;
            }

            DL(printf("|argv[%d]: optch='%c' optarg='%s'\n",
                i, optch, optarg));

            switch (optch)
            {
            case '?':
                /* Print info about this program */
                about();
                break;

            case 'a':
                /* All files, including hidden ones */
                DL(printf("|-a\n"));
                opt.o_all = true;
                break;

            case 'A':
                /* All files, including hidden ones, except "." and ".." */
                DL(printf("|-A\n"));
                opt.o_almostall = true;
                break;

            case 'd':
                /* Date specification(s) */
                DL(printf("|-d '%s'\n", optarg));
                if (opt.o_date2 == NULL)
                {
                    opt.o_date2 = opt.o_date1;
                    opt.o_date2_v = opt.o_date1_v;
                }

                opt.o_date1 = optarg;
                if (not parse_date(optarg, &opt.o_date1_v))
                {
                    fprintf(stderr, "%s: improper date specification '%s'\n\n",
                        prog, optarg);
                    usage();
                }
                goto nextarg;

            case 'D':
                /* Enable debugging trace output */
                DL(printf("|-D\n"));
                opt_debug = true;
                break;

            case 'f':
                /* Show filenames without drive/path prefixes */
                DL(printf("|-f\n"));
                opt.o_nameonly = true;
                break;

            case 'g':
                /* Owner group name */
                DL(printf("|-g '%s'\n", optarg));
                opt.o_group = optarg;
                goto nextarg;

            case 'l':
                /* Long (verbose) listing */
                DL(printf("|-l\n"));
                opt.o_longlist = true;
                break;

            case 'm':
                /* Use short MS-DOS names */
                DL(printf("|-m\n"));
                opt.o_dosnames = true;
                break;

            case 'n':
                /* Show list summary */
                DL(printf("|-n\n"));
                opt.o_summary = true;
                break;

            case 'r':
                /* Do not search subdirectories */
                DL(printf("|-r\n"));
                opt.o_nosubdirs = true;
                break;

            case 's':
                /* Size specification */
                DL(printf("|-s '%s'\n", optarg));
                opt.o_size = optarg;

                if (not parse_size(optarg, &opt.o_size_v))
                {
                    fprintf(stderr, "%s: improper size specification '%s'\n\n",
                        prog, optarg);
                    usage();
                }
                goto nextarg;

            case 't':
                /* Entry type */
                DL(printf("|-t '%s'\n", optarg));
                opt.o_type = optarg;
                goto nextarg;

            case 'u':
                /* Owner user name */
                DL(printf("|-u '%s'\n", optarg));
                opt.o_user = optarg;
                goto nextarg;

            case 'v':
                /* Verbose output */
                DL(printf("|-v\n"));
                opt.o_verbose = true;
                break;

            default:
                /* Unknown option */
                DL(printf("|bad option '%c'\n", optch));
                fprintf(stderr, "%s: bad option '-%c'\n\n", prog, optch);
                usage();
            }
        }

        continue;

nextarg:
        DL(printf("|nextarg %d\n", nexti));
        i = nexti-1;
    }

    DL(printf("|%d args parsed\n", i));
    return (i);
}


/*------------------------------------------------------------------------------
* main()
*/

int main(int argc, char **argv)
{
    int		i;
    long	c;
    long	tot_ent =	0;
    long	tot_dir =	0;
    long	tot_file =	0;
    long	tot_visib =	0;
    uint64_t	tot_byte =	0;
    uint64_t	tot_block =	0;
    char	c_ent[30+1];
    char	c_dir[30+1];
    char	c_file[30+1];
    char	c_byte[30+1];
    char	c_block[30+1];
    char	c_visib[30+1];

    DL(printf("|========================================\n"));

    /* Get options */
    i = parse_opts(argc, argv);
    argc -= i;
    argv += i;

    DL(printf("|========================================\n"));

    /* Check usage */
    if (argc < 1)
        usage();

    /* Search for matching directory entries */
    for (i = 0;  i < argc;  i++)
    {
        count.c_dir =    0;
        count.c_file =   0;
        count.c_hidden = 0;
        count.c_bytes =  0;
        count.c_blocks = 0;

        if (opt.o_longlist  and  i > 0)
            fprintf(stdout, "\n");

        c = search(stdout, argv[i]);

        /* Print totals */
        if (opt.o_longlist  or  opt.o_summary)
        {
            s_size(c, c_ent);
            s_size(count.c_dir, c_dir);
            s_size(count.c_file, c_file);
            s_size(c - count.c_hidden, c_visib);
            s_size(count.c_bytes, c_byte);
            s_size(count.c_blocks, c_block);

            if (c > 0)
                fprintf(stdout, "\n");

            fprintf(stdout, " Entries:     %12s  (%s)\n",
                c_ent, c_visib);
            fprintf(stdout, " Directories: %12s  Files:  %12s\n",
                c_dir, c_file);
            fprintf(stdout, " Bytes:       %12s  Blocks: %12s\n",
                c_byte, c_block);
        }

        tot_ent   += c;
        tot_dir   += count.c_dir;
        tot_file  += count.c_file;
        tot_visib += c - count.c_hidden;
        tot_byte  += count.c_bytes;
        tot_block += count.c_blocks;
    }

    /* Print grand totals */
    if ((opt.o_longlist  or  opt.o_summary)  and  argc > 1)
    {
        s_size(tot_ent, c_ent);
        s_size(tot_dir, c_dir);
        s_size(tot_file, c_file);
        s_size(tot_byte, c_byte);
        s_size(tot_block, c_block);
        s_size(tot_visib, c_visib);

        fprintf(stdout, "\n");
        fprintf(stdout, " Total Entries:     %12s  (%s)\n",
            c_ent, c_visib);
        fprintf(stdout, " Total Directories: %12s  Files:  %12s\n",
            c_dir, c_file);
        fprintf(stdout, " Total Bytes:       %12s  Blocks: %12s\n",
            c_byte, c_block);
    }

    return (c > 0 ? RC_OKAY : RC_ERR);
}

/* End vfind.c */
