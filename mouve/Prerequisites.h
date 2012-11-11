#pragma once

#include <string>
#include <vector>
#include <unordered_map>

using std::string;
using std::vector;
using std::unordered_map;

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

typedef quint32 NodeId;
typedef quint32 NodeTypeId;
typedef quint32 SocketId;
