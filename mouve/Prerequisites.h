#pragma once

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

class Controller;

// Forward declarations
//struct InputSocketConfig;
//struct OutputSocketConfig;
//struct NodeConfig;

// Model part
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

// View part
class NodeEditorView;
class NodeConnectorView;
class NodeTemporaryLinkView;
class NodeSocketView;
class NodeLinkView;
class NodeScene;
class NodeView;

struct NodeDataIndex
{
	static const int NodeKey = 0;
	static const int SocketKey = 0;
};
