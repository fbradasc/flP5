/* Copyright (C) 2002  Mark Andrew Aikens <marka@desert.cx>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * $Id: Util.h,v 1.4 2002/09/14 02:20:44 marka Exp $
 */
#ifndef __Util_h
#define __Util_h

#include "Preferences.h"

#include <stdio.h>

#include <stdarg.h>
#include <string.h>

#ifndef WIN32
#  include <strings.h>
#endif

#include <ctype.h>

#include <string>
using namespace std;

/*
 * Apparently Unixware defines "index" to strchr (!) rather than
 * providing a proper entry point or not providing the (obsolete)
 * BSD function.  Make sure index is not defined...
 */

#ifdef index
#  undef index
#endif /* index */

#if defined(WIN32) && !defined(__CYGWIN__)
#  define strcasecmp(s,t) stricmp((s), (t))
#  define strncasecmp(s,t,n)  strnicmp((s), (t), (n))
#elif defined(__EMX__)
#  define strcasecmp(s,t) stricmp((s), (t))
#  define strncasecmp(s,t,n)  strnicmp((s), (t), (n))
#endif /* WIN32 */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
/*
 * MetroWerks' CodeWarrior put thes "non-standard" functions in
 * <extras.h> which unfortunatly does not play well otherwise
 * when included - to be resolved...
 */

#if defined(__APPLE__) && defined(__MWERKS__)
int strcasecmp(const char*,const char*);
int strncasecmp(const char*,const char*,int);
char *strdup(const char*);
#endif

#if defined(WIN32)
extern int fl_snprintf(char *, size_t, const char *, ...);
#  define snprintf fl_snprintf
#endif /* !HAVE_SNPRINTF */

#if defined(WIN32)
extern int fl_vsnprintf(char *, size_t, const char *, va_list ap);
#  define vsnprintf fl_vsnprintf
#endif /* !HAVE_VSNPRINTF */

/*
 * strlcpy() and strlcat() are some really useful BSD string functions
 * that work the way strncpy() and strncat() *should* have worked.
 */

#if defined(WIN32)
extern size_t fl_strlcat(char *, const char *, size_t);
#  define strlcat fl_strlcat
#endif /* !HAVE_STRLCAT */

#if defined(WIN32)
extern size_t fl_strlcpy(char *, const char *, size_t);
#  define strlcpy fl_strlcpy
#endif /* !HAVE_STRLCPY */

#ifdef __cplusplus
}
#endif /* __cplusplus */

/** \file */


/** A class composed entirely of random static utility functions */
class Util
{
public:
    static string programPath;

    /** Match a string against an extended regular expression.
     * \param regex The regular expression.
     * \param string The string to test against the regular expression.
     * \returns A boolean value indicating if the string matched the
     *          regular expression.
     * \throws logic_error If the regex compilation fails.
     */
    static bool regexMatch(const char *regex, char *string);

    /** Tests if the filename exists.
     * \param filename The file name to test
     * \returns A boolean value indicating if the filename exists.
     */
    static bool fileExists(const char *filename);
    static bool fileExists(string filename);

    static bool fileIsDirectory(const char* name);
    static const char *convertToUnixSlashes(string& path);
    /**
     * Given the path to a program executable, get the directory part of the
     * path with the file stripped off.  If there is no directory part, the
     * empty string is returned.
     */
    static void splitProgramPath (
        const char* in_name,
        string& dir,
        string& file
    );
    /**
     * Given the path to a program executable, get the directory part of the
     * path with the file stripped off.  If there is no directory part, the
     * empty string is returned.
     */
    static void setProgramPath(const char *path);

    /** Returns the full pathname of a file located into the distribution tree.
     * \param path the path of the file to search relative to the distribution
     *             tree
     * \returns The full pathname of searched file.
     */
    static const char *getDistFile(const char *path);

    /** Switch user
     * \param user ID to switch on
     */
    static bool setUser(int userid); 

    static int snprintfcat(size_t &pos, char *buffer, size_t buffer_size, const char *fmt, ...);
};


#endif
