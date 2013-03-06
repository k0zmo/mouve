#pragma once

#include "Prerequisites.h"

class MOUVE_LOGIC_EXPORT NodeSocketReader
{
	Q_DISABLE_COPY(NodeSocketReader);
public:
	NodeSocketReader(NodeTree* tree)
		: _nodeTree(tree)
		, _numInputSockets(0)
		, _nodeID(InvalidNodeID)
	{}

	/// xXx: make it private? and friend with nodetree/node?
	void setNode(NodeID nodeID, SocketID numInputSockets);

	const cv::Mat& readSocket(SocketID socketID) const;
	bool allInputSocketsConnected() const;

private:
	NodeTree* _nodeTree;
	SocketID _numInputSockets;
	NodeID _nodeID;
};

class MOUVE_LOGIC_EXPORT NodeSocketWriter
{
	Q_DISABLE_COPY(NodeSocketWriter);
public:
	NodeSocketWriter()
		: _outputs(nullptr)
	{}

	/// xXx: make it private? and friend with nodetree/node?
	void setOutputSockets(std::vector<cv::Mat>& outputs);

	void writeSocket(SocketID socketID, cv::Mat& image);
	cv::Mat& lockSocket(SocketID socketID);

private:
	std::vector<cv::Mat>* _outputs;
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

	// TODO: Do I need copy constructor ??
};

enum class EPropertyType
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
	std::string name;
	std::string humanName;
	std::string description;
	//NodeSocketData defaultData;
};

struct OutputSocketConfig
{
	std::string name;
	std::string humanName;
	std::string description;
	//int type;
};

struct PropertyConfig
{
	EPropertyType type;
	std::string name; // humanName
	std::string uiHint;
};

enum ENodeFlags
{
	Node_Stateless   = 0, // default 
	Node_State       = 1 << 0,
};
Q_DECLARE_FLAGS(NodeFlags, ENodeFlags);
Q_DECLARE_OPERATORS_FOR_FLAGS(NodeFlags)

struct NodeConfig
{
	const InputSocketConfig* pInputSockets;
	const OutputSocketConfig* pOutputSockets;
	const PropertyConfig* pProperties;
	std::string description;
	NodeFlags flags;

	NodeConfig()
		: pInputSockets(nullptr)
		, pOutputSockets(nullptr)
		, pProperties(nullptr)
		, description()
		, flags(Node_Stateless)
	{
	}
};

enum class EStatus
{
	Ok,
	Error,
	Tag
};

struct ExecutionStatus
{
	ExecutionStatus()
		: status(EStatus::Ok)
		, errorMessage()
	{
	}

	ExecutionStatus(EStatus status, 
		const std::string& errorMessage = std::string())
		: status(status)
		, errorMessage(errorMessage)
	{
	}

	EStatus status;
	std::string errorMessage;
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
	virtual bool initialize();
};

inline bool NodeType::setProperty(PropertyID, const QVariant&)
{ return false; }
inline QVariant NodeType::property(PropertyID) const
{ return QVariant(); }
inline bool NodeType::initialize()
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
