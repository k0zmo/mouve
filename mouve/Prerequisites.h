#pragma once

#include <QString>
#include <QList>
#include <QHash>
#include <QDebug>

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

typedef quint32 NodeID;
typedef quint32 NodeTypeID;
typedef quint32 SocketID;
