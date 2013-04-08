#pragma once

#include "Prerequisites.h"

#include <exception>

struct BadSocketException : std::exception
{
	virtual const char* what() const noexcept
	{
		return "BadSocketException: "
			"bad socketID provided";
	}
};

struct BadNodeException : std::exception
{
	virtual const char* what() const noexcept
	{
		return "BadNodeException: "
			"validation failed for given nodeID";
	}
};
