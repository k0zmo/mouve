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

#include "NodeLink.h"

NodeLink::NodeLink(SocketAddress from, SocketAddress to)
    : fromNode(from.node)
    , fromSocket(from.socket)
    , toNode(to.node)
    , toSocket(to.socket)
{
}

NodeLink::NodeLink(NodeID fromNode, SocketID fromSocket,
    NodeID toNode, SocketID toSocket)
    : fromNode(fromNode)
    , fromSocket(fromSocket)
    , toNode(toNode)
    , toSocket(toSocket)
{
}

bool NodeLink::operator==(const NodeLink& rhs) const
{
    return fromNode   == rhs.fromNode &&
           fromSocket == rhs.fromSocket &&
           toNode     == rhs.toNode &&
           toSocket   == rhs.toSocket;
}

bool NodeLink::operator<(const NodeLink& rhs) const
{
    if(fromNode < rhs.fromNode)
        return true;
    else if(fromNode > rhs.fromNode)
        return false;
    else if(fromSocket < rhs.fromSocket)
        return true;
    else if(fromSocket > rhs.fromSocket)
        return false;
    else if(toNode < rhs.toNode)
        return true;
    else if(toNode > rhs.toNode)
        return false;
    else if(toSocket < rhs.toSocket)
        return true;
    else
        return false;
}
