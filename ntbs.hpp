#pragma once

#include <cstring>

#define NTBS(V,L)\
	char V ## _ntbs[L];\
	ntbs V(V ## _ntbs, sizeof(V ## _ntbs), false)

namespace c4s {

class ntbs
{
public:
	ntbs(size_t _max = 0);
	ntbs(const char* src);
	ntbs(char* src, size_t srclen, bool allocated=false);
	virtual ~ntbs() {
		if (alloc)
			delete[] bs;
	}

	void operator=(const char*);
	void operator=(char* src) { *this = (const char*) src; }
	// void operator=(const ntbs&);
	// void operator+=(char *src);
	// void operator+=(const ntbs&);
	int sprintf(const char* fmt, ...);
	// int appendf(const char* fmt, ...);

	char* get() const { return bs; }
	bool is_allocated() const { return alloc; }
	size_t size() const { return max; }
	size_t len() const { return std::strlen(bs); }

protected:
	void realloc(size_t req_bytes);

	char*	bs;
	size_t  max;
	bool    alloc;

};

}