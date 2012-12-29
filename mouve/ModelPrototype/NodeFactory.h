#pragma once

#include "Prerequisites.h"

class NodeFactory
{
public:
	virtual std::unique_ptr<NodeType> create() = 0;
};

template <class Type>
class DefaultNodeFactory : public NodeFactory
{
public:
	virtual std::unique_ptr<NodeType> create();
};

class AutoRegisterNodeBase : public NodeFactory
{
public:
	AutoRegisterNodeBase(const std::string& typeName)
		: typeName(typeName)
	{
		// Append at the beginning of the list
		// here the order doesn't really matter
		next = head;
		head = this;
	}

	std::string typeName;

	// Intrusive single-linked list
	AutoRegisterNodeBase* next;
	static AutoRegisterNodeBase* head;
};

template <class Type>
class AutoRegisterNode : public AutoRegisterNodeBase
{
public:
	AutoRegisterNode(const std::string& typeName)
		: AutoRegisterNodeBase(typeName)
	{}

	virtual std::unique_ptr<NodeType> create();
};

#define REGISTER_NODE(NodeTypeName, NodeClass) \
	AutoRegisterNode<NodeClass> __auto_registered_##NodeClass(NodeTypeName);

template <class Type>
inline std::unique_ptr<NodeType> DefaultNodeFactory<Type>::create()
{ return std::unique_ptr<Type>(new Type()); }

template <class Type>
inline std::unique_ptr<NodeType> AutoRegisterNode<Type>::create()
{ return std::unique_ptr<Type>(new Type()); }