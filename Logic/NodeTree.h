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
#include "Node.h"
#include "NodeLink.h"

enum class ELinkNodesResult
{
    // Linking was successful
    Ok,
    // Cycle would be created (which is forbidden)
    CycleDetected,
    // Tried to link already linked input (two)
    TwoOutputsOnInput,
    // Linking addresses were invalid
    InvalidAddress
};

class LOGIC_EXPORT NodeTree
{
    K_DISABLE_COPY(NodeTree)
public:
    NodeTree(NodeSystem* nodeSystem);
    ~NodeTree();

    void clear();
    size_t nodesCount() const;
    const NodeSystem* nodeSystem() const;

    void tagNode(NodeID nodeID);
    void untagNode(NodeID nodeID);
    void setNodeEnable(NodeID nodeID, bool enable);
    bool isNodeExecutable(NodeID nodeID) const;
    bool isTreeStateless() const;

    std::vector<NodeID> prepareList();
    void execute(bool withInit = false);
    void notifyFinish();
    
    // Returns sorted list of node in order of their execution
    // Returned list is valid between prepareList() calls.
    std::vector<NodeID> executeList() const;

    NodeID createNode(const std::string& nodeTypeName, const std::string& name);
    NodeID createNode(NodeTypeID typeID, const std::string& name);
    bool removeNode(NodeID nodeID);
    bool removeNode(const std::string& nodeName);
    NodeID duplicateNode(NodeID nodeID);
    std::string generateNodeName(NodeTypeID typeID) const;

    ELinkNodesResult linkNodes(SocketAddress from, SocketAddress to);
    bool unlinkNodes(SocketAddress from, SocketAddress to);

    // Set a new name for a given node
    bool setNodeName(NodeID nodeID, const std::string& newNodeName);
    // Returns the current node name for a given node
    const std::string& nodeName(NodeID nodeID) const;

    // For a given a nodeName returns NodeID
    NodeID resolveNode(const std::string& nodeName) const;
    // For a given NodeID returns its type ID
    NodeTypeID nodeTypeID(NodeID nodeID) const;
    // For a given NodeID returns its type name as string
    const std::string& nodeTypeName(NodeID nodeID) const;

    // Returns output socket address from which given input socket is connected to
    SocketAddress connectedFrom(SocketAddress iSocketAddress) const;
    const NodeFlowData& outputSocket(NodeID nodeID, SocketID socketID) const;
    const NodeFlowData& inputSocket(NodeID nodeID, SocketID socketID) const;

    // Returns true if given input socket is connected 
    bool isInputSocketConnected(NodeID nodeID, SocketID socketID) const;
    // Returns true if given output socket is connected
    bool isOutputSocketConnected(NodeID nodeID, SocketID socketID) const;

    bool allRequiredInputSocketConnected(NodeID nodeID) const;
    bool taggedButNotExecuted(NodeID nodeID) const;

    const NodeConfig& nodeConfiguration(NodeID nodeID) const;
    const NodeConfig* nodeConfigurationPtr(NodeID nodeID) const;
    bool nodeSetProperty(NodeID nodeID, PropertyID propID, const NodeProperty& value);
    NodeProperty nodeProperty(NodeID nodeID, PropertyID propID);
    const std::string& nodeExecuteInformation(NodeID nodeID) const;
    double nodeTimeElapsed(NodeID nodeID) const;

    std::unique_ptr<NodeIterator> createNodeIterator();
    std::unique_ptr<NodeLinkIterator> createNodeLinkIterator();	

private:
    NodeID allocateNodeID();
    void deallocateNodeID(NodeID id);

    size_t firstOutputLink(NodeID fromNode, SocketID fromSocket, size_t start = 0) const;
    std::tuple<size_t, size_t> outLinks(NodeID fromNode) const;
    bool checkCycle(NodeID startNode);
    void prepareListImpl();

    enum class ENodeColor : int;
    bool depthFirstSearch(NodeID startNodeID,
        std::vector<ENodeColor>& colorMap,
        std::vector<NodeID>* execList = nullptr);

    bool validateLink(SocketAddress& from, SocketAddress& to);
    bool validateNode(NodeID nodeID) const;

    void cleanUpAfterExecution(const std::vector<NodeID>& selfTagging,
        const std::vector<NodeID>& correctlyExecutedNodes);
    void handleException(const std::string& nodeName, const std::string& nodeTypeName);

private:
    std::vector<Node> _nodes;
    std::vector<NodeID> _recycledIDs;
    std::vector<NodeLink> _links;
    std::vector<NodeID> _executeList;
    std::unordered_map<std::string, NodeID> _nodeNameToNodeID;
    NodeSystem* _nodeSystem;
    bool _executeListDirty;

private:
    // Interfaces implementations
    class NodeIteratorImpl;
    class NodeLinkIteratorImpl;
};
