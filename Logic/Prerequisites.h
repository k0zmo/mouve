#pragma once

#include <QtCore/qglobal.h>

#ifdef MOUVE_LOGIC_LIB
# define MOUVE_LOGIC_EXPORT Q_DECL_EXPORT
#else
# define MOUVE_LOGIC_EXPORT Q_DECL_IMPORT
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

// QTL
#include <QString>
#include <QList>
#include <QHash>
#include <QDebug>

// Disable annoying warnings about using deprecated functions
#if defined(Q_CC_MSVC)
#  pragma warning(disable : 4996)
#endif

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
class NodeFactory;
class NodeSystem;

struct NodeLink;
struct SocketAddress;

class NodeIterator;
class NodeLinkIterator;
class NodeTypeIterator;

/// xXx: Temporary here
namespace cv { class Mat; }
