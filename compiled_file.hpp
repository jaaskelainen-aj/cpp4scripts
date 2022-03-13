/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */

#ifndef C4S_COMPILED_FILE_HPP
#define C4S_COMPILED_FILE_HPP

namespace c4s {

// -----------------------------------------------------------------------------------------------------------
//! Defines a file that is compiled from source to the target.
class compiled_file
{
  public:
    //! Default constructor.
    compiled_file() {}
    //! Constructor sets the source and target
    compiled_file(const char* s, const char* t)
      : source(s)
      , target(t)
    {}
    //! Constructor sets the source and target
    compiled_file(const char* s, const path& t)
      : source(s)
      , target(t)
    {}
    //! Constructor sets the source and target
    compiled_file(const path& s, const path& t)
      : source(s)
      , target(t)
    {}
    //! Constructor copies the source and builds the target from tdir and ext.
    compiled_file(const path& s, const std::string& tdir, const char* ext)
      : source(s)
      , target(tdir, s.get_base(ext))
    {}
    //! Constructor copies source and target as they are given.
    compiled_file(const path& s, const std::string& t)
      : source(s)
      , target(t)
    {}

    //! Sets the source and builds the target from given parameters..
    /*! Source path is copied as it is. Target is build by taking the tdir as directory, base from
      source changing its extension to ext. \param s Source directory \param tdir Target directory
      \param ext Extension for the target base. Filename of the target is copied from base of the
      source. */
    void set(const path& s, const std::string& tdir, const char* ext)
    {
        source = s;
        target.set(tdir, s.get_base(), ext);
    }
    //! Sets the source and target from given parameters.
    void set(const std::string s, const std::string t)
    {
        source = s;
        target = t;
    }

    //! Clears the attributes.
    void clear()
    {
        source.clear();
        target.clear();
    }
    //! Returns true if either source or target is empty.
    bool empty() { return source.empty() || target.empty(); }
    //! Returs true if source is newer than target, i.e. should be compiled.
    bool outdated() { return source.outdated(target); }
    //! Swaps the source and target
    void swap()
    {
        path tmp;
        tmp = source;
        source = target;
        target = tmp;
    }
#ifdef __linux
    //! Copies the source over the target
    void update()
    {
        source.cp(target, PCF_FORCE);
        target.ch_owner_mode();
    }
#else
    //! Copies the source over the target
    void update() { source.cp(target, PCF_FORCE); }
#endif
    path source; //!< Path to the source file
    path target; //!< Path to the target file.
};

}
#endif
