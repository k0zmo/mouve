#pragma once

#include "Logic/Prerequisites.h"

// Forward declarations
class Controller;
class Property;
class PropertyManager;
class PropertyDelegate;
class PropertyModel;
class NodeEditorView;
class NodeConnectorView;
class NodeTemporaryLinkView;
class NodeSocketView;
class NodeLinkView;
class NodeView;

class PreviewWidget;

struct NodeDataIndex
{
	static const int NodeKey = 0;
	static const int SocketKey = 0;
};
