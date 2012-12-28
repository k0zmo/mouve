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

//bool NodeLink::operator==(const NodeLink& rhs) const
//{
//	return fromNode   == rhs.fromNode &&
//		   fromSocket == rhs.fromSocket &&
//		   toNode     == rhs.toNode &&
//		   toSocket   == rhs.toSocket;
//}

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
