/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */
#ifndef C4S_PATH_HPP
#define C4S_PATH_HPP

#if defined(__linux) || defined(__APPLE__)
#define TIME_T time_t
#else
#define TIME_T long long int
#endif

#include <vector>

namespace c4s {

class path_list;
class user;

//! Flags and options for path's copy function.
const int PCF_NONE = 0;     //!< None.
const int PCF_FORCE = 0x1;  //!< Force / overwrite if exists.
const int PCF_MOVE = 0x2;   //!< Move the file i.e. delete original.
const int PCF_APPEND = 0x4; //!< Append to possibly existing file.
const int PCF_ONAME = 0x8;  //!< Use original base name for the target.
const int PCF_DEFPERM =
    0x10; //!< Use default file permissions given by operating system. Note, this also overrides
          //!< possible user and permission settings for the target.
const int PCF_BACKUP = 0x20; //!< Backup original if it exists.
const int PCF_RECURSIVE =
    0x40; //!< Copy recursively. Valid only if source is a directory (i.e. base is empty).

//! Flags for path compare function.
const unsigned char CMP_DIR = 1;  //!< Compare Dir parts together
const unsigned char CMP_BASE = 2; //!< Compare Base parts together

enum class OWNER_STATUS : unsigned short int
{
    OK,          /// Owner+mode exists and matches actual owner+mode
    EMPTY,       /// Owner not specified
    MISSING,     /// Owner does not exist in system
    NOPATH,      /// Path does not exist
    NOMATCH_UG,  /// Actual owner/group does not match given user/group
    NOMATCH_MODE /// Actual mode does not match given mode.
};

// -----------------------------------------------------------------------------------------------------------
//! Class that encapsulates a path to a file or directory.
/*! Path has directory part (dir) and file name part (base). File name includes the extension if
  there is one. Dir and base can be set and queried together or separately. If set together the
  library separates the dir part from base. Path can be relative.<br> Defines:<br>
  C4S_FORCE_NATIVE_PATH = If this define is set during compile time, it enforces native path
  separators to directory. It is recommended if the same code is to be used in different
  environments.
*/
class path
{
  public:
    //! Default constructor that initializes empty path.
    path();
    //! Copy constructor.
    path(const path& p) { *this = p; }
    //! Constructs path from dir part and given base.
    path(const path& dir, const char* base);
    //! Constructs path from single string.
    path(const std::string& p);
    //! Path constructor. Combines path from directory, base and extension.
    path(const std::string& d, const std::string& b, const std::string& e);
    path(const char* d, const char* b, const char* e);
    //! Constructs a path from directory and base
    path(const std::string& d, const std::string& b);
    //! Constructs a path from directory and base
    path(const std::string& d, const char* b);
    //! Constructs a path from directory and base
    path(const char* d, const char* b);
#if defined(__linux) || defined(__APPLE__)
    //! Constructs path with user data
    path(const std::string& d, const std::string& b, user* o, int m = -1);
    //! Constructs path with user data
    path(const std::string& p, user* o, int m = -1);
#endif

    //! Sets path so that it equals another path.
    void operator=(const path& p);
    //! Sets the path from pointer to const char.
    void operator=(const char* p) { set(std::string(p)); }
    //! Sets the path from constant string.
    void operator=(const std::string& p) { set(p); }
    //! Synonym for merge() function
    void operator+=(const path& p) { merge(p); }
    //! Synonym for merge() function
    void operator+=(const char* cp) { merge(path(cp)); }

    //! Clears the path.
    void clear()
    {
        change_time = 0;
        dir.clear();
        base.clear();
    }
    //! Checks whether the path is clear (or empty). \retval bool True if empty.
    bool empty() { return dir.empty() && base.empty(); }

    //! Returns the directory part of full path
    std::string get_dir() const { return dir; }
    //! Returns the directory part without the trailin slash.
    std::string get_dir_plain() const { return dir.substr(0, dir.size() - 1); }
    //! Returns the directory part as array of sub-directories
    void get_dir_parts(std::vector<std::string>& vs) const;
    //! Returns the base part with extension.
    std::string get_base() const { return base; }
    //! Returns the base and swaps its extension to the one given as parameter.
    std::string get_base(const std::string& ext) const;
    //! Returns the base without the extension
    std::string get_base_plain() const;
    //! Returns the base or if it is empty the last directory entry.
    std::string get_base_or_dir();
    //! Returns the extension from base if there is any.
    std::string get_ext() const;
    //! Returns the complete path.
    std::string get_path() const { return dir + base; }
    //! Returns the full path with quotes if the file name contains any spaces.
    std::string get_path_quot() const;
    //! Returns pointer to path string. Path is overwritten at each call. Don't store for long!
    const char* get_pp() const;

    //! Sets the directory part of the path.
    void set_dir(const std::string& d);
    //! Sets the directory part to user's home directory.
    void set_dir2home();
    //! Changes the base (=file name) part of the path.
    void set_base(const std::string& b);
    //! Sets the extension for the file name.
    void set_ext(const std::string& e);
    //! Sets the path components by parsing the given string
    void set(const std::string& p);
    //! Sets path attributes from given directory name, base name and optional extension
    void set(const std::string& d, const std::string& b, const std::string& e);
    void set(const char* d, const char* b, const char* e);

    //! Sets path attributes from given directory name and base name.
    void set(const std::string& d, const std::string& b);
    //! Changes current working directory to given path.
    static void cd(const char*);
    //! Changes current working directory to the directory stored in this object.
    void cd() const { cd(dir.c_str()); }
    //! Reads the current workd directory and sets it to dir-part. Base is not affected.
    void read_cwd();

#if defined(__linux) || defined(__APPLE__)
    //! Verifies that owner exists and the is owner of this path. (Linux only)
    OWNER_STATUS owner_status();
    //! Reads the owner information from the path. (Linux only)
    void owner_read();
    //! Writes the current owner to disk, i.e. possibly changes ownership.  (Linux only)
    void owner_write();
    //! Sets the user and mode for this path. Does not commit change to disk. (Linux only)
    void set(user* u, int m)
    {
        owner = u;
        mode = m;
    }
    //! Sets the user for the path. Does not commit change to disk. (Linux only)
    void set_owner(user* u) { owner = u; }
    //! Returns true if the owner has bee defined for this path.
    bool has_owner() { return owner ? true : false; }
    //! Returns the user for this path. (Linux only)
    user* get_owner() { return owner; }
    //! Reads current path mode from file system.
    void read_mode();
#endif
    //! Returns true if path has directory part.
    bool is_dir() const { return dir.empty() ? false : true; }
    //! Returns true if path has a base i.e. filename
    bool is_base() const { return base.empty() ? false : true; }
    //! Returns true if the path is absolute, false if not.
    bool is_absolute() const;
    //! Makes the path absolute if it is relative. Otherwice it does nothing.
    void make_absolute();
    //! Makes this path absolute based on a given root directory.
    void make_absolute(const std::string& root);
    //! Makes the path relative to the current working directory.
    void make_relative();
    //! Makes the path relative to the given parent directory.
    void make_relative(const path&);
    //! Rewinds the directory down to its parent as many times as given in parameter.
    void rewind(int count = 1);
    //! Merges two paths.
    void merge(const path&);
    //! Appends the last directory from source to this directory.
    void append_last(const path&);
    //! Appends given directory to the directory part.
    void append_dir(const char*);

    //! Checks if the base exists in any of the directories specified in the given environment
    //! variable.
    bool exists_in_env_path(const char* envar, bool set_dir = false);
    //! Returns true if the directory specified by the path exists in the file system. False if not.
    bool dirname_exists() const;
    //! Checks if the directory and base exists.
    bool exists() const;

    //! Sets a selection flag.
    void flag_set() { flag = true; }
    //! Toggles the selection flag.
    void flag_toggle() { flag = !flag; }
    //! Returns the current selection.
    bool flag_get() { return flag; }

    //! compares full path or just the base depending on flag
    int compare(const path& target, unsigned char flag) const;

    //! Creates the directory specified by the directory part of the path.
    void mkdir() const;
    //! Removes the directory from the disk this path points to.
    void rmdir(bool recursive = false) const;

    //! Copy file pointed by path to a new location
    int cp(const char* to, int flags = PCF_NONE)
    {
        path target(to);
        return cp(target, flags);
    }
    //! Copy file pointed by path to a new location
    int cp(const path&, int flags = PCF_NONE) const;
    //! Concatenate file
    void cat(const path&) const;
    //! Rename the base part
    void ren(const std::string&, bool force = false);
    //! Remove / delete file.
    bool rm() const;
    //! Make symbolic link
    void symlink(const path&) const;
    //! Changes file / directory permissions.
    void chmod(int mod = -1);

    //! Reads the last change time from the disk.
    TIME_T read_changetime();
    //! Returns true if this file is newer than the given file.
    bool outdated(path& p);
    //! Checks the outdated status against a list of files.
    bool outdated(path_list& lst);
    //! Compares this files timestamp to given targets timestamp.
    int compare_times(path&) const;
    //! Calculates FNV-hash for the file.
    uint64_t fnv_hash64() const;
    //! Changes all directory separators from unix '/' to dos '\'.
    void unix2dos();
    //! Changes all directory separators from dos '\' to unix '/'.
    void dos2unix();
    //! Performs a search-replace for a file pointed by this path
    int search_replace(const std::string& search, const std::string& replace, bool bu = false);
    //! Performs a single block replacement in a file pointed by this path.
    bool replace_block(const std::string&, const std::string&, const std::string&, bool bu = false);
    // SIZE_T search_text(const std::string &needle);
    void dump(std::ostream&);

  private:
    //! Common functionality for all constructors
    void init_common();
    //! Copies attributes and permissions from this file to target.
    void copy_mode(const path& target) const;
    //! Recursive copy from this to target.
    int copy_recursive(const path&, int) const;

#if defined(__linux) || defined(__APPLE__)
    c4s::user* owner; //!< Pointer to User and group for this file's permissions
    int mode;         //!< Path/file access mode.
#endif
    TIME_T change_time; //!< Time that the file was last changed. Zero until internal function
                        //!< update_time has been called.
    std::string
        dir; //!< directory part of the path. Directory needs to end at the directory separator.
    std::string base; //!< Base name (file name) part of the path.
    bool flag;        //!< General purpose flag for application use.
    friend class path_list;
    friend bool compare_paths(c4s::path fp, c4s::path sp);
};

}
#endif
