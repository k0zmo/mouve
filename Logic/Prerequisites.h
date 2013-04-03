#pragma once

#include "Kommon/konfig.h"

#if defined(LOGIC_LIB)
#  define LOGIC_EXPORT K_DECL_EXPORT
#else
#  define LOGIC_EXPORT K_DECL_IMPORT
#endif

#define BIT(x) 1 << x

#if K_COMPILER == K_COMPILER_MSVC
#  define noexcept throw()
#endif

// CRT
#include <cstdio>
#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <stdexcept>

// STL
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <functional>
#include <algorithm>

// Property system
#include <QVariant>

// Id of a socket - unique in each node
typedef std::uint8_t SocketID;
// Id of a node - unique in each tree
typedef std::uint16_t NodeID;
// Id of a node type - just unique
typedef std::uint16_t NodeTypeID;
// Id of a property - unique in each node
typedef std::int8_t PropertyID;

static const NodeID InvalidNodeID         = ~NodeID(0);
static const SocketID InvalidSocketID     = ~SocketID(0);
static const PropertyID InvalidPropertyID = ~PropertyID(0);
static const NodeTypeID InvalidNodeTypeID = NodeTypeID(0);

// Forward declarations
struct InputSocketConfig;
struct OutputSocketConfig;
struct PropertyConfig;
struct NodeConfig;
struct ExecutionStatus;

class Node;
class NodeType;
class NodeSocket;
class NodeFlowData;
class NodeSocketReader;
class NodeSocketWriter;
class NodeTree;
class NodeFactory;
class NodeSystem;
class NodeModule;

struct NodeLink;
struct SocketAddress;

class NodeIterator;
class NodeLinkIterator;
class NodeTypeIterator;

enum class ENodeFlags : int;
enum class EPropertyType : int;
enum class EStatus : int;
enum class ENodeFlowDataType : int;
