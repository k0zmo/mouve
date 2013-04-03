#pragma once

#include "Prerequisites.h"

#include <exception>

struct node_bad_socket : std::exception
{
	virtual const char* what() const noexcept
	{
		return "node_bad_socket: "
			"bad socketID provided";
	}
};

struct node_bad_node : std::exception
{
	virtual const char* what() const noexcept
	{
		return "node_bad_node: "
			"validation failed for given nodeID";
	}
};