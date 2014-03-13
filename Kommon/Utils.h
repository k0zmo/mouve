#pragma once

#include <cmath>

template<typename T>
bool fcmp(const T& a, const T& b)
{
	return std::fabs(a - b) < T(1e-8);
}

inline bool is64Bit()
{
#if K_ARCH == K_ARCH_64
	return true;
#else
	return false;
#endif
}