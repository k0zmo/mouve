#pragma once

#include "Prerequisites.h"
#include "NodeFlowData.h"
#include "NodeProperty.h"

/// Class responsible for reading data from a node socket
class LOGIC_EXPORT NodeSocketReader
{
	K_DISABLE_COPY(NodeSocketReader);

	friend class NodeTree;
public:
	explicit NodeSocketReader(NodeTree* tree)
		: _nodeTree(tree)
		, _numInputSockets(0)
		, _nodeID(InvalidNodeID)
	{
	}

	/// Reads data from the socket of given ID
	const NodeFlowData& readSocket(SocketID socketID) const;
	/// Reads data from the socket as an opencv image 
	const cv::Mat& readSocketImage(SocketID socketID) const;

private:
	void setNode(NodeID nodeID, SocketID numInputSockets);

private:
	NodeTree* _nodeTree;
	SocketID _numInputSockets;
	NodeID _nodeID;
};

/// Class responsible for writing data to a node socket
class LOGIC_EXPORT NodeSocketWriter
{
	K_DISABLE_COPY(NodeSocketWriter);

	friend class Node;
public:
	explicit NodeSocketWriter()
		: _outputs(nullptr)
	{
	}

	/// Writes data to the socket of given ID
	void writeSocket(SocketID socketID, NodeFlowData&& image);
	/// Returns a reference to underlying socket data
	NodeFlowData& acquireSocket(SocketID socketID);

private:
	void setOutputSockets(std::vector<NodeFlowData>& outputs);

private:
	std::vector<NodeFlowData>* _outputs;
};

/// Describes input socket parameters
struct InputSocketConfig
{
	/// Type of underlying data
	ENodeFlowDataType dataType;
	/// Name of input socket
	std::string name;
	/// Human-readable name of socket
	std::string humanName;
	/// Optional description
	std::string description;
};

/// Describes output socket parameters
struct OutputSocketConfig
{
	/// Type of underlying data
	ENodeFlowDataType dataType;
	/// Name of output socket
	std::string name;
	/// Human-readable name of socket
	std::string humanName;
	/// Optional description
	std::string description;
};

/// Describes node property parameters
struct PropertyConfig
{
	/// Type of property data
	EPropertyType type;
	/// Human-readable property name
	std::string name;
	/// Optional parameters for UI engine
	std::string uiHint;
};

/// Additional, per-node settings
enum ENodeConfigurationFlags
{
	Node_NoFlags                  = 0,
	/// Node markes as a stateful
	Node_HasState                 = K_BIT(0),
	/// After one exec-run, node should be tagged for next one
	Node_AutoTag                  = K_BIT(1),
	/// Don't automatically do time computations for this node as it'll do it itself
	Node_OverridesTimeComputation = K_BIT(2)
};
typedef int NodeConfigurationFlags;

/// Describes node configuration
struct NodeConfig
{
	/// Array of input socket descriptors
	const InputSocketConfig* pInputSockets;
	/// Array of output socket descriptors
	const OutputSocketConfig* pOutputSockets;
	/// Array of node property descriptors
	const PropertyConfig* pProperties;
	/// Optional human-readable description
	std::string description;
	/// Optional name of module that this node belongs to
	std::string module;
	/// Additional settings
	NodeConfigurationFlags flags;

	NodeConfig()
		: pInputSockets(nullptr)
		, pOutputSockets(nullptr)
		, pProperties(nullptr)
		, description()
		, module()
		, flags(Node_NoFlags)
	{
	}
};

/// Node execution status 
enum class EStatus : int
{
	/// Everything was ok
	Ok,
	/// There was an error during execution
	Error,
	/// Mark this node for execution for next run
	/// (requires Node_AutoTag flag set on)
	Tag
};

/// Represents execution return information
struct ExecutionStatus
{
	ExecutionStatus()
		: timeElapsed(0)
		, status(EStatus::Ok)
		, message()
	{
	}

	ExecutionStatus(EStatus status, 
		const std::string& message = std::string())
		: timeElapsed(0)
		, status(status)
		, message(message)
	{
	}

	ExecutionStatus(EStatus status,
		double timeElapsed,
		const std::string& message = std::string())
		: timeElapsed(timeElapsed)
		, status(status)
		, message(message)
	{
	}

	/// If Node_OverridesTimeComputation is set on this value will be used
	/// to display overriden time elapsed value
	double timeElapsed;
	/// Node execution status
	EStatus status;
	/// Additional message in case of an error
	std::string message;
};

class NodeType
{
public:
	virtual ~NodeType() {}

	// Required methods
	virtual void configuration(NodeConfig& nodeConfig) const = 0;
	virtual ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) = 0;

	// Optional methods
	virtual bool setProperty(PropertyID propId, const NodeProperty& newValue);
	virtual NodeProperty property(PropertyID propId) const;
	virtual bool restart();
	virtual void finish();

	virtual bool init(const std::shared_ptr<NodeModule>& module);

	LOGIC_EXPORT static std::string formatMessage(const char* msg, ...);
};

inline bool NodeType::setProperty(PropertyID, const NodeProperty&)
{ return false; }
inline NodeProperty NodeType::property(PropertyID) const
{ return NodeProperty(); }
inline bool NodeType::restart()
{ return false; }
inline void NodeType::finish()
{ return; }
inline bool NodeType::init(const std::shared_ptr<NodeModule>&)
{ return false; }

class NodeTypeIterator
{
public:
	struct NodeTypeInfo
	{
		NodeTypeID typeID;
		std::string typeName;
	};

	virtual ~NodeTypeIterator() {} 
	virtual bool next(NodeTypeInfo& nodeTypeInfo) = 0;
};
