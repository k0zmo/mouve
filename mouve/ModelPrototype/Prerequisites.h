#pragma once

// CRT
#include <cstdio>
#include <cstdio>
#include <cassert>
#include <cstdint>
#include <cstddef>

// STL
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <algorithm>

// Disable annoying warnings about using deprecated functions
#ifdef _MSV_VER
#  pragma warning(disable : 4996)
#endif

#define ASSERT(x) assert(x)

// Id of a socket - unique in each node
typedef std::uint8_t  SocketID;
// Id of a node - unique in each tree
typedef std::uint16_t NodeID;
// Id of a node type - just unique
typedef std::uint16_t NodeTypeID;

static const NodeID InvalidNodeID         = ~NodeID(0);
static const SocketID InvalidSocketID     = ~SocketID(0);
static const NodeTypeID InvalidNodeTypeID = NodeTypeID(0);

// Forward declarations
struct InputSocketConfig;
struct OutputSocketConfig;
struct NodeConfig;

class Node;
class NodeType;
class NodeSocket;
class NodeSocketReader;
class NodeSocketWriter;
class NodeTree;
class NodeSystem;

struct NodeLink;
struct SocketAddress;
class NodeIterator;
class NodeLinkIterator;
