#include "StringUtils.h"

#include <cstdarg>
#include <memory>
#include <cstring>

std::string string_format(std::string fmt, ...)
{
	int n = fmt.size() * 2;
	std::unique_ptr<char[]> formatted;
	va_list ap;

	while(1) {
		formatted.reset(new char[n]);
#if K_COMPILER == K_COMPILER_MSVC
#  pragma warning(push)
#  pragma warning(disable : 4996)
#endif
		strncpy(formatted.get(), fmt.c_str(), fmt.size());
		va_start(ap, fmt);
		int final_n = vsnprintf(formatted.get(), n, fmt.c_str(), ap);
		va_end(ap);
#if K_COMPILER == K_COMPILER_MSVC
#  pragma warning(pop)
#endif
		// string truncated
		if(final_n < 0 || final_n >= n)
			n += std::abs(final_n - n + 1);
		else
			break;
	}

	return std::string(formatted.get());
}
