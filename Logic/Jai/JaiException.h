#pragma once

#include "Prerequisites.h"

/// TODO: Querying error message
struct JaiException : public std::exception
{
	JaiException(int error, const char* message = nullptr)
		: error(error), message(message) {}

	virtual const char* what() throw()
	{
		return "JaiException";
	}

	int error;
	const char* message;
};
