#pragma once

#include "../Prerequisites.h"

#include <exception>
#include <sstream>

struct GpuNodeException : public std::exception
{
	GpuNodeException(int error, const std::string& message)
		: error(error), message(message)
	{
		formatMessage();
	}

	virtual const char* what() const throw()
	{
		return formatted.c_str();
	}

	int error;
	std::string message;
	std::string formatted;

private:
	void formatMessage()
	{
		std::ostringstream strm;
		strm << "OpenCL error (" << error << "): " << message;
		formatted = strm.str();
	}
};

struct GpuBuildException : public std::exception
{
	GpuBuildException(const std::string& log)
		: log(log)
	{
		formatMessage();
	}

	virtual const char* what() const throw()
	{
		return formatted.c_str();
	}

	std::string log;
	std::string formatted;

private:
	void formatMessage()
	{
		std::string logMessage = log;
		if(logMessage.size() > 1024)
		{
			logMessage.erase(1024, std::string::npos);
			logMessage.append("\n...");
		}

		std::ostringstream strm;
		strm << "Building program failed: \n" << logMessage;
		formatted = strm.str();
	}
};
