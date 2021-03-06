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

#include "NodeTree.h"
#include "Node.h"
#include "NodeType.h"
#include "NodeLink.h"
#include "NodeSystem.h"
#include "NodeModule.h"
#include "NodeException.h"

namespace {
static bool validateNodeName(const std::string& nodeName)
{
    return nodeName.find('/') == std::string::npos;
}
}

NodeTree::NodeTree(NodeSystem* nodeSystem)
    : _nodeSystem(nodeSystem)
    , _executeListDirty(false)
{
}

NodeTree::~NodeTree()
{
    clear();
}

void NodeTree::clear()
{
    _nodes.clear();
    _recycledIDs.clear();
    _links.clear();
    _executeList.clear();
    _nodeNameToNodeID.clear();
    _executeListDirty = false;
}

size_t NodeTree::nodesCount() const
{
    return _nodes.size();
}

const NodeSystem* NodeTree::nodeSystem() const
{
    return _nodeSystem;
}

void NodeTree::tagNode(NodeID nodeID)
{
    if(!validateNode(nodeID))
        return;

    _nodes[nodeID].setFlag(ENodeFlags::Tagged);
    _executeListDirty = true;
}

void NodeTree::untagNode(NodeID nodeID)
{
    if(!validateNode(nodeID))
        return;

    auto& node = _nodes[nodeID];

    if(node.flag(ENodeFlags::Tagged))
    {
        node.unsetFlag(ENodeFlags::Tagged);
        _executeListDirty = true;
    }
}

void NodeTree::setNodeEnable(NodeID nodeID, bool enable)
{
    if(!validateNode(nodeID))
        return;

    auto& node = _nodes[nodeID];

    if(node.flag(ENodeFlags::Disabled) == enable)
    {
        _executeListDirty = true;

        if(enable)
            node.unsetFlag(ENodeFlags::Disabled);
        else
            node.setFlag(ENodeFlags::Disabled);
    }
}

bool NodeTree::isNodeExecutable(NodeID nodeID) const
{
    return validateNode(nodeID) 
        && !_nodes[nodeID].flag(ENodeFlags::Disabled)
        && allRequiredInputSocketConnected(nodeID);
}

bool NodeTree::isTreeStateless() const
{
    for(const Node& node : _nodes)
    {
        if(node.flag(ENodeFlags::StateNode))
            return false;
    }

    return true;
}

std::vector<NodeID> NodeTree::prepareList()
{
    if(!_executeListDirty)
        return _executeList;

    prepareListImpl();

    _executeListDirty = false;
    return _executeList;
}

void NodeTree::handleException(NodeID nodeID, const NodeSocketTracer& tracer)
{
    // Exception dispatcher pattern
    try
    {
        _executeListDirty = true;
        throw;
    }
    catch(ExecutionError&)
    {
        // Dummy but a must - if not std::exception handler would catch it
        throw;
    }
    catch(BadConnectionException& ex)
    {
        if (tracer.isLastOutput())
            // Means node tried to acquired socket
            // with different type than declared in config
            throw BadConfigException();
        ex.node = tracer.lastNode();
        ex.socket = tracer.lastSocket();
        throw;
    }
    catch(cv::Exception& ex)
    {
        throw ExecutionError(nodeName(nodeID), nodeTypeName(nodeID), 
            std::string("OpenCV exception - ") + ex.what());
    }
    catch(std::exception& ex)
    {
        throw ExecutionError(nodeName(nodeID), nodeTypeName(nodeID), ex.what());
    }
}

void NodeTree::execute(bool withInit)
{
    if(_executeListDirty)
        prepareList();

    NodeSocketTracer tracer;
    NodeSocketReader reader(this, tracer);
    NodeSocketWriter writer(tracer);

    // Traverse through just-built exec list and process each node 
    for(NodeID nodeID : _executeList)
    {
        Node& node = _nodes[nodeID];

        tracer.setNode(nodeID);
        reader.setNode(nodeID, node.numInputSockets());

        try
        {
            if(withInit && _nodes[nodeID].flag(ENodeFlags::StateNode))
            {
                // Try to restart node internal state if any
                if(!node.restart())
                {
                    throw ExecutionError{node.nodeName(), nodeTypeName(nodeID), 
                        "Error during node state restart"};
                }
            }

            ExecutionStatus ret = node.execute(reader, writer);

            switch (ret.status)
            {
            case EStatus::Tag:
                // Tag for next execution
                tagNode(nodeID);
                break;

            case EStatus::Error:
                // If node reported an error
                tagNode(nodeID);
                throw ExecutionError{node.nodeName(), nodeTypeName(nodeID), 
                    ret.message};
                break;

            case EStatus::Ok:
            default:
                break;
            }
        }
        catch(...)
        {
            handleException(nodeID, tracer);
        }
    }

    _executeListDirty = true;
}

void NodeTree::notifyFinish()
{
    for(NodeID nodeID = 0; nodeID < NodeID(_nodes.size()); ++nodeID)
    {
        if(!validateNode(nodeID))
            continue;

        _nodes[nodeID].finish();

        // Finally, tag node that need to be auto-tagged
        if(_nodes[nodeID].flag(ENodeFlags::AutoTag))
            tagNode(nodeID);
    }
}

std::vector<NodeID> NodeTree::executeList() const
{
    return _executeList;
}

NodeID NodeTree::createNode(const std::string& nodeTypeName, const std::string& name)
{
    return createNode(_nodeSystem->nodeTypeID(nodeTypeName), name);
}

NodeID NodeTree::createNode(NodeTypeID typeID, const std::string& name)
{
    if(!_nodeSystem)
        return InvalidNodeID;

    // Check if the given name is unique
    if(_nodeNameToNodeID.find(name) != _nodeNameToNodeID.end())
        return InvalidNodeID;

    // Validate correctness of given node name
    if(!validateNodeName(name))
        return InvalidNodeID;

    // Allocate NodeID
    NodeID id = allocateNodeID();
    if(id == InvalidNodeID)
        return id;

    // Create node (type) of a given type
    std::unique_ptr<NodeType> nodeType = _nodeSystem->createNode(typeID);
    if(nodeType == nullptr)
    {
        deallocateNodeID(id);
        return InvalidNodeID;
    }

    // If node type belongs to a registered module
    const NodeConfig& nodeConfig = nodeType->config();
    if(!nodeConfig.module().empty())
    {
        bool res = false;
        const auto& module = _nodeSystem->nodeModule(nodeConfig.module());
        if(module && module->ensureInitialized())
            res = nodeType->init(module);

        // Module not initialized - rollback
        if(!res)
        {
            deallocateNodeID(id);
            return InvalidNodeID;
        }
    }

    // Create actual node object
    _nodes[id] = Node(std::move(nodeType), name, typeID);
    // Add the pair <node name, node ID> to hash map
    _nodeNameToNodeID.insert(std::make_pair(name, id));
    // Tag it for next execution
    tagNode(id);

    return id;
}

bool NodeTree::removeNode(NodeID nodeID)
{
    if(!validateNode(nodeID))
        return false;

    // Tag nodes that are directly connected to the removed node
    for(const auto& nodeLink : _links)
    {
        if(nodeLink.fromNode == nodeID)
            tagNode(nodeLink.toNode);
    }

    // Remove any referring links
    auto removeFirstNodeLink = std::remove_if(
        std::begin(_links), std::end(_links),
        [&](const NodeLink& nodeLink)
        {
            return nodeLink.fromNode == nodeID ||
                   nodeLink.toNode   == nodeID;
        });
    _links.erase(removeFirstNodeLink, std::end(_links));

    // Keep links sorted
    std::sort(std::begin(_links), std::end(_links));

    // Finally remove the node
    deallocateNodeID(nodeID);

    return true;
}

bool NodeTree::removeNode(const std::string& nodeName)
{
    auto iter = _nodeNameToNodeID.find(nodeName);
    if(iter == _nodeNameToNodeID.end())
        return false;

    return removeNode(iter->second);
}

NodeID NodeTree::duplicateNode(NodeID nodeID)
{
    if(!validateNode(nodeID))
        return InvalidNodeID;
    NodeTypeID typeID = _nodes[nodeID].nodeTypeID();

    // Generate unique name
    std::string newNodeTitle = generateNodeName(typeID);

    // Create node of the same typeID
    NodeID newNodeID = createNode(typeID, newNodeTitle);
    if(newNodeID == InvalidNodeID)
        return InvalidNodeID;

    // Set properties
    for(const auto& prop : _nodes[nodeID].config().properties())
        _nodes[newNodeID].setProperty(prop.propertyID(), _nodes[nodeID].property(prop.propertyID()));

    return newNodeID;
}

std::string NodeTree::generateNodeName(NodeTypeID typeID) const
{
    // Generate unique name - defaultNodeName() method guarantees there won't be any '/' char
    std::string defaultNodeTitle = _nodeSystem->defaultNodeName(typeID);
    std::string nodeTitle = defaultNodeTitle;
    int num = 1;
    while(resolveNode(nodeTitle) != InvalidNodeID)
    {
        nodeTitle = defaultNodeTitle + " " + std::to_string(num);
        ++num;
    }
    return nodeTitle;
}

ELinkNodesResult NodeTree::linkNodes(SocketAddress from, SocketAddress to)
{
    // Check if given addresses are OK
    if(!validateLink(from, to))
        return ELinkNodesResult::InvalidAddress;

    // Check if we are not trying to link input socket with more than one output
    bool alreadyExisingConnection = std::any_of(std::begin(_links), std::end(_links), 
        [&](const NodeLink& link) {
            return link.toNode == to.node && link.toSocket == to.socket;
        });
    if(alreadyExisingConnection)
        return ELinkNodesResult::TwoOutputsOnInput;

    // Try to make a connection
    _links.emplace_back(NodeLink(from, to));

    // Keep links sorted
    std::sort(std::begin(_links), std::end(_links));

    // Check for created cycle(s)
    if(checkCycle(to.node))
    {
        // Look for given node link
        NodeLink nodeLinkToFind(from, to);
        auto iter = std::lower_bound(std::begin(_links), 
            std::end(_links), nodeLinkToFind);

        assert(iter != std::end(_links));

        if(iter != std::end(_links))
        {
            _links.erase(iter);

            // Keep links sorted
            std::sort(std::begin(_links), std::end(_links));
        }

        return ELinkNodesResult::CycleDetected;
    }

    // tag affected node
    tagNode(to.node);

    return ELinkNodesResult::Ok;
}

bool NodeTree::unlinkNodes(SocketAddress from, SocketAddress to)
{
    if(!validateLink(from, to))
        return false;

    // Look for given node link
    NodeLink nodeLinkToFind(from, to);
    auto iter = std::lower_bound(std::begin(_links), 
        std::end(_links), nodeLinkToFind);

    if(iter != std::end(_links))
    {
        // Need to store ID before erase() invalidates iterator
        NodeID nodeID = iter->toNode;

        _links.erase(iter);

        // Keep links sorted
        std::sort(std::begin(_links), std::end(_links));

        // tag affected node
        tagNode(nodeID);

        return true;
    }

    return false;
}

bool NodeTree::setNodeName(NodeID nodeID, const std::string& newNodeName)
{
    if(nodeID < _nodes.size())
    {
        auto& node = _nodes[nodeID];
        if(node.isValid() && node.nodeName() != newNodeName)
        {
            // Check if new node name doesn't contain forbidden characters ('/')
            if(validateNodeName(newNodeName))
            {
                // Remove old mapping
                _nodeNameToNodeID.erase(node.nodeName());

                // Change a name and add a new mapping
                node.setNodeName(newNodeName);
                _nodeNameToNodeID.insert(std::make_pair(newNodeName, nodeID));

                return true;
            }
        }
    }

    return false;
}

const std::string& NodeTree::nodeName(NodeID nodeID) const
{
    static const std::string InvalidNodeStr("InvalidNode");
    static const std::string InvalidNodeIDStr("InvalidNodeID");

    if(nodeID < _nodes.size())
    {
        const auto& node = _nodes[nodeID];
        if(node.isValid())
        {
            return node.nodeName();
        }
        return InvalidNodeStr;
    }
    return InvalidNodeIDStr;
}

NodeID NodeTree::resolveNode(const std::string& nodeName) const
{
    auto iter = _nodeNameToNodeID.find(nodeName);
    if(iter != _nodeNameToNodeID.end())
    {
        return iter->second;
    }
    else
    {
        return InvalidNodeID;
    }
}

NodeTypeID NodeTree::nodeTypeID(NodeID nodeID) const
{
    if(!validateNode(nodeID))
        return InvalidNodeTypeID;
    return _nodes[nodeID].nodeTypeID();
}

const std::string& NodeTree::nodeTypeName(NodeID nodeID) const
{
    if(_nodeSystem != nullptr)
    {
        if(nodeID < _nodes.size())
        {
            NodeTypeID nodeTypeID = _nodes[nodeID].nodeTypeID();
            return _nodeSystem->nodeTypeName(nodeTypeID);
        }
    }

    static const std::string EmptyString = "";
    return EmptyString;
}

SocketAddress NodeTree::connectedFrom(SocketAddress iSocketAddr) const
{
    // This function does not rely on a assumption that links are sorted
    SocketAddress ret(InvalidNodeID, InvalidSocketID, false);

    // ... and only works when addr points to input socket
    if(iSocketAddr.isOutput)
        return ret;

    for(const NodeLink& link: _links)
    {
        if(link.toNode == iSocketAddr.node &&
           link.toSocket == iSocketAddr.socket)
        {
            ret.node = link.fromNode;
            ret.socket = link.fromSocket;
            ret.isOutput = true;
            break;
        }
    }

    return ret;
}

const NodeFlowData& NodeTree::outputSocket(NodeID nodeID, SocketID socketID) const
{
    if(!validateNode(nodeID))
        throw BadNodeException();

    if(!allRequiredInputSocketConnected(nodeID))
    {
        /// TODO: Transform this to a method (same as NodeType::readSocket())
        static NodeFlowData _default = NodeFlowData();
        return _default;
    }

    // outputSocket verifies socketID
    return _nodes[nodeID].outputSocket(socketID);
}

const NodeFlowData& NodeTree::inputSocket(NodeID nodeID, SocketID socketID) const
{
    if(!validateNode(nodeID))
        throw BadNodeException();

    SocketAddress outputAddr = connectedFrom(
        SocketAddress(nodeID, socketID, false));
    // outputSocket verifies outputAddr
    return outputSocket(outputAddr.node, outputAddr.socket);
}

bool NodeTree::isInputSocketConnected(NodeID nodeID, SocketID socketID) const
{
    SocketAddress outputAddr = connectedFrom(
        SocketAddress(nodeID, socketID, false));
    return outputAddr.isValid();
}

bool NodeTree::isOutputSocketConnected(NodeID nodeID, SocketID socketID) const
{
    return firstOutputLink(nodeID, socketID) != _links.size();
}

bool NodeTree::allRequiredInputSocketConnected(NodeID nodeID) const
{
    if(!validateNode(nodeID))
        return false;

    const auto& node = _nodes[nodeID];

    for(SocketID socketID = 0; socketID < node.numInputSockets(); ++socketID)
    {
        SocketAddress outputAddr = connectedFrom(SocketAddress(nodeID, socketID, false));
        if(outputAddr.node == InvalidNodeID || outputAddr.socket == InvalidSocketID)
            return false;
    }

    return true;
}

bool NodeTree::taggedButNotExecuted(NodeID nodeID) const
{
    // Node is tagged ..
    if(_nodes[nodeID].flag(ENodeFlags::Tagged)
        // .. and isn't on the executed list
        && std::find(_executeList.begin(), _executeList.end(), nodeID) == _executeList.end())
    {
        return true;
    }

    return false;
}

const NodeConfig& NodeTree::nodeConfiguration(NodeID nodeID) const
{
    const NodeConfig* cfg = nodeConfigurationPtr(nodeID);

    if(!cfg)
        throw BadNodeException();
    return *cfg;
}

const NodeConfig* NodeTree::nodeConfigurationPtr(NodeID nodeID) const
{
    if(!validateNode(nodeID))
        return nullptr;

    return &_nodes[nodeID].config();
}

bool NodeTree::nodeSetProperty(NodeID nodeID, PropertyID propID, const NodeProperty& value)
{
    if(!validateNode(nodeID))
        return false;

    return _nodes[nodeID].setProperty(propID, value);
}

NodeProperty NodeTree::nodeProperty(NodeID nodeID, PropertyID propID)
{
    if(!validateNode(nodeID))
        return NodeProperty();

    return _nodes[nodeID].property(propID);
}

const std::string& NodeTree::nodeExecuteInformation(NodeID nodeID) const
{
    static const std::string empty;
    if(!validateNode(nodeID))
        return empty;

    return _nodes[nodeID].executeInformation();
}

double NodeTree::nodeTimeElapsed(NodeID nodeID) const
{
    if(!validateNode(nodeID))
        return 0.0;

    return _nodes[nodeID].timeElapsed();
}

NodeID NodeTree::allocateNodeID()
{
    NodeID id = InvalidNodeID;

    // Check for recycled ids
    if(_recycledIDs.empty())
    {
        // Need to create new one
        id = NodeID(_nodes.size());
        // Expand node array - createNode relies on it
        _nodes.resize(_nodes.size() + 1); 
    }
    else
    {
        // Reuse unused but still cached one
        id = _recycledIDs.back();
        _recycledIDs.pop_back();
    }

    return id;
}

void NodeTree::deallocateNodeID(NodeID id)
{
    if(!validateNode(id))
        return;

    // Remove mapping from node name to node ID
    _nodeNameToNodeID.erase(_nodes[id].nodeName());

    untagNode(id);

    // Invalidate node
    _nodes[id] = Node();

    // Add NodeID to recycled ones
    _recycledIDs.push_back(id);
}

size_t NodeTree::firstOutputLink(NodeID fromNode,
    SocketID fromSocket, size_t start) const
{
    for(size_t i = start; i < _links.size(); ++i)
    {
        const auto& link = _links[i];

        if(link.fromNode == fromNode &&
           link.fromSocket == fromSocket)
        {
            return i;
        }
    }
    return _links.size();
}

std::tuple<size_t, size_t> NodeTree::outLinks(NodeID fromNode) const
{
    auto start = std::find_if(std::begin(_links), std::end(_links), 
        [=](const NodeLink& link) {
            return link.fromNode == fromNode;
    });

    // There are some output links from this node
    if(start != std::end(_links))
    {
        // Now, get last+1 output link index
        auto end = std::find_if(start + 1, std::end(_links), 
            [=](const NodeLink& link) {
                return link.fromNode != fromNode;
        });

        return std::make_tuple(
            size_t(start - std::begin(_links)), 
            size_t(end - std::begin(_links)));
    }
    else
    {
        // No output links from this node
        return std::make_tuple(_links.size(), _links.size());
    }
}

enum class NodeTree::ENodeColor : int
{
    White,
    Gray,
    Black
};

// Internally uses DFS for graph traversal
bool NodeTree::checkCycle(NodeID startNode)
{
    // Initialize color map with white color
    std::vector<ENodeColor> colorMap(_nodes.size(), ENodeColor::White);

    return !depthFirstSearch(startNode, colorMap);
}

// Internally uses DFS for graph traversal (in fact it's topological sort)
void NodeTree::prepareListImpl()
{
    // Initialize color map with white color
    std::vector<ENodeColor> colorMap(_nodes.size(), ENodeColor::White);

    _executeList.clear();

    // For each invalid nodes (in terms of executable) mark them black
    // so "true" graph traversal can skip them
    for(NodeID nodeID = 0; nodeID < NodeID(_nodes.size()); ++nodeID)
    {
        if(!validateNode(nodeID))
            continue;

        if(colorMap[nodeID] != ENodeColor::White)
            continue;

        if(isNodeExecutable(nodeID))
            continue;

        if(!depthFirstSearch(nodeID, colorMap))
            return;
    }	

    // For each tagged and valid nodes that haven't been visited yet do ..
    for(NodeID nodeID = 0; nodeID < NodeID(_nodes.size()); ++nodeID)
    {
        if(!validateNode(nodeID))
            continue;

        if(colorMap[nodeID] != ENodeColor::White)
            continue;

        if(!_nodes[nodeID].flag(ENodeFlags::Tagged))
            continue;

        if(!depthFirstSearch(nodeID, colorMap, &_executeList))
        {
            _executeList.clear();
            return;
        }
    }

    // Topological sort gives result in inverse order
    std::reverse(std::begin(_executeList), std::end(_executeList));
}

bool NodeTree::depthFirstSearch(NodeID nodeID, 
                                std::vector<ENodeColor>& colorMap, 
                                std::vector<NodeID>* execList)
{
    colorMap[nodeID] = ENodeColor::Gray;

    // Helper container for iterative DFS
    // <NodeID, index of first output link, index of last+1 output link>
    typedef std::tuple<NodeID, size_t, size_t> EdgeInfo;

    std::vector<EdgeInfo> stack;
    stack.push_back(std::tuple_cat(std::make_tuple(nodeID), outLinks(nodeID)));

    while(!stack.empty())
    {
        NodeID nodeID; size_t link, link_end;
        std::tie(nodeID, link, link_end) = stack.back();
        stack.pop_back();

        // For each output links
        while(link != link_end)
        {
            // Get target node and its color
            auto targetLink = std::begin(_links) + link;
            NodeID targetNodeID = targetLink->toNode;
            ENodeColor color = colorMap[targetNodeID];

            // Still not explored vertex - more depth search
            if(color == ENodeColor::White)
            {
                stack.push_back(std::make_tuple(nodeID, ++link, link_end));
                nodeID = targetNodeID;
                colorMap[nodeID] = ENodeColor::Gray;
                std::tie(link, link_end) = outLinks(nodeID);
            }
            // Shouldn't happen in acyclic graph
            else if(color == ENodeColor::Gray)
            {
                return false;
            }
            // We've been here - go to next node
            else// if(color == EVertexColor == Black)
            {
                ++link; 
            }
        }

        colorMap[nodeID] = ENodeColor::Black;
        if(execList) execList->push_back(nodeID);
    }

    return true;
}

bool NodeTree::validateLink(SocketAddress& from, SocketAddress& to)
{
    if(from.isOutput == to.isOutput)
        return false;

    // A little correction
    if(!from.isOutput)
        std::swap(from, to);

    if(!validateNode(from.node) || 
        !_nodes[from.node].validateSocket(from.socket, from.isOutput))
        return false;

    if(!validateNode(to.node) ||
        !_nodes[to.node].validateSocket(to.socket, to.isOutput))
        return false;

    return true;
}

bool NodeTree::validateNode(NodeID nodeID) const
{
    if(nodeID >= _nodes.size() || nodeID == InvalidNodeID)
        return false;
    return _nodes[nodeID].isValid();
}

// -----------------------------------------------------------------------------

class NodeTree::NodeIteratorImpl : public NodeIterator
{
public:
    NodeIteratorImpl(const NodeTree* parent)
        : _parent(parent)
        , _iter(parent->_nodeNameToNodeID.begin())
    {}

    virtual const Node* next(NodeID& nodeID)
    {
        const Node* out = nullptr;
        nodeID = InvalidNodeID;

        if(_iter != _parent->_nodeNameToNodeID.end())
        {
            nodeID = _iter->second;
            out = &_parent->_nodes[nodeID];
            ++_iter;
        }

        return out;
    }

private:
    const NodeTree* _parent;
    std::unordered_map<std::string, NodeID>::const_iterator _iter;
};

std::unique_ptr<NodeIterator> NodeTree::createNodeIterator() const
{
    return std::unique_ptr<NodeIterator>(new NodeIteratorImpl(this));
}

class NodeTree::NodeLinkIteratorImpl : public NodeLinkIterator
{
public:
    NodeLinkIteratorImpl(const NodeTree* parent)
        : _parent(parent)
        , _iter(parent->_links.begin())
    {}

    virtual bool next(NodeLink& nodeLink)
    {
        if(_parent)
        {
            if(_iter == _parent->_links.end())
                return false;

            nodeLink.fromNode = _iter->fromNode;
            nodeLink.fromSocket = _iter->fromSocket;
            nodeLink.toNode = _iter->toNode;
            nodeLink.toSocket = _iter->toSocket;

            ++_iter;
        }
        return true;
    }

private:
    const NodeTree* _parent;
    std::vector<NodeLink>::const_iterator _iter;
};

std::unique_ptr<NodeLinkIterator> NodeTree::createNodeLinkIterator() const
{
    return std::unique_ptr<NodeLinkIterator>(new NodeLinkIteratorImpl(this));
}

class NodeTree::NodeExecutorImpl : public NodeExecutor
{
public:
     NodeExecutorImpl(NodeTree* nodeTree, bool withInit)
        : _nodeTree{nodeTree}
        , _writer{_tracer}
        , _reader{_nodeTree, _tracer}
        , _withInit{withInit}
    {
    }

    ~NodeExecutorImpl() override
    {
        _nodeTree->_executeListDirty = true;
    }

    NodeID currentNode() const override
    {
        return _nodeTree->executeList()[_pos];
    }

    bool hasWork() const override
    {
        return _pos < _nodeTree->executeList().size();
    }

    void doWork() override
    {
        if (!hasWork()) return;
        NodeID nodeID = currentNode();
        Node& node = _nodeTree->_nodes[nodeID];

        _tracer.setNode(nodeID);
        _reader.setNode(nodeID, node.numInputSockets());

        try
        {
            if(_withInit && node.flag(ENodeFlags::StateNode))
            {
                // Try to restart node internal state if any
                if(!node.restart())
                {
                    throw ExecutionError{node.nodeName(), nodeTypeName(nodeID),
                        "Error during node state restart"};
                }
            }

            ExecutionStatus ret = node.execute(_reader, _writer);

            switch (ret.status)
            {
            case EStatus::Tag:
                // Tag for next execution
                tagNode(nodeID);
                break;

            case EStatus::Error:
                // If node reported an error
                tagNode(nodeID);
                throw ExecutionError{node.nodeName(), nodeTypeName(nodeID), 
                    ret.message};
                break;

            case EStatus::Ok:
            default:
                break;
            }

            ++_pos;
        }
        catch (...)
        {
            _nodeTree->handleException(nodeID, _tracer);
        }
    }

private:
    void tagNode(NodeID nodeID)
    {
        _nodeTree->tagNode(nodeID);
    }

    const std::string& nodeTypeName(NodeID nodeID) const
    {
        return _nodeTree->nodeTypeName(nodeID);
    }

private:
    NodeTree* _nodeTree;
    NodeSocketTracer _tracer;
    NodeSocketWriter _writer;
    NodeSocketReader _reader;
    size_t _pos{0};
    bool _withInit;
};

std::unique_ptr<NodeExecutor> NodeTree::createNodeExecutor(bool withInit)
{
    if(_executeListDirty)
        prepareList();

    return std::unique_ptr<NodeExecutor>(new NodeExecutorImpl{this, withInit});
}
