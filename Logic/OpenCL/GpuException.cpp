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

#include "GpuException.h"

#include <fmt/core.h>

namespace {

std::string formatMessage(const std::string& log)
{
    std::string logMessage = log;
    if (logMessage.size() > 1024)
    {
        logMessage.erase(1024, std::string::npos);
        logMessage.append("\n...");
    }

    return fmt::format("Building program failed: \n{}", logMessage);
}
} // namespace

GpuNodeException::GpuNodeException(int error, const std::string& message)
    : error(error),
      message(message),
      formatted(fmt::format("OpenCL error ({}): {}", error, message))
{
}

const char* GpuNodeException::what() const throw()
{
    return formatted.c_str();
}

GpuBuildException::GpuBuildException(const std::string& log)
    : log(log), formatted(formatMessage(log))
{
}

const char* GpuBuildException::what() const throw()
{
    return formatted.c_str();
}