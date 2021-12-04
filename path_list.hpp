/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */
#ifndef C4S_PATH_LIST_HPP
#define C4S_PATH_LIST_HPP

namespace c4s {

typedef std::list<path>::iterator path_iterator;
/** \defgroup PathListFlags Flags for adding files into the list
    @{
*/
const int PLF_NONE = 0;    //!< Add normal files only.
const int PLF_DIRS = 0x1;  //!< Include directories.
const int PLF_SYML = 0x2;  //!< Include symbolic links.
const int PLF_NOREG = 0x4; //!< Discard regular files.
const int PLF_NOSEARCHDIR = 0x8; //!< Don't include search target directory into results. 
/**@}*/

// -----------------------------------------------------------------------------------------------------------
//! List of paths.
/*! Class is provided for convenience. Mimics STL list container. Class allows developer to
    perform single operation over multiple files.
*/
class path_list
{
  public:
    //! Default constructor
    path_list() {}
    //! Constructs path list by adding given PATH like string into the list.
    /*! \param str List of paths
       \param sep Path separator used in str */
    path_list(const char* str, const char sep) { add(str, sep); }
    /// Creates a list of paths from the source list.
    path_list(const path_list& pl, const std::string& dir, const char* ext = 0)
    {
        add(pl, dir, ext);
    }
    /// Constructs list by reading given directory with supplied wild card.
    /** \param target Path to the target directory. Only dir-part is considered.
       \param grep Grep regular expression to match included files. If null includes all files.
       \param plo See \sa PathListFlags
       \param exex Regular expression of files to exclude.*/
    path_list(const path& target,
              const std::string& grep,
              int plo = PLF_NONE,
              const std::string& exex = std::string());

    //! Adds a given path to the list
    void operator+=(const path& p) { add(p); }
    //! Appends the given list into this list.
    void operator+=(const path_list& pl) { add(pl); }

    //! Returns iterator to the beginning of list.
    path_iterator begin() { return plist.begin(); }
    //! Returns iterator to the end of list.
    path_iterator end() { return plist.end(); }
    //! Returns number of paths in the list.
    size_t size() { return plist.size(); }
    //! Returns true if list is empty.
    bool empty() { return plist.empty(); }

    //! Parses the given string and adds paths to the list.
    size_t add(const char* str, const char separator = C4S_PSEP);
    //! Appends given list into this one.
    size_t add(const path_list& pl);
    //! Append single path to the list
    void add(const path& p) { plist.push_back(p); }
    //! Adds all files from the given directory that match the given grep regular expression.
    size_t add(const path& p,
               const std::string& grep,
               int plo = PLF_NONE,
               const std::string& exex = std::string());
    //! Appends source files to path-list
    size_t add(const path_list& pl, const std::string&, const char* ext = 0);
    //! Appends source files recursively starting from the path given.
    size_t add_recursive(const path& p, const char* wild);
    //! Discards a path referenced by this iterator.
    void discard(path_iterator& pi) { plist.erase(pi); }
    //! Finds the name of the given base from the list and discards it from the list.
    bool discard_matching(const std::string&);
    //! Copies this list of files to given target directory.
    int copy_to(const path&, int flag = PCF_NONE);
    //! Changes the given mode to all paths.
    void chmod(int mod);
    //! Copies the given string to as directory to all paths in the list.
    void set_dir(path& p) { set_dir(p.get_dir()); }
    //! Copies the given string to as directory to all paths in the list.
    void set_dir(const std::string& p);
    //! Sets the extension for all files in the list.
    void set_ext(const std::string& e);
#if defined(__linux) || defined(__APPLE__)
    //! Sets the same user and mode to all paths in the list. Note, will not commit changes to disk.
    void set_usermode(user*, const int);
#endif
    //! Deletes all files specified in this list from the disk.
    void rm_all();
    //! Creates a list of compilation targets from this source list.
    void create_targets(path_list& target, const std::string& dir, const char* ext);
    //! Returns the paths as a string separating them with given separator.
    std::string str(const char separator, bool baseonly = true);

    //! Returns the first path in the list.
    path front() { return plist.front(); }
    //! Returns the last path in the list.
    path back() { return plist.back(); }
    //! Sorts files in alphabetical order. (PARTIAL = Only base part is considered.)
    enum SORTTYPE
    {
        ST_PARTIAL,
        ST_FULL
    };
    void sort(SORTTYPE);

    //! outputs the dump of each path in this list to given stream.
    void dump(std::ostream&);

  protected:
    std::list<path> plist;
};

}
#endif
