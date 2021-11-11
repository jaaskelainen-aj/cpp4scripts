/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */

#ifndef C4S_PATH_STACK
#define C4S_PATH_STACK

namespace c4s {
// -----------------------------------------------------------------------------------------------------------
//! Stack of current directories.
/* Designed to be used when current directory needs to be changed during program execution. Pushing
   new path causes the current directory to be stored into the stack and then current directory is
   changed to pushed directory. Popping a path reverses the action. When stack is deleted original
   path is restored.
*/
class path_stack
{
  public:
    //! Initializes an empty stack.
    path_stack() {}
    //! Changes into given directory and saves the original into the stack.
    path_stack(const path& cdto) { push(cdto); }
    //! Destroys the stack and restores the original directory.
    ~path_stack()
    {
        if (pstack.size() > 0)
            pstack.front().cd();
    }
    //! Pushes current dir to stack and changes to given directory
    void push(const char* cdto)
    {
        path p;
        p.read_cwd();
        pstack.push_back(p);
        path::cd(cdto);
    }
    //! Pushes current dir to stack and changes to given directory
    void push(const path& cdto)
    {
        path p;
        p.read_cwd();
        pstack.push_back(p);
        cdto.cd();
    }
    //! Pushes the 'from' directory into stack and changes to 'to'
    void push(const path& to, const path& from)
    {
        pstack.push_back(from);
        to.cd();
    }
    //! Changes into the topmost path and pops it out of stack.
    void pop()
    {
        if (pstack.size() > 0) {
            pstack.back().cd();
            pstack.pop_back();
        }
    }
    //! Changes into first (bottom) path and removes all stack items.
    void pop_all()
    {
        pstack.front().cd();
        pstack.clear();
    }
    //! Retuns the first path from the 'bottom' of the stack. Stack is not altered.
    path start() { return pstack.front(); }
    //! Returns number of items in the stack.
    size_t size() { return pstack.size(); }

  protected:
    std::list<path> pstack;
};

}
#endif
