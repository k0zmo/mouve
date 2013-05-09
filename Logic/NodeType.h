#pragma once

#include "Prerequisites.h"
#include "NodeFlowData.h"

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

	const NodeFlowData& readSocket(SocketID socketID) const;
	const cv::Mat& readSocketImage(SocketID socketID) const;

private:
	void setNode(NodeID nodeID, SocketID numInputSockets);

private:
	NodeTree* _nodeTree;
	SocketID _numInputSockets;
	NodeID _nodeID;
};

class LOGIC_EXPORT NodeSocketWriter
{
	K_DISABLE_COPY(NodeSocketWriter);

	friend class Node;
public:
	explicit NodeSocketWriter()
		: _outputs(nullptr)
	{
	}

	void writeSocket(SocketID socketID, NodeFlowData&& image);
	NodeFlowData& acquireSocket(SocketID socketID);

private:
	void setOutputSockets(std::vector<NodeFlowData>& outputs);

private:
	std::vector<NodeFlowData>* _outputs;
};

struct Matrix3x3
{
	double v[9];

	Matrix3x3()
	{
		for(int i = 0; i < 9; ++i)
			v[i] = 0.0;
	}

	Matrix3x3(double center)
	{
		for(int i = 0; i < 9; ++i)
			v[i] = 0.0;
		v[4] = center;
	}

	Matrix3x3(double m11, double m12, double m13,
		double m21, double m22, double m23,
		double m31, double m32, double m33)
	{
		v[0] = m11; v[1] = m12; v[2] = m13;
		v[3] = m21; v[4] = m22; v[5] = m23;
		v[6] = m31; v[7] = m32; v[8] = m33;
	}
};

enum class EPropertyType : int
{
	Unknown,
	Boolean,
	Integer,
	Double,
	Enum,
	Matrix,
	Filepath,
	String
}; 

Q_DECLARE_METATYPE(Matrix3x3)

struct InputSocketConfig
{
	ENodeFlowDataType dataType;
	std::string name;
	std::string humanName;
	std::string description;
};

struct OutputSocketConfig
{
	ENodeFlowDataType dataType;
	std::string name;
	std::string humanName;
	std::string description;
};

struct PropertyConfig
{
	EPropertyType type;
	std::string name; // humanName
	std::string uiHint;
};

enum ENodeConfigurationFlags
{
	Node_NoFlags                  = 0, // default 
	Node_HasState                 = K_BIT(0),
	Node_AutoTag                  = K_BIT(1),
	Node_OverridesTimeComputation = K_BIT(2)
};
Q_DECLARE_FLAGS(NodeConfigurationFlags, ENodeConfigurationFlags);
Q_DECLARE_OPERATORS_FOR_FLAGS(NodeConfigurationFlags)

struct NodeConfig
{
	const InputSocketConfig* pInputSockets;
	const OutputSocketConfig* pOutputSockets;
	const PropertyConfig* pProperties;
	std::string description;
	std::string module;
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

enum class EStatus : int
{
	Ok,
	Error,
	Tag
};

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

	double timeElapsed;
	EStatus status;
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
	virtual bool setProperty(PropertyID propId, const QVariant& newValue);
	virtual QVariant property(PropertyID propId) const;
	virtual bool restart();
	virtual void finish();

	virtual bool init(const std::shared_ptr<NodeModule>& module);
};

inline bool NodeType::setProperty(PropertyID, const QVariant&)
{ return false; }
inline QVariant NodeType::property(PropertyID) const
{ return QVariant(); }
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
