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

#include "mouve_export.h"

#define LOGIC_VERSION 1

// CRT
#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <cstdarg>
#include <exception>

// STL
#include <string>
#include <vector>
#include <unordered_map>
#include <map>
#include <memory>
#include <functional>
#include <algorithm>

// Id of a node - unique in each tree
typedef std::uint16_t NodeID;
// Id of a socket - unique in each node
typedef std::uint8_t SocketID;
// Id of a property - unique in each node
typedef std::int8_t PropertyID;
// Id of a node type - just unique
typedef std::uint16_t NodeTypeID;

static const NodeID InvalidNodeID{0xFFFFU};
static const SocketID InvalidSocketID{0xFFU};
static const PropertyID InvalidPropertyID{-1};
static const NodeTypeID InvalidNodeTypeID{0};

// Forward declarations
class SocketConfig;
class PropertyConfig;
class NodeConfig;
struct PropertyValidator;
struct PropertyObserver;
struct ExecutionStatus;

class Node;
class NodeType;
class NodeSocket;
class NodeFlowData;
class NodeProperty;
class NodeSocketTracer;
class NodeSocketReader;
class NodeSocketWriter;
class NodeTree;
class NodeFactory;
class NodeSystem;
class NodeModule;
class NodePlugin;
class NodeTreeSerializer;
class NodeResolver;

struct NodeLink;
struct SocketAddress;

class NodeIterator;
class NodeLinkIterator;
class NodeTypeIterator;

enum class ENodeFlags : int;
enum class EPropertyType : int;
enum class EStatus : int;
enum class ENodeFlowDataType : int;
