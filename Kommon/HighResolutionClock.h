#pragma once

#include "konfig.h"

#if K_SYSTEM == K_SYSTEM_LINUX
#  include <sys/time.h>
#endif

class HighResolutionClock
{
public:
	HighResolutionClock();
	double currentTimeInSeconds();

private:
#if K_SYSTEM == K_SYSTEM_WINDOWS
	double _periodTime;
#else
	timeval _startTime;
#endif
};
