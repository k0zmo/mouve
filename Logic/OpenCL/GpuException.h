/*
 * Copyright (c) 2013-2014 Kajetan Swierk <k0zmo@outlook.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#pragma once

#include "../Prerequisites.h"
#include "Kommon/StringUtils.h"

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

        formatted = string_format("Building program failed: \n%s", logMessage.c_str());
    }
};
