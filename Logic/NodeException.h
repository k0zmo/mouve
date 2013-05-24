#pragma once

#include "Prerequisites.h"

struct BadSocketException : std::exception
{
	virtual const char* what() const throw()
	{
		return "BadSocketException: "
			"bad socketID provided";
	}
};

struct BadNodeException : std::exception
{
	virtual const char* what() const throw()
	{
		return "BadNodeException: "
			"validation failed for given nodeID";
	}
};

struct ExecutionError : std::exception
{
	ExecutionError(const std::string& nodeName, 
		const std::string& nodeTypeName, 
		const std::string& errorMessage)
		: nodeName(nodeName)
		, nodeTypeName(nodeTypeName)
		, errorMessage(errorMessage)
	{
	}

	virtual const char* what() const throw()
	{
		return "ExecutionError: "
			"bad thing happened during one of node execution";
	}

	std::string nodeName;
	std::string nodeTypeName;
	std::string errorMessage;
};
