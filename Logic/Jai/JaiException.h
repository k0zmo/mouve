#pragma once

#if defined(HAVE_JAI)

#include "Prerequisites.h"

/// TODO: Querying error message
struct JaiException : public std::exception
{
	JaiException(int error, const char* message = nullptr)
		: error(error), message(message) {}

	virtual const char* what() noexcept
	{
		return "JaiException";
	}

	int error;
	const char* message;
};

#endif