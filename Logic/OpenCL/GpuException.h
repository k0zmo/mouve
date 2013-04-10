#pragma once

#if defined(HAVE_OPENCL)

#include "Prerequisites.h"

#include <exception>

struct GpuNodeException : public std::exception
{
	GpuNodeException(int error, const std::string& message)
		: error(error), message(message)
	{}

	virtual const char* what() const noexcept
	{
		return "GpuNodeException";
	}

	int error;
	std::string message;
};

struct GpuBuildException : public std::exception
{
	GpuBuildException(const std::string& log)
		: log(log)
	{}

	virtual const char* what() const noexcept
	{
		return "GpuBuildException";
	}

	std::string log;
};

#endif