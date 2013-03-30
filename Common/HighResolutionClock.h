#pragma once

#if !defined(_WIN32)
#  include <sys/time.h>
#endif

class HighResolutionClock
{
public:
	HighResolutionClock();
	double currentTimeInSeconds();

private:
#if defined(_WIN32)
	double _periodTime;
#else
	timeval _startTime;
#endif
};
