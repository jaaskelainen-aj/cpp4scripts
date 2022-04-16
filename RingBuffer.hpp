/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Note: This file is also part of libfcgi.
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 * Disclaimer of Warranty: Work is provided on an "as is" basis, without warranties or conditions of
 * any kind
 */
#ifndef C4S_RINGBUFFER_HPP
#define C4S_RINGBUFFER_HPP

#include <iostream>
#include "ntbs/ntbs.hpp"

namespace c4s {

class RBCallBack
{
  public:
    RBCallBack() {}
    virtual ~RBCallBack() {}
    virtual void init_push(size_t) = 0;
    virtual void push_back(char ch) = 0;
    virtual void end_push() = 0;
};

class RingBuffer
{
  public:
    RingBuffer(size_t max=0);
    ~RingBuffer();

    size_t write(const void*, size_t);
    size_t write_from(int fd);
    //! Read binary data.
    size_t read_data(void*, size_t);
    //! Read character string.
    size_t read(char*, size_t);
    size_t read_into(ntbs*);
    size_t read_into(std::string&);
    size_t read_into(std::ostream&);
    size_t read_into(int fd, size_t len);
    size_t read_line(std::ostream&, bool partial_ok = false);
    size_t read_line(char* line, size_t len, bool partial_ok = false);
    size_t read_max(void*, size_t, size_t, bool);
    size_t peek(void*, size_t);

    bool is_eof() const { return eof; }
    size_t size() const { return size_internal(); }
    void clear()
    {
        reptr = wrptr;
        eof = false;
    }
    size_t capacity() { return capacity_internal(); }
    size_t gcount() { return last_read; }
    bool unread(size_t len = 0);
    size_t copy(RingBuffer&, size_t);
    size_t push_to(RBCallBack*, size_t);
    size_t max_size() const { return rb_max; }
    void max_size(size_t);

    enum EXP_TYPE
    {
        TEXT,
        HEX
    };
    size_t exp_as_text(std::ostream&, size_t, EXP_TYPE);
    size_t discard(size_t);
    void dump(std::ostream&);

  protected:
    void initialize(size_t);
    size_t size_internal() const;
    size_t capacity_internal() const;
    bool is_line_available() const;

    size_t rb_max, last_read;
    char* rb;
    char* reptr;
    char* wrptr;
    char* end;
    bool eof;
};

} // namespace c4s

#endif