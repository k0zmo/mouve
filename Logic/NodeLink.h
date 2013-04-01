#pragma once

#include "Prerequisites.h"

struct SocketAddress
{
	SocketAddress()
	{}

	SocketAddress(NodeID node, SocketID socket, bool isOutput)
		: node(node)
		, socket(socket)
		, isOutput(isOutput)
	{}
//
//	inline bool operator==(const SocketAddress& rhs) const
//	{
//		return node == rhs.node &&
//			   socket == rhs.socket &&
//			   isOutput == rhs.isOutput;
//	}
//
//	inline bool operator!=(const SocketAddress& rhs) const
//	{
//		return !operator==(rhs);
//	}
//

	inline bool isValid() const
	{ return node != InvalidNodeID && socket != InvalidSocketID; }

	NodeID node;
	SocketID socket;
	bool isOutput;
};

struct LOGIC_EXPORT NodeLink
{
	NodeLink() {}

	NodeLink(SocketAddress from, SocketAddress to);
	NodeLink(NodeID fromNode, SocketID fromSocket,
		NodeID toNode, SocketID toSocket);

	bool operator==(const NodeLink& rhs) const;
	bool operator<(const NodeLink& rhs) const;

	NodeID fromNode;
	SocketID fromSocket;
	NodeID toNode;
	SocketID toSocket;
};

class NodeLinkIterator
{
public:
	virtual ~NodeLinkIterator() {}
	virtual bool next(NodeLink& nodeLink) = 0;
};