/* This file is part of 'CPP for Scripts' C++ library (libc4s)
 * https://github.com/jaaskelainen-aj/cpp4scripts
 *
 * Note: This file is also part of libfcgi.
 *
 * Copyright (c) 2021: Antti Jääskeläinen
 * License: http://www.gnu.org/licenses/lgpl-2.1.html
 */
#include <unistd.h>
#include <stdexcept>
#include <string.h>

#include "RingBuffer.hpp"

namespace c4s {

// -------------------------------------------------------------------------------------------------
RingBuffer::RingBuffer(size_t max)
{
    initialize(max);
}

void RingBuffer::initialize(size_t max)
{
    rb_max = max;
    if (!rb_max) {
        rb = nullptr;
    } else {
        eof = false;
        rb = new char[rb_max];
        memset(rb, 0, rb_max);
    }
    last_read = 0;

    reptr = rb;
    wrptr = rb;
    eof = false;
    end = wrptr + rb_max;
}
// -------------------------------------------------------------------------------------------------
RingBuffer::~RingBuffer()
{
    if (rb)
        delete[] rb;
}
// -------------------------------------------------------------------------------------------------
void RingBuffer::reallocate(size_t max)
{
    if (rb)
        delete[] rb;
    initialize(max);
}
// -------------------------------------------------------------------------------------------------
size_t
RingBuffer::write(const void* input, size_t slen)
{
    if (eof || !input || !slen)
        return 0;
    size_t maxwrite = capacity_internal();
    if (maxwrite < slen)
        slen = maxwrite;

    size_t fp = end - wrptr;
    if (slen < fp) {
        memcpy(wrptr, input, slen);
        wrptr += slen;
        if (wrptr == end)
            wrptr = rb;
    } else {
        memcpy(wrptr, input, fp);
        memcpy(rb, (char*)input + fp, slen - fp);
        wrptr = rb + (slen - fp);
    }
    if (wrptr == reptr)
        eof = true;
    return slen;
}
// -------------------------------------------------------------------------------------------------
size_t
RingBuffer::write_from(int fd)
{
    ssize_t fd_bytes1 = 0;
    ssize_t fd_bytes2 = 0;
    if (eof || !fd)
        return 0;
    if (wrptr >= reptr) {
        fd_bytes1 = ::read(fd, wrptr, end - wrptr);
        if (fd_bytes1 < 0) {
            if (errno == EAGAIN)
                return 0;
            goto write_from_err;
        }
        wrptr += fd_bytes1;
        if (wrptr == end)
            wrptr = rb;
        if (wrptr == reptr) {
            eof = true;
            return fd_bytes1;
        }
    }
    if (wrptr < reptr) {
        fd_bytes2 = ::read(fd, wrptr, reptr - wrptr);
        if (fd_bytes2 < 0) {
            if (errno == EAGAIN)
                return 0;
            goto write_from_err;
        }
        wrptr += fd_bytes2;
        if (wrptr == reptr) {
            eof = true;
        }
    }
    return (size_t)(fd_bytes1 + fd_bytes2);

 write_from_err:
    char emsg[100];
    sprintf(emsg, "RingBuffer::write_from - read pipe %d failed with err %d", fd, errno);
    throw std::runtime_error(emsg);
    return 0;
}
// -------------------------------------------------------------------------------------------------
size_t
RingBuffer::read_data(void* store, size_t slen)
{
    if (!slen || !store || !rb) {
        return 0;
    }
    size_t ss = size_internal();
    if (!ss) {
        last_read = 0;
        return 0;
    }
    if (slen > ss)
        slen = ss;
    size_t fp = end - reptr;
    if (slen < fp) {
        memcpy(store, reptr, slen);
        reptr += slen;
        if (reptr == end)
            reptr = rb;
    } else {
        memcpy(store, reptr, fp);
        memcpy((char*)store + fp, rb, slen - fp);
        reptr = rb + (slen - fp);
    }
    eof = false;
    last_read = slen;
    return slen;
}
/*! String will be null terminated. I.e. actual amount of data read is slen-1.
 * \param str       target string.
 * \param slen      target buffer size = sizeof(target)
 */
size_t
RingBuffer::read(char* str, size_t slen)
{
    size_t rb = read_data(str, slen-1);
    str[rb] = 0;
    return rb;
}
// -------------------------------------------------------------------------------------------------
size_t
RingBuffer::read_into(std::string& output)
{
    last_read = 0;
    size_t ss = size_internal();
    if (!ss) {
        return 0;
    }
    output.reserve(ss);
    while (reptr != wrptr || eof) {
        output.push_back(*reptr);
        reptr++;
        last_read++;
        if (reptr == end)
            reptr = rb;
        eof = false;
    }
    return last_read;
}
// -------------------------------------------------------------------------------------------------
size_t
RingBuffer::read_into(std::ostream& output)
{
    last_read = 0;
    size_t ss = size_internal();
    if (!ss) {
        return 0;
    }
    while (reptr != wrptr || eof) {
        output << *reptr;
        reptr++;
        last_read++;
        if (reptr == end)
            reptr = rb;
        eof = false;
    }
    return last_read;
}
// -------------------------------------------------------------------------------------------------
size_t
RingBuffer::read_line(std::ostream& output, bool partial_ok)
{
    last_read = 0;
    if (!rb)
        return 0;
    if (!partial_ok && !is_line_available())
        return 0;
    while ((reptr != wrptr || eof) && *reptr != '\n') {
        output.put(*reptr);
        reptr++;
        last_read++;
        if (reptr == end)
            reptr = rb;
        eof = false;
    }
    if (*reptr == '\n') {
        reptr++;
        if (reptr == end)
            reptr = rb;
    }
    return last_read;
}
bool
RingBuffer::is_line_available() const
{
    char* check = reptr;
    while (check != wrptr && *check != '\n') {
        check++;
        if (check == end)
            check = rb;
    }
    if (*check != '\n')
        return false;
    return true;
}
size_t
RingBuffer::read_line(char* line, size_t len, bool partial_ok)
{
    last_read = 0;
    if (!rb || len < 2)
        return 0;
    if (!partial_ok && !is_line_available())
        return 0;
    len--; // Save room for null byte.
    while ((reptr != wrptr || eof) && *reptr != '\n' && last_read < len) {
        *line = *reptr;
        line++;
        reptr++;
        last_read++;
        if (reptr == end)
            reptr = rb;
        eof = false;
    }
    if (reptr != wrptr && *reptr == '\n') {
        reptr++;
        if (reptr == end)
            reptr = rb;
    }
    if (last_read)
        *line = 0;
    return last_read;
}
// -------------------------------------------------------------------------------------------------
size_t
RingBuffer::read_into(int fd, size_t slen)
{
    if (!slen || fd < 0) {
        return 0;
    }
    size_t ss = size_internal();
    if (!ss) {
        last_read = 0;
        return 0;
    }
    if (slen > ss)
        slen = ss;
    size_t fp = end - reptr;
    if (slen < fp) {
        ::write(fd, reptr, slen);
        reptr += slen;
        if (reptr == end)
            reptr = rb;
    } else {
        ::write(fd, reptr, fp);
        ::write(fd, rb, slen - fp);
        reptr = rb + (slen - fp);
    }
    eof = false;
    last_read = slen;
    return slen;
}
// -------------------------------------------------------------------------------------------------
size_t
RingBuffer::read_max(void* store, size_t store_size, size_t max, bool partial)
{
    if (!rb)
        return 0;
    if (store_size >= max)
        return read_data(store, max);
    size_t br = 0;
    if (partial)
        br = read_data(store, store_size);
    discard(max - br);
    return br;
}

// -------------------------------------------------------------------------------------------------
size_t
RingBuffer::peek(void* store, size_t slen)
{
    if (!slen || !store)
        return 0;
    size_t ss = size_internal();
    if (!ss) {
        return 0;
    }
    if (slen > ss)
        slen = ss;
    size_t fp = end - reptr;
    if (slen < fp) {
        memcpy(store, reptr, slen);
    } else {
        memcpy(store, reptr, fp);
        memcpy((char*)store + fp, rb, slen - fp);
    }
    return slen;
}

// -------------------------------------------------------------------------------------------------
size_t
RingBuffer::exp_as_text(std::ostream& os, size_t slen, EXP_TYPE type)
{
    unsigned short int sich;
    char ch;
    if (!slen) {
        return 0;
    }
    size_t ss = size_internal();
    if (!ss) {
        last_read = 0;
        return 0;
    }
    if (slen > ss)
        slen = ss;
    if (type == HEX)
        os << std::hex;
    while (reptr != wrptr && slen) {
        ch = *reptr;
        if (type == HEX) {
            sich = 0xff & ((unsigned short)ch);
            os << sich << ',';
        } else
            os << ch;
        reptr++;
        slen--;
        if (reptr == end)
            reptr = rb;
    }
    if (type == HEX)
        os << std::dec;
    eof = false;
    last_read = slen;
    return slen;
}
// -------------------------------------------------------------------------------------------------
size_t
RingBuffer::discard(size_t slen)
{
    if (!slen) {
        return 0;
    }
    size_t ss = size_internal();
    if (!ss) {
        return 0;
    }
    if (slen > ss)
        slen = ss;
    while ((reptr != wrptr || eof) && slen) {
        reptr++;
        slen--;
        if (reptr == end)
            reptr = rb;
        eof = false;
    }
    return slen;
}

// -------------------------------------------------------------------------------------------------
bool
RingBuffer::unread(size_t length)
{
    if (!rb)
        return false;
    size_t rewind = length ? length : last_read;
    if (!rewind)
        return true;
    if (capacity_internal() < rewind)
        return false;
    reptr -= rewind;
    if (reptr < rb)
        reptr = end - (rb - reptr);
    return true;
}

size_t
RingBuffer::size_internal() const
//! Returns number of bytes waiting for reading.
{
    if (!rb)
        return 0;
    if (eof)
        return rb_max;
    if (wrptr == reptr)
        return 0;
    if (wrptr > reptr)
        return wrptr - reptr;
    return (end - reptr) + (wrptr - rb);
}

size_t
RingBuffer::capacity_internal() const
//! Returns the remaining write size
{
    if (eof || !rb)
        return 0;
    if (wrptr == reptr)
        return rb_max;
    if (wrptr > reptr)
        return (end - wrptr) + (reptr - rb);
    return reptr - wrptr;
}

// -------------------------------------------------------------------------------------------------
void
RingBuffer::dump(std::ostream& os)
{
    if (!rb)
        return;
    os << "RingBuffer:\n  begin:" << (const void*)rb << "; reptr:" << (const void*)reptr
       << "; wrptr:" << (const void*)wrptr << ";\n";
    os << "  size:" << size_internal() << "; capacity:" << capacity_internal()
       << "; rb_max:" << rb_max << "; eof:";
    if (eof)
        os << "true;";
    else
        os << "false;";
    if (rb_max < 100) {
        os << " chars:";
        for (char* ndx = rb; ndx < end; ndx++) {
            if (*ndx > 31 && *ndx < 127)
                os << *ndx;
            else
                os << '.';
        }
    }
    os << '\n';
}

} // namespace fcgi_driver