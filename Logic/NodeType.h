/*
 * Copyright (c) 2013-2014 Kajetan Swierk <k0zmo@outlook.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#pragma once

#include "Prerequisites.h"
#include "NodeFlowData.h"
#include "NodeProperty.h"
#include "Kommon/EnumFlags.h"

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
enum class ENodeConfig
{
    NoFlags                  = 0,
    /// Node markes as a stateful
    HasState                 = K_BIT(0),
    /// After one exec-run, node should be tagged for next one
    AutoTag                  = K_BIT(1),
    /// Don't automatically do time computations for this node as it'll do it itself
    OverridesTimeComputation = K_BIT(2)
};
typedef EnumFlags<ENodeConfig> NodeConfigFlags;
K_DEFINE_ENUMFLAGS_OPERATORS(NodeConfigFlags)

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
    NodeConfigFlags flags;

    NodeConfig()
        : pInputSockets(nullptr)
        , pOutputSockets(nullptr)
        , pProperties(nullptr)
        , description()
        , module()
        , flags(ENodeConfig::NoFlags)
    {
    }
};

// These allow for iterating over NodeConfig fields: input/output socket config
// and property config
template <class Iter>
const Iter* begin_config(const NodeConfig&)
{
    return nullptr;
}

template <class Iter>
bool end_config(const Iter*)
{
    return true;
}

template <class Iter>
int pos_config(const Iter*, const NodeConfig&)
{
    return 0;
}

template <>
inline const InputSocketConfig* begin_config(const NodeConfig& nodeConfig)
{
    return nodeConfig.pInputSockets;
}

template <>
inline bool end_config(const InputSocketConfig* iter)
{
    return !iter || iter->dataType == ENodeFlowDataType::Invalid;
}

template <>
inline int pos_config(const InputSocketConfig* iter, const NodeConfig& nodeConfig)
{
    return iter - nodeConfig.pInputSockets;
}

template <>
inline const OutputSocketConfig* begin_config(const NodeConfig& nodeConfig)
{
    return nodeConfig.pOutputSockets;
}

template <>
inline bool end_config(const OutputSocketConfig* iter)
{
    return !iter || iter->dataType == ENodeFlowDataType::Invalid;
}

template <>
inline int pos_config(const OutputSocketConfig* iter, const NodeConfig& nodeConfig)
{
    return iter - nodeConfig.pOutputSockets;
}

template <>
inline const PropertyConfig* begin_config(const NodeConfig& nodeConfig)
{
    return nodeConfig.pProperties;
}

template <>
inline bool end_config(const PropertyConfig* iter)
{
    return !iter || iter->type == EPropertyType::Unknown;
}

template <>
inline int pos_config(const PropertyConfig* iter, const NodeConfig& nodeConfig)
{
    return iter - nodeConfig.pProperties;
}

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
