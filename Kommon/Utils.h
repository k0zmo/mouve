#pragma once

#include <cmath>

template<typename T>
bool fcmp(const T& a, const T& b)
{
	return std::fabs(a - b) < T(1e-8);
}
