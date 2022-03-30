#include <cstdarg>
#include <cstdio>
#include <stdexcept>

#include "ntbs.hpp"

namespace c4s {

ntbs::ntbs(size_t _max)
	: max(_max), alloc(true)
{
	if (max == 0)
		bs = 0;
	else
		bs = new char[max]();	
}

ntbs::ntbs(const char* source)
	: alloc(true)
{
	max = strlen(source) + 1;
	bs = new char[max];
	strcpy(bs, source);
}

ntbs::ntbs(char *source, size_t _max, bool _a) 
	: max(_max), alloc(_a)
{
	if (alloc) {
		bs = new char[max]();
		strncpy(bs, source, max-1);
		bs[max-1] = 0;
	} else {
		bs = source;
	}
}

void
ntbs::realloc(size_t req_bytes)
{
	if (req_bytes < max)
		return;
	if (alloc && bs)
		delete[] bs;
	max = req_bytes + 1;
	bs = new char[max]();
	alloc = true;
}

void
ntbs::operator=(const char* source)
{
	size_t sl = strlen(source);
	if (sl > max) 
		realloc (sl);
	strcpy(bs, source);
}

int
ntbs::sprintf(const char* fmt, ...)
{
	va_list vl;
	bool first = true;
	retry:
	va_start(vl, fmt);
	int br = std::vsnprintf(bs, max, fmt, vl);
	va_end(vl);
	if (br < 0)
		throw std::runtime_error("ntbs::sprintf format error.");
	if ((size_t)br >= max) {
		if (first) {
			first = false;
			realloc(br);
			goto retry;
		} else
			throw std::runtime_error("ntbs::sprintf unable to alloc more memory.");
	}
	return br;
}

} // namespace c4s