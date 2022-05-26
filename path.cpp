/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */

#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "config.hpp"
#include "exception.hpp"
#include "path.hpp"
#include "path_list.hpp"
#include "user.hpp"
#include "util.hpp"

using namespace std;
using namespace c4s;

// -------------------------------------------------------------------------------------------------
void
c4s::path::init_common()
{
    change_time = 0;
    flag = false;
    owner = 0;
    mode = -1;
}
// -------------------------------------------------------------------------------------------------
c4s::path::path()
{
    init_common();
}
// -------------------------------------------------------------------------------------------------
c4s::path::path(const path& _dir, const char* _base, user* _owner, int _mode)
{
    init_common();
    dir = _dir.dir;
    if (_base)
        base = _base;
    if (_owner)
        owner = _owner;
    else
        owner = _dir.owner;
    if (_mode)
        mode = _mode;
    else
        mode = _dir.mode;
}
// -------------------------------------------------------------------------------------------------
/** If string has dir separator as last character then the base is empty. If no directory
  separators are detected then the dir is empty.
  \param init Path name to initialize the object with. String can be a file name, path or
  combination of both i.e. full path.  */
c4s::path::path(const string& p)
{
    set(p);
}
// -------------------------------------------------------------------------------------------------
c4s::path::path(const string& d, const string& b, const string& e)
{
    set(d, b, e);
}
// -------------------------------------------------------------------------------------------------
c4s::path::path(const char* d, const char* b, const char* e)
{
    if (!d || !b)
        throw path_exception("path::path - Missing dir and base from path constructor.");
    if (e)
        set(string(d), string(b), string(e));
    else
        set(string(d), string(b), string());
}
// -------------------------------------------------------------------------------------------------
c4s::path::path(const string& d, const string& b)
{
    set(d, b);
}
// -------------------------------------------------------------------------------------------------
c4s::path::path(const string& d, const char* b)
{
    if (b)
        set(d, string(b));
    else
        set_dir(string(d));
}
// -------------------------------------------------------------------------------------------------
c4s::path::path(const char* d, const char* b)
{
    if (d && b) {
        set(string(d), string(b));
        return;
    }
    init_common();
    if (!d && b)
        base = b;
    else if (d && !b)
        set_dir(string(d));
}
// -------------------------------------------------------------------------------------------------
c4s::path::path(const string& d, const string& b, user* o, int m)
{
    set(d, b);
    owner = o;
    mode = m;
}
c4s::path::path(const string& p, user* o, int m)
{
    set(p);
    owner = o;
    mode = m;
}
// -------------------------------------------------------------------------------------------------
void c4s::path::operator=(const path& p)
{
    dir = p.dir;
    base = p.base;
    change_time = p.change_time;
    flag = p.flag;
    owner = p.owner;
    mode = p.mode;
}
// -------------------------------------------------------------------------------------------------
/** If the string ends with directory separator string is copied into dir and base is left
  empty. If the string does not have any directory separators string is copied to base and dir is
  left epty. Otherwice the string after the last directory separator is taken as base and
  beginning is taken as directory.

  \param init String to initialize with.
*/
void
c4s::path::set(const string& init)
{
    string work;
    init_common();
    work = init;
    size_t last = work.rfind(C4S_DSEP);
    if (last == string::npos) {
        dir.clear();
        base = work;
        return;
    }
    size_t len = work.size();
    if (last == len) {
        base.clear();
        set_dir(work);
        return;
    }
    set_dir(work.substr(0, last));
    base = work.substr(last + 1);
}
// -------------------------------------------------------------------------------------------------
/**
   \param dir_in Directory name.
   \param base_in Base name (=file name) with option exception.
   \param ext Extension. If base has extension then it is changed to this extension. Parameter is
   optional. Defaults to null.
*/
void
c4s::path::set(const string& dir_in, const string& base_in, const string& ext)
{
    init_common();
    set_dir(dir_in);
    base = base_in;
    if (!ext.empty()) {
        if (base.find('.') == string::npos)
            base += ext;
        else
            base = get_base(ext);
    }
}
// -------------------------------------------------------------------------------------------------
void
c4s::path::set(const char* d, const char* b, const char* e)
{
    init_common();
    if (!d || !b)
        throw path_exception("path::set - dir nor base parameter can be null.");
    if (e)
        set(string(d), string(b), string(e));
    else
        set(string(d), string(b), string());
}
// -------------------------------------------------------------------------------------------------
/**
  \param dir_in Directory name.
  \param base_in Base name (=file name)
*/
void
c4s::path::set(const string& dir_in, const string& base_in)
{
    init_common();
    set_dir(dir_in);
    base = base_in;
}
// -------------------------------------------------------------------------------------------------
/**
  \param vs Reference to an array of strings where the directory parts are filled into.
 */
void
c4s::path::get_dir_parts(vector<string>& vs) const
{
    string::size_type pt = 1, prev = 1;
    auto si = dir.begin();
    si++; // skip the first separator;
    while (si != dir.end()) {
        if (*si == C4S_DSEP) {
            vs.push_back(dir.substr(prev, pt - prev));
            prev = pt + 1;
        }
        si++;
        pt++;
    }
}
// -------------------------------------------------------------------------------------------------
string
c4s::path::get_path_quot() const
{
    ostringstream os;
    bool quotes = false;
    if ((!dir.empty() && dir.find(' ') != string::npos) ||
        (!base.empty() && base.find(' ') != string::npos))
        quotes = true;
    if (quotes)
        os << C4S_QUOT;
    os << dir;
    os << base;
    if (quotes)
        os << C4S_QUOT;
    return os.str();
}
// -------------------------------------------------------------------------------------------------
const char*
c4s::path::get_pp() const
{
    static string ps;
    ps = dir;
    ps += base;
    return ps.c_str();
}
// -------------------------------------------------------------------------------------------------
/** If extension is given it is exchanged with the base's current extension or then appended to
  base name if it currently does not have extension.

  \param ext Extension for the name. Include the '.' separator to the beginning of extension.
  \retval string Resulting base name.
*/
string
c4s::path::get_base(const string& ext) const
{
    if (ext.empty())
        return base;
    size_t loc = base.find_last_of('.');
    if (loc == string::npos)
        return base + ext;
    return base.substr(0, loc) + ext;
}
// -------------------------------------------------------------------------------------------------
string
c4s::path::get_base_or_dir()
{
    if (!base.empty())
        return base;
    size_t loc = dir.find_last_of(C4S_DSEP);
    if (loc == string::npos)
        return dir;
    return dir.substr(loc + 1);
}
// -------------------------------------------------------------------------------------------------
/**
  \retval string Extension of the file name part. Empty string is returned if extension does not
  exist.
*/
string
c4s::path::get_ext() const
{
    size_t extOffset = base.find_last_of('.');
    if (extOffset != string::npos)
        return base.substr(extOffset);
    return string();
}
// -------------------------------------------------------------------------------------------------
/** Replaces the current directory part of this path. The directory separator is added to the end
  of given string if it is missing. Tilde character at the front is automatically expanded to
  user's absolute home path since fstream library does not recognize it.
  \param new_dir New directory for path.
  \retval size_t Position of last directory separator in path.
*/
void
c4s::path::set_dir(const string& new_dir)
{
    if (new_dir.empty())
        return;
    string work = append_slash(new_dir);
    if (work[0] == '~') {
        set_dir2home();
        dir += work.substr(2);
    } else
        dir = work;
}
// -------------------------------------------------------------------------------------------------
void
c4s::path::set_dir2home()
{
    string home;
    if (!get_env_var("HOME", home)) {
        throw path_exception("path::set_dir2home error: Unable to find HOME environment variable");
    }
    dir = home;
    if (dir.at(dir.size() - 1) != C4S_DSEP)
        dir += C4S_DSEP;
}
// -------------------------------------------------------------------------------------------------
/**
  \param newb New base. If empty the base is cleared.
*/
void
c4s::path::set_base(const string& newb)
{
    if (newb.empty()) {
        base.clear();
        return;
    }
    base = newb;
}
// -------------------------------------------------------------------------------------------------
/**
  If given ext is empty then the extension is cleared.
  \param ext New extension string.
*/
void
c4s::path::set_ext(const string& ext)
{
    if (base.empty())
        return;
    size_t extOffset = base.find_last_of('.');
    if (extOffset != string::npos)
        base.erase(extOffset);
    if (ext.empty())
        return;
    base += ext;
}
// -------------------------------------------------------------------------------------------------
/**
  \retval String Base without extension
*/
string
c4s::path::get_base_plain() const
{
    size_t extOffset = base.find_last_of('.');
    if (extOffset == string::npos)
        return base;
    return base.substr(0, extOffset);
}
// -------------------------------------------------------------------------------------------------
/**
  \param to Directory to change to.
  \retval True on success, false on failure.
*/
void
c4s::path::cd(const char* to)
{
    ostringstream os;
    if (!to || to[0] == 0)
        return;
    if (chdir(to)) {
        os << "Unable chdir to:" << to << " Error:" << strerror(errno);
        throw path_exception(os.str());
    }
}
// -------------------------------------------------------------------------------------------------
void
c4s::path::read_cwd()
{
    char chCwd[512];
    if (!getcwd(chCwd, sizeof(chCwd)))
        throw path_exception("Unable to get current dir");
    dir = chCwd;
    dir += C4S_DSEP;
}
// -------------------------------------------------------------------------------------------------
/** Checks the status of the path's owner.
 *  \retval c4s::OWNER
 */
c4s::OWNER_STATUS
c4s::path::owner_status()
{
    struct stat dsbuf;
    if (!owner)
        return OWNER_STATUS::EMPTY;
    if (!owner->is_ok())
        return OWNER_STATUS::MISSING;
    if (stat(get_dir_plain().c_str(), &dsbuf))
        return OWNER_STATUS::NOPATH;
    if (owner->match(dsbuf.st_uid, dsbuf.st_gid)) {
        if (mode >= 0) {
            string fp = base.empty() ? get_dir_plain() : get_path();
            if (get_path_mode(fp.c_str()) == mode)
                return OWNER_STATUS::OK;
            else
                return OWNER_STATUS::NOMATCH_MODE;
        }
        return OWNER_STATUS::OK;
    }
    return OWNER_STATUS::NOMATCH_UG;
}
// -------------------------------------------------------------------------------------------------
/** Reads the current owner of the path on disk and loads this path with that particular user.
  If path does not exist an exception is thrown.
*/
void
c4s::path::owner_read()
{
    struct stat dsbuf;
    if (!exists())
        throw path_exception("Cannot read owner for non-existing path.");
    if (!owner)
        throw path_exception("Cannot read owner into null.");
    string fp = base.empty() ? get_dir_plain() : get_path();
    if (stat(fp.c_str(), &dsbuf)) {
        ostringstream os;
        os << "Unable to get ownership for file:" << get_path() << ". Error:" << strerror(errno);
        throw path_exception(os.str());
    }
    owner->set(dsbuf.st_uid, dsbuf.st_gid);
}
// -------------------------------------------------------------------------------------------------
/** If current user does not have a permission to do so, an exception is thrown. File mode
  needs to be set separately.
*/
void
c4s::path::owner_write()
{
    ostringstream os;
    if (!owner)
        throw path_exception("Cannot write non-existing owner.");
    if (!owner->is_ok()) {
        os << "Both user and group must be defined to write file ownership:" << get_path();
        os << " - user:" << owner->get_name() << " - group:" << owner->get_group();
        throw c4s_exception(os.str());
    }
    if (!exists())
        throw path_exception("Cannot write owner for non-existing path");
    string fp = base.empty() ? get_dir_plain() : get_path();
    if (chown(fp.c_str(), owner->get_uid(), owner->get_gid())) {
        os << "Unable to set path owner for " << get_path()
           << " - system error: " << strerror(errno);
        throw c4s_exception(os.str());
    }
}
// -------------------------------------------------------------------------------------------------
/** Reads current path mode from file system.
 */
void
c4s::path::read_mode()
{
    string fp = base.empty() ? get_dir_plain() : get_path();
    int pm = get_path_mode(fp.c_str());
    if (pm >= 0)
        mode = pm;
}
// -------------------------------------------------------------------------------------------------
bool
c4s::path::is_absolute() const
{
    if (dir.length() && dir[0] == '/')
            return true;
    return false;
}
// -------------------------------------------------------------------------------------------------
void
c4s::path::make_absolute()
{
    if (is_absolute())
        return;
    char chCwd[512];
    if (!getcwd(chCwd, sizeof(chCwd))) {
        ostringstream eos;
        eos << "Unable to get current dir - " << strerror(errno);
        throw path_exception(eos.str());
    }
    strcat(chCwd, "/");
    //    cout << "DEBUG - make_absolute original:"<<dir<<'\n';
    //    cout << "DEBUG - make_absolute current:"<<chCwd<<'\n';
    if (dir.length() && dir[0] == '.' && dir[1] == '.') {
        string cwd = chCwd;
        size_t dirIndex = 1;
        size_t slash = cwd.length() - 2;
        do {
            slash = cwd.find_last_of(C4S_DSEP, slash - 1);
            dirIndex += 3;
        } while (dirIndex < dir.length() && dir[dirIndex] == '.' && slash != string::npos);
        if (slash == string::npos) {
            ostringstream os;
            os << "Incorrect relevant path " << dir << " detected for " << cwd;
            throw path_exception(os.str().c_str());
        }
        dir = cwd.substr(0, slash + 1) + dir.substr(dirIndex - 1);
    } else if (dir.length() && dir[0] == '.')
        dir.replace(0, 2, chCwd);
    else
        dir.insert(0, chCwd);
    //    cout << "DEBUG - Make absolute final:"<<dir<<'\n';
}
// -------------------------------------------------------------------------------------------------
/**  Root is expected to be absolute directory and this path relative. If this path is absolute
  the current dir is simply replaced with root.  Otherwise this relative dir is added into the
  root.
  \param root Source full / absolute directory.
*/
void
c4s::path::make_absolute(const string& root)
{
    // If absolute - replace dir and return
    if (is_absolute()) {
        set_dir(root);
        return;
    }
    // Roll down possible parent directory markers.
    size_t offset = root.length() - 1;
    int count = 0;
    while (dir.find("..", count) == 0) {
        offset = root.find_last_of(C4S_DSEP, offset - 1);
        count += 3;
    }
    // Append the remaining dir to rolled down root
    string tmp(root);
    tmp.replace(offset + 1, root.length() - offset + 1, dir);
    dir = tmp;
}
// -------------------------------------------------------------------------------------------------
void
c4s::path::make_relative()
{
    path parent;
    parent.read_cwd();
    make_relative(parent);
}
// -------------------------------------------------------------------------------------------------
/**  If parent is longer than this directory or if the parent is not found from this directory
  the function does nothing. The './' is NOT added to the front.

  \param parent Parent part of the directory name is removed from this path.
*/
void
c4s::path::make_relative(const path& parent)
{
    size_t pos = dir.find(parent.dir);
    if (pos > 0 || pos == string::npos)
        return;
    dir.erase(0, parent.dir.size());
}
// -------------------------------------------------------------------------------------------------
/** If count is larger than directories in path then the dir part is left empty.

  \param count Number of directories to drop from the end of the dir tree.  to drop.
*/
void
c4s::path::rewind(int count)
{
    size_t dirIndex = dir.length();
    if (dirIndex == 0)
        return;
    dirIndex--;
    for (int i = 0; i < count && dirIndex != string::npos; i++) {
        dirIndex--;
        dirIndex = dir.find_last_of(C4S_DSEP, dirIndex);
    }
    if (dirIndex != string::npos)
        dir.erase(dirIndex + 1);
    else
        dir.clear();
}
// -------------------------------------------------------------------------------------------------
/**  First base is directly copied from append-path. Then the directories are merged. If append
  directory is absolute it is simply copied over this dir.  If append directory has .. - this
  dir is rolled down untill no .. exist and then remaining append dir is appended. Otherwice
  the the append directory is appended to this dir.

  \param append Path to append to this one.
*/
void
c4s::path::merge(const path& append)
{
    if (!append.base.empty())
        base = append.base;
    if (append.is_absolute()) {
        dir = append.dir;
        return;
    }
    if (append.dir[0] == '.') {
        if (append.dir[1] == C4S_DSEP) {
            dir += append.dir.substr(2);
            return;
        }
        // Roll down this path directories
        size_t offset = dir.length() - 1;
        int count = 0;
        while (append.dir.find("..", count) == 0) {
            offset = dir.find_last_of(C4S_DSEP, offset - 1);
            count += 3;
        }
        // Append the remaining dir to rolled down dir
        dir.replace(offset + 1, string::npos, append.dir, count, string::npos);
    } else
        dir += append.dir;
}
// -------------------------------------------------------------------------------------------------
bool
c4s::path::dirname_exists() const
{
    struct stat file_stat;
    if (!stat(get_dir_plain().c_str(), &file_stat)) {
        if (S_ISDIR(file_stat.st_mode))
            return true;
    }
    return false;
}
// -------------------------------------------------------------------------------------------------
/**
  \param target Path to compare to
  \param option Compare option: CMP_DIR compares directory parts only, CMP_BASE compares bases only.
                combine options to compare entire path.
 */
int
c4s::path::compare(const path& target, unsigned char option) const
{
    if ((option & (CMP_DIR | CMP_BASE)) == 0)
        return 0;
    if ((option & CMP_DIR) > 0) {
        if ((option & CMP_BASE) > 0)
            return get_path().compare(target.get_path());
        return dir.compare(target.dir);
    }
    return base.compare(target.base);
}
// -------------------------------------------------------------------------------------------------
/**  If the dir is relative the dir is first translated to absolute path using current
  directory as reference point. Function is able to create entire tree specified by this
  path.
  \param l_mode     if specified will be used as mode when directory is created.
  \param l_owner    if specified will be used as directory owner when it is created.
*/
void
c4s::path::mkdir(c4s::user* l_owner, int l_mode) const
{
    string fullpath;
    path mkpath;
    if (is_absolute())
        fullpath = get_dir();
    else {
        path tmp(*(const path*)this);
        tmp.make_absolute();
        fullpath = tmp.get_dir();
    }
#ifdef C4S_DEBUGTRACE
    cout << "DEBUG: fullpath: "<<fullpath<<'\n';
#endif
    size_t offset = 1;
    do {
        offset = fullpath.find(C4S_DSEP, offset + 1);
        mkpath.dir = (offset == string::npos) ? fullpath : fullpath.substr(0, offset + 1);
        if (!mkpath.dirname_exists()) {
            if (::mkdir(mkpath.get_dir().c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1)
                {
                    ostringstream os;
                    os << "path::mkdir - Unable to create directory: " << mkpath.get_dir();
                    throw path_exception(os.str().c_str());
                }
            if (l_owner && l_owner->is_ok()) {
                if (chown(mkpath.get_dir_plain().c_str(), l_owner->get_uid(), l_owner->get_gid())) {
                    ostringstream os;
                    os << "path::mkdir - Unable to set path owner for dir " << mkpath.get_dir_plain()
                       << " - system error: " << strerror(errno);
                    throw c4s_exception(os.str());
                }
            }
            if (l_mode != -1) {
                int new_mode = l_mode;
                if ((l_mode & 0x400) > 0)
                    new_mode |= 0x100;
                if ((l_mode & 0x40) > 0)
                    new_mode |= 0x10;
                if ((l_mode & 0x4) > 0)
                    new_mode |= 0x1;
                mkpath.chmod(new_mode);
            }
        }
    } while (offset != string::npos);
}
// -------------------------------------------------------------------------------------------------
/**  Base name is ignored. If recursive is not set then the exception is thrown if the
  directory is not empty. If directory is not found this function does nothing.
  \param recursive If true then the directory is deleted recursively. USE WITH CARE!
*/
void
c4s::path::rmdir(bool recursive) const
{
    if (!::rmdir(dir.c_str()))
        return;
    if (errno == ENOENT)
        return;
    if (errno != ENOTEMPTY) {
        ostringstream os;
        os << "path::rmdir - failed on directory: " << dir << ". Error:" << strerror(errno);
        throw path_exception(os.str().c_str());
    }
    if (!recursive) {
        ostringstream os;
        os << "path::rmdir - Directory to be removed is not empty: " << dir;
        throw path_exception(os.str().c_str());
    }
    // Open the directory
    DIR* target_dir = opendir(dir.c_str());
    if (!target_dir) {
        ostringstream os;
        os << "path::rmdir - Unable to access directory: " << dir << '\n' << strerror(errno);
        throw path_exception(os.str().c_str());
    }

    char filename[1024];
    struct dirent* de = readdir(target_dir);
    while (de) {
        strcpy(filename, dir.c_str());
        strcat(filename, de->d_name);
        if (de->d_type == DT_DIR) {
            if (strcmp(de->d_name, ".") && strcmp(de->d_name, "..")) {
                strcat(filename, "/");
                path child(filename);
                child.rmdir(true);
            }
        } else {
            if (unlink(filename) == -1) {
                ostringstream os;
                os << "path::rmdir - Unable to delete file from to be removed directory: "
                   << filename << '\n'
                   << strerror(errno);
                throw path_exception(os.str().c_str());
            }
        }
        de = readdir(target_dir);
    }
    closedir(target_dir);
    if (::rmdir(dir.c_str())) {
        ostringstream os;
        os << "path::rmdir - Unable to remove directory: " << dir << '\n' << strerror(errno);
        throw path_exception(os.str().c_str());
    }
}
// -------------------------------------------------------------------------------------------------
/** If the base is empty function calls dirname_exists().  In Linux existence of symbolic link
  also returns true.
  \retval bool True if dir and base exists, false if not.
*/
bool
c4s::path::exists() const
{
    if (base.empty())
        return dirname_exists();
    // Simply stat the file
    struct stat target;
    if (!lstat(get_path().c_str(), &target)) {
        if (S_ISREG(target.st_mode) || S_ISLNK(target.st_mode)) {
            return true;
        }
    }
    return false;
}
// -------------------------------------------------------------------------------------------------
/**
  \param envar Pointer to environment variable.
  \param set_dir If true then the directory is replaced with the found directory path.
  \retval bool True if base exists, false if not.
*/
bool
c4s::path::exists_in_env_path(const char* envar, bool set_dir)
{
    string envpath;
    if (!get_env_var(envar, envpath)) {
        stringstream ss;
        ss << "path::exists_in_env_path - Unable to find variable: " << envar;
        throw path_exception(ss.str().c_str());
    }

    // Save the original dir
    string backupDir = dir;
    size_t end, start = 0;
    do {
        end = envpath.find(C4S_PSEP, start);
        if (end == string::npos)
            dir = envpath.substr(start);
        else
            dir = envpath.substr(start, end - start);
        start = end + 1;
        dir.push_back(C4S_DSEP);
        if (exists()) {
            if (!set_dir)
                dir = backupDir;
            return true;
        }
    } while (end != string::npos);
    dir = backupDir;
    return false;
}
// -------------------------------------------------------------------------------------------------
/**  This file should be the source file checked against compile output.

  \param target Path object of target file.
  \retval bool True if (this) source is newer than target or if target does not exist.
*/
bool
c4s::path::outdated(path& target)
{
    if (!target.exists())
        return true;
    if (!change_time)
        read_changetime();
    try {
        if (compare_times(target) > 0)
            return true;
    } catch (const path_exception&) {
        return true;
    }
    return false;
}
// -------------------------------------------------------------------------------------------------
/**
   \param lst List of files to check.
   \retval bool True if file should be compiled, false if not.
*/
bool
c4s::path::outdated(path_list& lst)
{
    if (!exists())
        return true;
    if (!change_time)
        read_changetime();
    list<path>::iterator pi;
    for (pi = lst.begin(); pi != lst.end(); pi++) {
        if (compare_times(*pi) < 0)
            return true;
    }
    return false;
}
// -------------------------------------------------------------------------------------------------
/**
  \param target Path to target file
  \retval int 0 if the file modification times are equal, -1 if this file is older than target and
   1 if this file is newer than target.
*/
int
c4s::path::compare_times(path& target) const
{
    if (!target.change_time)
        target.read_changetime();
    if (change_time < target.change_time)
        return -1;
    if (change_time > target.change_time)
        return 1;
    return 0;
}
// -------------------------------------------------------------------------------------------------
uint64_t
c4s::path::fnv_hash64() const
{
    if (base.empty() || !exists())
        return 0;
    // magic 64bit salt for fnv
    return fnv_hash64_file(get_pp(), FNV_1_PRIME);
}
// -------------------------------------------------------------------------------------------------
TIME_T
c4s::path::read_changetime()
{
    struct stat statBuffer;
    if (stat(get_path().c_str(), &statBuffer)) {
        ostringstream os;
        os << "path::read_changetime - Unable to find source file:" << get_path().c_str();
        throw path_exception(os.str());
    }
    change_time = statBuffer.st_mtime;
    return change_time;
}
// -------------------------------------------------------------------------------------------------
void
c4s::path::unix2dos()
{
    size_t offset = dir.find_first_of('/', 0);
    while (offset != string::npos) {
        dir[offset] = '\\';
        dir.find_first_of('/', offset + 1);
    }
}
// -------------------------------------------------------------------------------------------------
void
c4s::path::dos2unix()
{
    size_t offset = dir.find_first_of('\\', 0);
    while (offset != string::npos) {
        dir[offset] = '/';
        dir.find_first_of('\\', offset + 1);
    }
}
// -------------------------------------------------------------------------------------------------
inline bool
isflag(int f, const int t)
{
    return (f & t) == t ? true : false;
}
#define IS(x) isflag(flags, x)
// -------------------------------------------------------------------------------------------------
/** Copies this file into the target. In Linux, after the file is copied the owner and mode
  is changed if they have been defined for the target.
  \param to Path to target file
  \param flags See PCF constants
  \retval int Number of files copied. 1 or more if PCF_RECURSIVE is defined.
*/
int
c4s::path::cp(const path& to, int flags) const
{
    ostringstream ss;
    char rb[0x4000];
    const char* out_mode;
    FILE *f_from, *f_to;
    path tmp_to(to);
    size_t br;

    // Check for recursive copy
    if (base.empty()) {
        if (!to.base.empty())
            throw path_exception("path::cp - cannot copy directory into a file.");
        if (IS(PCF_RECURSIVE)) {
            return copy_recursive(to, flags);
        }
        throw path_exception(
            "path::cp - source is a directory and path::RECURSIVE is not defined.");
    }
    // Create the target path
    if (tmp_to.base.empty() || IS(PCF_ONAME))
        tmp_to.base = base;
    // If the target exists and force is not on: bail out.
    if (tmp_to.exists()) {
        if (IS(PCF_BACKUP)) {
            path backup(tmp_to);
            string bb(tmp_to.get_base());
            bb += "~";
            backup.ren(bb);
        } else if (!IS(PCF_FORCE)) {
            ss << "path::cp - target file exists: " << tmp_to.get_path();
            throw path_exception(ss.str());
        }
#ifdef C4S_DEBUGTRACE
        cout << "path::cp - DEBUG: forcing file overwrite:" << tmp_to.get_path() << '\n';
#endif
    }

    // Append if told so
    if (IS(PCF_APPEND))
        out_mode = "ab";
    else
        out_mode = "wb";

    // Open source file
    f_from = fopen(get_path().c_str(), "rb");
    if (!f_from) {
        ss << "path::cp - Unable to open source file: " << get_path() << "; errno=" << errno;
        throw path_exception(ss.str());
    }
    // Open target
    f_to = fopen(tmp_to.get_path().c_str(), out_mode);
    if (!f_to) {
        // If the directory did not exist: create it.
        if (!tmp_to.dirname_exists() && IS(PCF_FORCE)) {
            tmp_to.mkdir();
            f_to = fopen(tmp_to.get_path().c_str(), out_mode);
        }
        if (!f_to) {
            ss << "path::cp - unable to open target: " << tmp_to.get_path() << "; errno=" << errno;
            fclose(f_from);
            throw path_exception(ss.str());
        }
#ifdef C4S_DEBUGTRACE
        cout << "path::cp - DEBUG: Created new directory for target file\n";
#endif
    }
    // copy one chunk at the time.
    do {
        br = fread(rb, 1, sizeof(rb), f_from);
        if (fwrite(rb, 1, br, f_to) != br) {
            ss << "path::cp - output error to: " << tmp_to.get_path() << "; errno=" << errno;
            fclose(f_from);
            fclose(f_to);
            throw path_exception(ss.str());
        }
    } while (!feof(f_from));

    // Close the files and copy permissions.
    fclose(f_from);
    fclose(f_to);
    if (!IS(PCF_DEFPERM)) {
#ifdef C4S_DEBUGTRACE
        cout << "path::cp - DEBUG: Setting permissions\n";
#endif
        if (mode != -1)
            tmp_to.chmod(mode);
        else
            copy_mode(tmp_to);
        if (owner) {
            tmp_to.owner = owner;
            tmp_to.owner_write();
        }
    }

    // If this was a move operation, remove the source file.
    if (IS(PCF_MOVE))
        rm();
    return 1;
}
// -------------------------------------------------------------------------------------------------
void
c4s::path::cat(const path& tail) const
/** Concatenates given file into file pointed by this path
  \param tail Ref to path that is added to the end of this file.
*/
{
    ostringstream ss;
    char rb[1024];

    // Check that base is not empty;
    if (base.empty())
        throw path_exception("path::cat - cannot cat to directory");
    // Open this file
    fstream target(get_path().c_str(), ios::in | ios::out | ios::ate | ios::binary);
    if (!target) {
        ss << "path::cat - Unable to open target file: " << get_path();
        throw path_exception(ss.str());
    }

    // Open tail file
    ifstream tfil(tail.get_path().c_str(), ios::in | ios::binary);
    if (!tfil) {
        target.close();
        ss << "path::cat - unable to open file to concatenate: " << tail.get_path();
        throw path_exception(ss.str());
    }
    // copy one chunk at the time.
    do {
        tfil.read(rb, sizeof(rb));
        target.write(rb, tfil.gcount());
        if (target.fail()) {
            target.close();
            tfil.close();
            ss << "path::cat - unable to write to the cat target: " << get_path();
            throw path_exception(ss.str());
        }
    } while (!tfil.eof());
    // Close the files
    target.close();
    tfil.close();
}
// -------------------------------------------------------------------------------------------------
/**  In Windos this only copies file time-attributes only.

  \param target Path to target where the attributes are copied into.
*/
void
c4s::path::copy_mode(const path& target) const
{
    ostringstream ss;
    int src = open(get_path().c_str(), O_RDONLY);
    if (src == -1) {
        ss << "path::cp - unable to open source: " << get_path() << '\n';
        throw path_exception(ss.str());
    }
    int tgt = open(target.get_path().c_str(), O_WRONLY);
    if (tgt == -1) {
        ss << "path::cp - unable to open target: " << target.get_path() << '\n';
        throw path_exception(ss.str());
    }

    struct stat sbuf;
    fstat(src, &sbuf);
    fchmod(tgt, sbuf.st_mode);
    // cout << "Debug: mode copied - "<<hex<<sbuf.st_mode<<dec<<'\n';
    close(src);
    close(tgt);
}
// -------------------------------------------------------------------------------------------------
int
c4s::path::copy_recursive(const path& target, int flags) const
/** Copies everything from this directory to target. If this object has base defined it will be
  ignored. If the target does not exist it will be created (recursively). If files exist in target
  they will be copied over. File times are preserved. Only regular files are copied. \param target
  Target directory for the copied files.
*/
{
    int copy_count = 0;
    // Open the directory
    DIR* source_dir = opendir(dir.c_str());
    if (!source_dir) {
        ostringstream os;
        os << "path::cpr - Unable to access directory: " << dir << '\n' << strerror(errno);
        throw path_exception(os.str().c_str());
    }

    // Make sure the target directory exists
    if (!target.dirname_exists())
        target.mkdir();

    // Read the source directory
    path cp_source;
    string file_name;
    struct stat file_stat;
    struct dirent* de = readdir(source_dir);
    while (de) {
        cp_source.dir = dir;

        // Get the file's lstat. The dirent type is not reliable
        file_name = dir;
        file_name += de->d_name;
        if (!lstat(file_name.c_str(), &file_stat)) {
            // If entry is a regular file: copy it
            if (S_ISREG(file_stat.st_mode)) {
                cp_source.base = de->d_name;
                copy_count += cp_source.cp(target, flags);
            }
            // Else if directory:
            else if (S_ISDIR(file_stat.st_mode) && de->d_name[0] != '.') {
                cp_source.dir += de->d_name;
                cp_source.dir += C4S_DSEP;
                path sub_target(target);
                sub_target.dir += de->d_name;
                sub_target.dir += C4S_DSEP;
                copy_count += cp_source.copy_recursive(sub_target, flags);
            }
        }
        de = readdir(source_dir);
    }
    closedir(source_dir);
    return copy_count;
}
// -------------------------------------------------------------------------------------------------
/**
  Renames the file pointed by this path. Only the base name is affected. Use copy function to
  move the file to another directory.
  \param new_base New base name for the path
  \param force If new file alerady exist, it is deleted if force is true. Otherwice exception is
  thrown.
*/
void
c4s::path::ren(const string& new_base, bool force)
{
    string old_base = base;
    string old = get_path();
    set_base(new_base);
    string nw = get_path();
    if (exists()) {
        if (force) {
            if (!rm()) {
                set_base(old_base);
                throw path_exception("path::ren - unable to remove existing file.");
            }
        } else {
            set_base(old_base);
            throw path_exception("path::ren - target already exist.");
        }
    }
    if (rename(old.c_str(), nw.c_str()) == -1) {
        set_base(old_base);
        ostringstream ss;
        ss << "path::ren from " << old << " to " << nw << " - error: " << strerror(errno);
        throw path_exception(ss.str());
    }
}
// -------------------------------------------------------------------------------------------------
/**
  Removes the file from the disk. Removes empty directories as well. If file does not exist or
  directory has files this function returns false. Other errors cause exception. USE WITH CARE!!
  \retval bool True if deletion is successful or if file does not exist. False otherwise.
*/
bool
c4s::path::rm() const
{
    string name = base.empty() ? get_dir_plain() : get_path();
    if (unlink(name.c_str()) < 0) {
        if (errno == ENOENT)
            return true;
        if (errno == EPERM) {
            if (::rmdir(name.c_str()) < 0)
                return false;
            return true;
        }
        ostringstream estr;
        estr << "path::rm - unable to delete" << get_path() << "; error:" << strerror(errno);
        throw path_exception(estr.str());
    }
    return true;
}
// -------------------------------------------------------------------------------------------------
/**
  Creates a named symbolic link to current path.
  \param link Name of the link
*/
void
c4s::path::symlink(const path& link) const
{
    string source;
    if (base.empty()) {
        if (!dirname_exists()) {
            ostringstream os;
            os << "path::symlink - Symbolic link target dir:" << get_path() << " does no exist";
            throw path_exception(os.str().c_str());
        }
        source = get_dir_plain();
    } else {
        if (!exists()) {
            ostringstream os;
            os << "path::symlink - Symbolic link target:" << get_path() << " does no exist";
            throw path_exception(os.str().c_str());
        }
        source = get_path();
    }
    string linkname = link.base.empty() ? link.get_dir_plain() : link.get_path();
    if (::symlink(source.c_str(), linkname.c_str())) {
        ostringstream os;
        os << "path::symlink - Unable to create link '" << linkname << "' to '" << source << "' - "
           << strerror(errno);
        throw path_exception(os.str().c_str());
    }
}
// -------------------------------------------------------------------------------------------------
/**  In Windows environment file is changed to read-only if user write flag is not set.

  \param mode_in New mode to set. Use hex values for representing permissions. If default value -1
  is used then the mode specified in constructor will be used. If nothing was specified at
  constructor function does nothing.
*/
void
c4s::path::chmod(int mode_in)
{
    ostringstream os;
    if (mode_in == -1) {
        if (mode >= 0)
            mode_in = mode;
        else
            return;
    } else if (mode < 0)
        mode = mode_in;
    mode_t final = hex2mode(mode_in);
    if (::chmod(get_path().c_str(), final) == -1) {
        os << "path::chmod failed - " << get_path() << " - Error:" << strerror(errno);
        throw path_exception(os.str());
    }
}
// -------------------------------------------------------------------------------------------------
/**
  Sample: if this path's directory is '/original/short/' and src is '/path/to/append/' this path
  will be '/original/short/append/' after this function is called.
 */
void
c4s::path::append_last(const path& src)
{
    if (src.dir.empty())
        return;
    size_t dirIndex = src.dir.find_last_of(C4S_DSEP, src.dir.size() - 2);
    if (dirIndex == string::npos)
        return;
    dir += src.dir.substr(dirIndex + 1);
}
// -------------------------------------------------------------------------------------------------
void
c4s::path::append_dir(const char* srcdir)
{
    dir += srcdir;
    if (dir.at(dir.length() - 1) != C4S_DSEP)
        dir += '/';
}
// -------------------------------------------------------------------------------------------------
void
c4s::path::dump(ostream& out)
{
    out << "Path == dir:" << dir << "; base:" << base << "; flag:";
    if (flag)
        cout << "true; ";
    else
        cout << "false; ";
    cout << "time:" << change_time;
    cout << "; mode:" << hex << mode << dec << "; ";
    if (owner)
        owner->dump(out);
    else
        out << "owner: NULL;\n";
}
// -------------------------------------------------------------------------------------------------
/** All instances of the search text are replaced. Thows an exception if files cannot be opened or
  written.
  \param search String to search for
  \param replace Text that will be written instead of search string.
  \param backup If true the original file will be backed up.
  \retval int Number of replacements done.
 */
int
c4s::path::search_replace(const string& search, const string& replace, bool backup)
{
    char *btr, buffer[0x800];
    int count = 0;
    SIZE_T reread = 0, find_pos;
    streamsize br;
    fstream src(get_path().c_str());
    if (!src)
        throw path_exception("path::search_replace - Unable to open file.");
    path target(dir, base, ".~c4s");
    ofstream tgt(target.get_path().c_str());
    if (!tgt) {
        src.close();
        throw path_exception("path::search_replace - Unable to open temporary file.");
    }
    if (search.size() > sizeof(buffer))
        throw path_exception(
            "path::search_replace - Search string size exceeds the internal buffer size.");

    do {
        src.read(buffer + reread, sizeof(buffer) - reread);
        br = src.gcount();
        btr = buffer;
        do {
            if (search_bmh((unsigned char*)btr, (SIZE_T)br, (unsigned char*)search.c_str(),
                           search.size(), &find_pos)) {
                count++;
                tgt.write(btr, find_pos);
                tgt.write(replace.c_str(), replace.size());
                btr += find_pos + search.size();
                br -= find_pos + search.size();
            } else {
                tgt.write(btr, br);
                br = 0;
            }
        } while (br > 0);
    } while (!src.eof());
    src.close();
    tgt.close();

    if (count) {
        // Rename the source (to make backup) and target as source.
        try {
            string sbase_old = base;
            if (backup) {
                string sbase_backup(base + "~");
                ren(sbase_backup, true);
            } else
                rm();
            target.ren(sbase_old, true);
            (*this) = target;
        } catch (const c4s_exception&) {
            throw path_exception("path::search_replace - temp file rename error.");
        }
    }
    return count;
}
// -------------------------------------------------------------------------------------------------
/** Relaces the text between start and end tags with the given replacement string. Only first
  instance is replaced. Replace tags are left in place, only text in between is replaced. Throws
  exception if file cannot be opened or written and other read/write errors. \param start_tag Start
  tag. \param end_tag End tag. \param rpl_txt Replacemnt text. \param backup If true then original
  file is backed up. \retval bool True if replacement was done. False if start or end tag was not
  found. */
bool
c4s::path::replace_block(const string& start_tag,
                         const string& end_tag,
                         const string& rpl_txt,
                         bool backup)
{
    char buffer[0x1000];
    streamsize br, soffset, eoffset;

    if (base.empty())
        throw path_exception("path::replace_block - This path is a directory and replace function "
                             "cannot be applied.");
    // Search strings cannot exceed the buffer size.
    if (start_tag.size() >= (SSIZE_T)sizeof(buffer) || end_tag.size() >= (SSIZE_T)sizeof(buffer))
        throw path_exception(
            "path::replace_block - Tag size too big. Exceeds internal buffer size.");
    fstream src(get_path().c_str(), ios_base::in | ios_base::out | ios_base::binary);
    if (!src)
        throw path_exception("path::replace_block - Unable to open file.");

    // Search the start and end tags.
    if (!search_file(src, start_tag)) {
        src.close();
        return false;
    }
    soffset = src.tellg();
    soffset += start_tag.size();
    src.seekg(start_tag.size(), ios_base::cur);
    if (!search_file(src, end_tag)) {
        src.close();
        return false;
    }
    eoffset = src.tellg();

    // Do the replacing
    src.seekg(0, ios_base::beg);
    path target(dir, base, ".~c4s");
    ofstream tgt(target.get_path().c_str(), ios_base::out | ios_base::binary);
    if (!tgt) {
        src.close();
        throw path_exception("path::replace_block - Unable to open temporary file.");
    }
    // Copy until replacement start
    for (SIZE_T ndx = 0; ndx < soffset / sizeof(buffer); ndx++) {
        src.read(buffer, sizeof(buffer));
        if (src.gcount() != sizeof(buffer)) {
            src.close();
            tgt.close();
            target.rm();
            throw path_exception("path::replace_block - Read size mismatch. Aborting replace.");
        }
        tgt.write(buffer, sizeof(buffer));
    }
    src.read(buffer, soffset % sizeof(buffer));
    if ((SIZE_T)src.gcount() != soffset % sizeof(buffer)) {
        src.close();
        tgt.close();
        target.rm();
        throw path_exception("path::replace_block - Read size mismatch(2). Aborting replace.");
    }
    tgt.write(buffer, soffset % sizeof(buffer));

    // Copy rpl_txtment and write the rest.
    tgt.write(rpl_txt.c_str(), rpl_txt.size());
    src.seekg(eoffset, ios_base::beg);
    while (!src.eof()) {
        src.read(buffer, sizeof(buffer));
        br = src.gcount();
        tgt.write(buffer, br);
    }
    src.close();
    tgt.close();

    // Rename the source (to make backup) and target as source.
    try {
        string sbase_old = base;
        if (backup) {
            string sbase_backup(base + "~");
            ren(sbase_backup, true);
        } else
            rm();
        target.ren(sbase_old, true);
        (*this) = target;
    } catch (const c4s_exception&) {
        throw path_exception("path::replace_block - temp file rename error.");
    }
    return true;
}
