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
#include "NodeException.h"
#include "Kommon/EnumFlags.h"

// Class responsible for reading data from a node socket
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

    // Reads data from the socket of given ID
    const NodeFlowData& operator()(SocketID socketID) const
    {
        return readSocket(socketID);
    }

    // Reads data from the socket of given ID
    const NodeFlowData& readSocket(SocketID socketID) const;

private:
    void setNode(NodeID nodeID, SocketID numInputSockets);

private:
    NodeTree* _nodeTree;
    SocketID _numInputSockets;
    NodeID _nodeID;
};

// Class responsible for writing data to a node socket
class LOGIC_EXPORT NodeSocketWriter
{
    K_DISABLE_COPY(NodeSocketWriter);

    friend class Node;
public:
    explicit NodeSocketWriter()
        : _outputs(nullptr)
    {
    }

    // Returns a reference to underlying socket data
    NodeFlowData& operator()(SocketID socketID)
    {
        return acquireSocket(socketID);
    }

    // Returns a reference to underlying socket data
    NodeFlowData& acquireSocket(SocketID socketID);

private:
    void setOutputSockets(std::vector<NodeFlowData>& outputs);

private:
    std::vector<NodeFlowData>* _outputs;
};

// Interface for property value validator
struct PropertyValidator
{
    virtual bool validate(const NodeProperty& value) = 0;
};

// Helper method to construct property validator
template <class T, class... Args, 
    class = typename std::enable_if<std::is_base_of<PropertyValidator, T>::value>::type>
std::unique_ptr<PropertyValidator> make_validator(Args&&... args)
{
    return std::unique_ptr<PropertyValidator>(new T(std::forward<Args>(args)...));
}

// Interface for property value observer
struct PropertyObserver
{
    virtual void notifyChanged(const NodeProperty& value) = 0;
};

// Helper method to construct property observer
template <class T, class... Args,
    class = typename std::enable_if<std::is_base_of<PropertyObserver, T>::value>::type>
std::unique_ptr<PropertyObserver> make_observer(Args&&... args)
{
    return std::unique_ptr<PropertyObserver>(new T(std::forward<Args>(args)...));
}

// Property observer invoking given callable object on value changed
class FuncObserver : public PropertyObserver
{
public:
    using func = std::function<void(const NodeProperty&)>;

    explicit FuncObserver(func op) : _op(std::move(op)) {}

    virtual void notifyChanged(const NodeProperty& value) override
    {
        _op(value);
    }

private:
    func _op;
};

// Property validator using given callable object for validation
class FuncValidator : public PropertyValidator
{
public:
    using func = std::function<bool(const NodeProperty&)>;

    explicit FuncValidator(func op) : _op(std::move(op)) {}

    virtual bool validate(const NodeProperty& value) override
    {
        return _op(value);
    }

private:
    func _op;
};

// Validate property value checking if new value is in given range
template <class T, class MinOp, class MaxOp>
class RangePropertyValidator : public PropertyValidator
{
public:
    typedef T type;

    explicit RangePropertyValidator(T min, T max = 0)
        : min(min), max(max)
    {
        if(propertyType<T>() == EPropertyType::Unknown)
            throw BadConfigException();
    }

    virtual bool validate(const NodeProperty& value) override
    {
        return validateImpl(value.cast_value<T>());
    }

private:
    bool validateImpl(const T& value)
    {
        return MinOp()(value, min) && MaxOp()(value, max);
    }

private:
    type min;
    type max;
};

template <class T>
struct no_limit : public std::binary_function<T, T, bool>
{
    bool operator()(const T&, const T&) const { return true; }
};

// Range validator for (a, b)
template <class T>
using ExclRangePropertyValidator
    = RangePropertyValidator<T, std::greater<T>, std::less<T>>;

// Range validator for [a, b]
template <class T>
using InclRangePropertyValidator
    = RangePropertyValidator<T, std::greater_equal<T>, std::less_equal<T>>;

// Range validator for [a, inf+)
template <class T>
using MinPropertyValidator
    = RangePropertyValidator<T, std::greater_equal<T>, no_limit<T>>;

// Range validator for (a, inf+)
template <class T>
using GreaterPropertyValidator
    = RangePropertyValidator<T, std::greater<T>, no_limit<T>>;

// Range validator for (-inf, b]
template <class T>
using MaxPropertyValidator
    = RangePropertyValidator<T, no_limit<T>, std::less_equal<T>>;

// Range validator for (-inf, b)
template <class T>
using LessPropertyValidator
    = RangePropertyValidator<T, no_limit<T>, std::less<T>>;

// Describes input/output socket parameters
class LOGIC_EXPORT SocketConfig
{
public:
    explicit SocketConfig(SocketID socketID,
        std::string name, ENodeFlowDataType type)
        : _socketID(socketID)
        , _type(type)
        , _name(std::move(name))
    {
    }

    SocketConfig(const SocketConfig&) = default;
    SocketConfig& operator=(const SocketConfig&) = delete;

    SocketConfig(SocketConfig&& other)
        : _socketID(other._socketID)
        , _type(other._type)
        , _name(std::move(other._name))
        , _description(std::move(other._description))
    {
    }

    SocketConfig& operator=(SocketConfig&& other) = delete;

    SocketConfig& setDescription(std::string description)
    {
        _description = std::move(description);
        return *this;
    }

    SocketID socketID() const { return _socketID; }
    ENodeFlowDataType type() const { return _type; }
    const std::string& name() const { return _name; }
    const std::string& description() const { return _description; }
    bool isValid() const { return _socketID != InvalidSocketID; }

private:
    // Socket sequential number for given node type
    const SocketID _socketID;
    // Type of underlying data
    const ENodeFlowDataType _type;
    // Name of input socket
    const std::string _name;
    // Optional description
    std::string _description;
};

// Describes node property parameters
class LOGIC_EXPORT PropertyConfig
{
public:
    explicit PropertyConfig(PropertyID propertyID, 
        std::string name, NodeProperty& nodeProperty)
        : _propertyID(propertyID)
        , _nodeProperty(nodeProperty)
        , _type(nodeProperty.type())
        , _name(std::move(name))
    {
    }

    PropertyConfig(const PropertyConfig&) = delete;
    PropertyConfig& operator=(const PropertyConfig&) = delete;

    PropertyConfig(PropertyConfig&& other)
        : _propertyID(other._propertyID)
        , _nodeProperty(other._nodeProperty)
        , _type(other._type)
        , _name(std::move(other._name))
        , _uiHints(std::move(other._uiHints))
        , _description(std::move(other._description))
        , _validator(std::move(other._validator))
        , _observer(std::move(other._observer))
    {
    }

    PropertyConfig& operator=(PropertyConfig&& other) = delete;

    PropertyConfig& setUiHints(std::string uiHints)
    {
        _uiHints = std::move(uiHints);
        return *this;
    }

    PropertyConfig& setDescription(std::string description)
    {
        _description = std::move(description);
        return *this;
    }

    PropertyConfig& setValidator(std::unique_ptr<PropertyValidator> validator)
    {
        _validator = std::move(validator);
        return *this;
    }

    PropertyConfig& setObserver(std::unique_ptr<PropertyObserver> observer)
    {
        _observer = std::move(observer);
        return *this;
    }

    PropertyID propertyID() const { return _propertyID; }
    EPropertyType type() const { return _type; }
    const std::string& name() const { return _name; }
    const std::string& uiHints() const { return _uiHints; }
    const std::string& description() const { return _description; }
    bool isValid() const { return _propertyID != InvalidPropertyID; }

    bool setPropertyValue(const NodeProperty& newPropertyValue);
    const NodeProperty& propertyValue() const { return _nodeProperty; }

private:
    // Property sequential number for given node type
    const PropertyID _propertyID;
    // Reference to actual node property
    NodeProperty& _nodeProperty;
    // Type of property data
    const EPropertyType _type;
    // Human-readable property name
    const std::string _name;
    // Optional parameters for UI engine
    std::string _uiHints;
    // Optional description
    std::string _description;
    // Optional property value validator
    std::unique_ptr<PropertyValidator> _validator;
    // Optional observer notified when property is changed
    std::unique_ptr<PropertyObserver> _observer;
};

// Additional, per-node settings
enum class ENodeConfig
{
    NoFlags                  = 0,
    // Node markes as a stateful
    HasState                 = K_BIT(0),
    // After one exec-run, node should be tagged for next one
    AutoTag                  = K_BIT(1),
    // Don't automatically do time computations for this node as it'll do it itself
    OverridesTimeComputation = K_BIT(2)
};
typedef EnumFlags<ENodeConfig> NodeConfigFlags;
K_DEFINE_ENUMFLAGS_OPERATORS(NodeConfigFlags)

// Describes node type with all its i/o sockets and properties
class LOGIC_EXPORT NodeConfig
{
public:
    explicit NodeConfig()
        : _flags(ENodeConfig::NoFlags)
    {
    }

    NodeConfig(const NodeConfig&) = delete;
    NodeConfig& operator=(const NodeConfig&) = delete;

    NodeConfig(NodeConfig&& other);
    NodeConfig& operator=(NodeConfig&& other);

    SocketConfig& addInput(std::string name, ENodeFlowDataType dataType);
    SocketConfig& addOutput(std::string name, ENodeFlowDataType dataType);
    PropertyConfig& addProperty(std::string name, NodeProperty& nodeProperty);

    void clearInputs();
    void clearOutputs();
    void clearProperties();

    NodeConfig& setDescription(std::string description)
    {
        _description = std::move(description);
        return *this;
    }

    NodeConfig& setModule(std::string module)
    {
        _module = std::move(module);
        return *this;
    }

    NodeConfig& setFlags(NodeConfigFlags configFlags)
    {
        _flags = configFlags;
        return *this;
    }

    const std::vector<SocketConfig>& inputs() const { return _inputs; }
    const std::vector<SocketConfig>& outputs() const { return _outputs; }
    const std::vector<PropertyConfig>& properties() const { return _properties; }
    std::vector<PropertyConfig>& properties() { return _properties; }
    const std::string& description() const { return _description; }
    const std::string& module() const { return _module; }
    NodeConfigFlags flags() const { return _flags; }

private:
    template <class T>
    bool checkNameUniqueness(const std::vector<T>& containter,
                             const std::string& name);

private:
    // Array of input socket descriptors
    std::vector<SocketConfig> _inputs;
    // Array of output socket descriptors
    std::vector<SocketConfig> _outputs;
    // Array of node property descriptors
    std::vector<PropertyConfig> _properties;
    // Optional human-readable description
    std::string _description;
    // Optional name of module that this node belongs to
    std::string _module;
    // Additional settings
    NodeConfigFlags _flags;
};

// Node execution status 
enum class EStatus : int
{
    // Everything was ok
    Ok,
    // There was an error during execution
    Error,
    // Mark this node for execution for next run
    // (requires Node_AutoTag flag set on)
    Tag
};

// Represents execution return information
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

    // If Node_OverridesTimeComputation is set on this value will be used
    // to display overriden time elapsed value
    double timeElapsed;
    // Node execution status
    EStatus status;
    // Additional message in case of an error
    std::string message;
};

class NodeType : public NodeConfig
{
public:
    virtual ~NodeType() {}

    // Required methods
    virtual ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) = 0;

    // Optional methods
    virtual bool restart();
    virtual void finish();
    virtual bool init(const std::shared_ptr<NodeModule>& module);

    NodeConfig& config() { return *this; }
    const NodeConfig& config() const { return *this; }
};

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
