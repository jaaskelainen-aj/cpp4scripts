#pragma once

#include <cstring>
#ifdef C4S_DEBUGTRACE
  #include <iostream>
#endif

#define NTBS(V,L)\
	char V ## _ntbs[L];\
	ntbs V(V ## _ntbs, sizeof(V ## _ntbs))

namespace c4s {

enum class ALLOC { NONE, YES, CONST };

class ntbs
{
public:
	ntbs(size_t _max = 0);
	ntbs(const char* src, ALLOC alloc = ALLOC::CONST);
	ntbs(char* src, size_t srclen, ALLOC alloc = ALLOC::NONE);
	virtual ~ntbs() {
		if (alloc == ALLOC::YES)
			delete[] bs;
	}

	void operator=(const char*);
	void operator=(char* src) { *this = (const char*) src; }
	// void operator=(const ntbs&);
	// void operator+=(char *src);
	// void operator+=(const ntbs&);
	int printf(const char* fmt, ...);
	// int appendf(const char* fmt, ...);

	char* get() const { return bs; }
	void terminate(size_t bw) { if (bw < max) bs[bw] = 0;}
	bool is_allocated() const { return alloc == ALLOC::YES ? true : false; }
	size_t size() const { return max-1; }
	size_t len() const { return std::strlen(bs); }
	void realloc(size_t req_bytes);

#ifdef C4S_DEBUGTRACE
	void dump(std::ostream&);
#endif

protected:

	char*	bs;
	size_t  max;
	ALLOC   alloc;
};

}