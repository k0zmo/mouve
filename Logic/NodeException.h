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

struct BadConfigException : std::exception
{
    virtual const char* what() const throw()
    {
        return "BadConfigException: "
            "bad configuration value";
    }
};

struct BadConnectionException : std::exception
{
    BadConnectionException()
        : node(InvalidNodeID)
        , socket(InvalidSocketID)
    {
    }

    BadConnectionException(NodeID node, SocketID socket)
        : node(node)
        , socket(socket)
    {
    }

    virtual const char* what() const throw()
    {
        return "BadConnectionException: "
            "invalid connection between two sockets";
    }

    NodeID node;
    SocketID socket;
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
