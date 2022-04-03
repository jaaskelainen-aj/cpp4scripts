#include <cstdarg>
#include <cstdio>
#include <stdexcept>

#include "ntbs.hpp"

namespace c4s {

ntbs::ntbs(size_t _max)
	: max(_max)
{
	if (max == 0) {
		bs = 0;
		alloc = ALLOC::NONE;
	}
	else {
		bs = new char[max]();
		alloc = ALLOC::YES;
	}
}

ntbs::ntbs(const char* source, ALLOC _alloc)
	: alloc(_alloc)
{
	max = strlen(source) + 1;
	if(alloc == ALLOC::YES) {
		bs = new char[max];
		strcpy(bs, source);
	} else {
		bs = (char *) source;
	}
}

ntbs::ntbs(char* source, size_t srclen, ALLOC _alloc)
	: alloc(_alloc)
{
	max = srclen+1;
	if(alloc == ALLOC::YES) {
		bs = new char[max];
		strcpy(bs, source);
		return;
	} else if (alloc == ALLOC::CONST) {
		// CONST type not allowed here
		alloc = ALLOC::NONE;
	}
	bs = source;
}

void
ntbs::realloc(size_t req_bytes)
{
	if (req_bytes < max)
		return;
	if (bs && alloc == ALLOC::YES)
		delete[] bs;
	max = req_bytes + 1;
	bs = new char[max]();
	alloc = ALLOC::YES;
}

void
ntbs::operator=(const char* source)
{
	size_t sl = strlen(source);
	if (alloc == ALLOC::CONST)
		max = 0; // Force allocation
	if (sl > max) 
		realloc (sl);
	strcpy(bs, source);
}

int
ntbs::printf(const char* fmt, ...)
{
	va_list vl;
	bool first = true;
	if (alloc == ALLOC::CONST)
		max = 0; // Force allocation
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

#ifdef C4S_DEBUGTRACE
void
ntbs::dump(std::ostream& os)
{
	os << "ntbs (";
	if (bs) os << strlen(bs);
	else os << "--";
	os << "/" << max << "/";
	os << (alloc == ALLOC::YES ? "Alloc" : (alloc == ALLOC::CONST ? "Const" : "None"));
	os << ") = " << bs << " ..\n";
}
#endif
} // namespace c4s