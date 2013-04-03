#pragma once

#include "Prerequisites.h"

#include <exception>

struct gpu_node_exception : public std::exception
{
	gpu_node_exception(int error, const std::string& message)
		: error(error), message(message)
	{}

	virtual const char* what() const noexcept
	{
		return "gpu_node_exception";
	}

	int error;
	std::string message;
};

struct gpu_build_exception : public std::exception
{
	gpu_build_exception(const std::string& log)
		: log(log)
	{}

	virtual const char* what() const noexcept
	{
		return "gpu_build_exception";
	}

	std::string log;
};