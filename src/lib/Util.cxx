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
 * $Id: Util.cxx,v 1.4 2002/10/11 16:26:56 marka Exp $
 */
#include <RegularExpression.h>

#include "Util.h"

#include <sys/stat.h>

string Util::programPath="";

bool Util::regexMatch(char *regex, char *string)
{
RegularExpression re(regex);
        
    return re.find((const char *)string);
}

bool Util::fileExists(const char *filename)
{
struct stat fs;

    if (stat(filename, &fs) != 0) {
        return false;
    } else {
        return true;
    }
}

bool Util::fileExists(string filename)
{
    return Util::fileExists(filename.c_str());
}

bool Util::fileIsDirectory(const char* name)
{ 
struct stat fs;

    if(stat(name, &fs) == 0) {
#if WIN32
        return ((fs.st_mode & _S_IFDIR) != 0);
#else
        return S_ISDIR(fs.st_mode);
#endif
    } else {
        return false;
    }
}

const char *Util::convertToUnixSlashes(string& path)
{
string::size_type pos = 0;

    while((pos = path.find('\\', pos)) != string::npos) {
        path[pos] = '/';
        pos++;
    }
    // remove any trailing slash
    if(path.size() && path[path.size()-1] == '/') {
        path = path.substr(0, path.size()-1);
    }
    // if there is a tilda ~ then replace it with HOME
    if (path.find("~") == 0 && getenv("HOME")) {
        path = string(getenv("HOME")) + path.substr(1);
    }
    // if there is a /tmp_mnt in a path get rid of it!
    // stupid sgi's
    if (path.find("/tmp_mnt") == 0) {
        path = path.substr(8);
    }
    return path.c_str();
}

void Util::splitProgramPath (
    const char* in_name,
    string& dir,
    string& file
) {
    dir = in_name;
    file = "";
    Util::convertToUnixSlashes(dir);

    if (!Util::fileIsDirectory(dir.c_str())) {
        string::size_type slashPos = dir.rfind("/");
        if (slashPos != string::npos) {
            file = dir.substr(slashPos+1);
            dir = dir.substr(0, slashPos);
        } else {
            file = dir;
            dir = "";
        }
    }
    if ((dir != "") && !Util::fileIsDirectory(dir.c_str())) {
        // Error splitting file name off end of path: Directory not found
        std::string oldDir = in_name;
        Util::convertToUnixSlashes(oldDir);
        dir = in_name;
    }
}

void Util::setProgramPath(const char *path)
{
string file;

    Util::splitProgramPath(path,programPath,file);
}

const char *Util::getDistFile(const char *path)
{
static string flP5Root;

    // flP5Root = string("");
    if (getenv("FLP5_ROOT")) {
        // do FLP5_ROOT, look for the environment variable first
        flP5Root = getenv("FLP5_ROOT");
    }
    if (!Util::fileExists(flP5Root + path)) {
        // next try exe/..
        string::size_type slashPos = Util::programPath.rfind("/");
        if (slashPos != string::npos) {
            flP5Root = Util::programPath.substr(0, slashPos);
        }
    }
    if (!Util::fileExists(flP5Root + path)) {
        // try exe/../share/flP5
        flP5Root += "/share/flP5";
    }
#ifdef FLP5_ROOT_DIR
    if (!Util::fileExists(flP5Root + path)) {
        // try compiled in root directory
        flP5Root = FLP5_ROOT_DIR;
    }
#endif
#ifdef FLP5_PREFIX
    if (!Util::fileExists(flP5Root + path)) {
        // try compiled in install prefix
        flP5Root = FLP5_PREFIX "/share/flP5";
    }
#endif
    if (!Util::fileExists(flP5Root + path)) {
        // try exe/share/flP5
        flP5Root  = Util::programPath;
        flP5Root += "/share/flP5";
    }
    if (!Util::fileExists(flP5Root + path)) {
        // couldn't find modules
        return 0;
    }
    flP5Root += path;
    return flP5Root.c_str();
}

bool Util::setUser(int userid)
{
#ifndef WIN32
    if (userid != geteuid()) {
        fprintf (
            stderr,
            "User %4d as %4d is requesting to become %4d -> ",
            getuid(), geteuid(), userid
        );
        if (seteuid(userid) < 0) {
            fprintf( stderr, "FAILED : remaining %4d\n", geteuid() );
            return false;
        }
        fprintf( stderr, "SUCCESS: now is    %4d\n", geteuid() );
    }
#endif

    return true;
}
