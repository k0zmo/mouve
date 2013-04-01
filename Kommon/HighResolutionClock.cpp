#include "HighResolutionClock.h"

#if K_SYSTEM == K_SYSTEM_WINDOWS
#  include <Windows.h>
#  undef max
#  undef min

HighResolutionClock::HighResolutionClock()
{
	LARGE_INTEGER freq;
	SetThreadAffinityMask(GetCurrentThread(), 1);
	QueryPerformanceFrequency(&freq);
	_periodTime = 1.0 / static_cast<double>(freq.QuadPart);
}

double HighResolutionClock::currentTimeInSeconds()
{
	LARGE_INTEGER time;
	QueryPerformanceCounter(&time);
	return static_cast<double>(time.QuadPart) * _periodTime;
}

#elif K_SYSTEM == K_SYSTEM_LINUX

HighResolutionClock::HighResolutionClock()
{
	gettimeofday(&_startTime, nullptr);
}

double HighResolutionClock::currentTimeInSeconds()
{
	timeval current;
	gettimeofday(&current, nullptr);
	return current.tv_sec - _startTime.tv_sec + 
		0.000001 * (current.tv_usec - _startTime.tv_usec);
}
#endif