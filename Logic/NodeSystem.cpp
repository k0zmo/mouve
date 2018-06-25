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

#include "NodeSystem.h"
#include "NodeType.h"
#include "NodeTree.h"
#include "NodeModule.h"
#include "NodePlugin.h"

#include <fmt/core.h>
#include <boost/filesystem/path.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/dll/shared_library.hpp>
#include <boost/dll/runtime_symbol_info.hpp>
#include <boost/predef/os/windows.h>

static const std::string InvalidType("InvalidType");

NodeSystem::NodeSystem()
{
    // Register InvalidNodeTypeID
    NodeTypeID invalidNodeTypeID = registerNodeType(InvalidType, nullptr);
    assert(invalidNodeTypeID == InvalidNodeTypeID);
    registerAutoTypes();
}

NodeSystem::~NodeSystem()
{
    _registeredNodeTypes.clear();
    _typeNameToTypeID.clear();
    _registeredModules.clear();
    _plugins.clear();
}

NodeTypeID NodeSystem::registerNodeType(const std::string& nodeTypeName,
    std::unique_ptr<NodeFactory> nodeFactory)
{
    // Check if we haven't already registed so named node
    auto iter = _typeNameToTypeID.find(nodeTypeName);
    if(iter == _typeNameToTypeID.end())
    {
        // New type of node
        size_t nodeTypeID = _registeredNodeTypes.size();
        _typeNameToTypeID[nodeTypeName] = NodeTypeID(nodeTypeID);

        // Associate type name with proper factory
        _registeredNodeTypes.emplace_back(NodeTypeInfo(nodeTypeName, std::move(nodeFactory)));

        return NodeTypeID(nodeTypeID);
    }
    else
    {
        /// TODO: return invalid typeID or just override previous one - make it configurable with a return status
        NodeTypeID nodeTypeID = iter->second;

        // Override previous factory with a new one
        if(_registeredNodeTypes[nodeTypeID].automaticallyRegistered)
            // Stops unique_ptr from deleting something that was allocated on a stack
            (void) _registeredNodeTypes[nodeTypeID].nodeFactory.release();
        _registeredNodeTypes[nodeTypeID].nodeFactory = std::move(nodeFactory);
        _registeredNodeTypes[nodeTypeID].automaticallyRegistered = false;

        return nodeTypeID;
    }

    // TODO: Create temporary node here and decide if its OK. Also, store its description and so on
    // so we don't need to do it later
}

std::unique_ptr<NodeType> NodeSystem::createNode(NodeTypeID nodeTypeID) const
{
    // Check if the type exists
    assert(nodeTypeID < _registeredNodeTypes.size());
    if(nodeTypeID >= _registeredNodeTypes.size())
        return nullptr;

    // Retrieve proper factory
    const auto& nodeFactory = _registeredNodeTypes[nodeTypeID].nodeFactory;
    if(nodeFactory != nullptr)
    {
        try
        {
            // Can throw because of bad configuration
            return nodeFactory->create();
        }
        catch (std::exception&)
        {
        }
    }

    return nullptr;
}

std::unique_ptr<NodeTree> NodeSystem::createNodeTree()
{
    return std::unique_ptr<NodeTree>(new NodeTree(this));
}

const std::string& NodeSystem::nodeTypeName(NodeTypeID nodeTypeID) const
{
    assert(nodeTypeID < _registeredNodeTypes.size());
    if(nodeTypeID >= _registeredNodeTypes.size())
        return InvalidType;
    return _registeredNodeTypes[nodeTypeID].nodeTypeName;
}

std::string NodeSystem::nodeDescription(NodeTypeID nodeTypeID) const
{
    auto tmpNode = createNode(nodeTypeID);
    if(!tmpNode)
        return InvalidType;
    return tmpNode->config().description();
}

NodeTypeID NodeSystem::nodeTypeID(const std::string& nodeTypeName) const
{
    auto iter = _typeNameToTypeID.find(nodeTypeName);
    return iter != _typeNameToTypeID.end()
        ? iter->second
        : InvalidNodeTypeID;
}

std::string NodeSystem::defaultNodeName(NodeTypeID nodeTypeID) const
{
    std::string defaultNodeTitle = nodeTypeName(nodeTypeID);
    size_t pos = defaultNodeTitle.find_last_of('/');
    if(pos != std::string::npos && pos < defaultNodeTitle.size() - 1)
        defaultNodeTitle = defaultNodeTitle.substr(pos + 1);
    return defaultNodeTitle;
}

void NodeSystem::registerAutoTypes()
{
    AutoRegisterNodeBase* autoFactory = AutoRegisterNodeBase::head();
    while(autoFactory)
    {
        auto nodeFactory = std::unique_ptr<NodeFactory>(autoFactory);
        NodeTypeID nodeTypeID = registerNodeType(autoFactory->typeName, std::move(nodeFactory));
        autoFactory = autoFactory->next();

        // We need empty deleter as autoFactory
        // are allocated on (global) stack.
        // We could get rid of unique_ptr at all
        // but that would lead to less friendly
        // interface for manually registered node types.
        // Unfortunately, we can't mixed unique_ptrs with 
        // default deleters and overridden by us
        _registeredNodeTypes[nodeTypeID].automaticallyRegistered = true;
    }
}

int NodeSystem::numRegisteredNodeTypes() const
{
    return static_cast<int>(_registeredNodeTypes.size());
}

// -----------------------------------------------------------------------------

NodeSystem::NodeTypeInfo::~NodeTypeInfo()
{
    if(automaticallyRegistered)
    {
        // This should stop unique_ptr from calling deleter
        (void) nodeFactory.release();
    }
}

NodeSystem::NodeTypeInfo::NodeTypeInfo(NodeSystem::NodeTypeInfo&& rhs)
{
    operator=(std::forward<NodeSystem::NodeTypeInfo>(rhs));
}

NodeSystem::NodeTypeInfo& NodeSystem::NodeTypeInfo::operator=(NodeSystem::NodeTypeInfo&& rhs)
{
    nodeTypeName = std::move(rhs.nodeTypeName);
    nodeFactory = std::move(rhs.nodeFactory);
    automaticallyRegistered = rhs.automaticallyRegistered;

    return *this;
}

bool NodeSystem::registerNodeModule(const std::shared_ptr<NodeModule>& module)
{
    // Don't allow for re-registering (TODO: add deregistering??)
    if(_registeredModules.find(module->moduleName()) != _registeredModules.end())
        return false;

    _registeredModules.insert(std::make_pair(module->moduleName(), module));
    return true;
}

const std::shared_ptr<NodeModule>& NodeSystem::nodeModule(const std::string& name)
{
    return _registeredModules[name];
}

namespace filesystem = boost::filesystem;

size_t NodeSystem::loadPlugin(const std::string& pluginPath)
{
    if (_plugins.find(pluginPath) == _plugins.end())
    {
        // Below is the same as:
        //   boost::dll::import<NodePlugin>(pluginPath, "plugin_instance")
        // but it uses std::shared_ptr instead of boost's one
        auto library = std::make_shared<boost::dll::shared_library>(
            pluginPath, boost::dll::load_mode::default_mode);
        if (!library->has("plugin_instance"))
            return 0U;
        auto plugin = std::shared_ptr<NodePlugin>{
            library, std::addressof(library->get<NodePlugin>("plugin_instance"))};

        if (plugin->logicVersion() != LOGIC_VERSION)
        {
            throw std::runtime_error(fmt::format(
                "Logic ({}) and plugin {} ({}) version mismatch", LOGIC_VERSION,
                filesystem::path{pluginPath}.filename().string(), plugin->logicVersion()));
        }
        size_t before = _registeredNodeTypes.size();
        plugin->registerPlugin(*this);

        _plugins.emplace(pluginPath, std::move(plugin));
        return _registeredNodeTypes.size() - before;
    }

    return 0U;
}

void NodeSystem::loadPlugins()
{
    const auto lookUpDir = boost::dll::program_location().parent_path() / "plugins";
    if (!filesystem::exists(lookUpDir))
        return;

    for (const filesystem::directory_entry& entry : filesystem::directory_iterator{lookUpDir})
    {
#if BOOST_OS_WINDOWS
        static constexpr auto pluginExtension = ".dll";
#else
        static constexpr auto pluginExtension = ".so";
#endif

        if (entry.path().extension() != pluginExtension)
            continue;

        try
        {
            loadPlugin(entry.path().string());
            // TODO: (debug) Log all registered nodes if possible
        }
        catch (std::exception&)
        {
            // TODO: Log error
        }
    }
}

// -----------------------------------------------------------------------------

class NodeSystem::NodeTypeIteratorImpl : public NodeTypeIterator
{
public:
    NodeTypeIteratorImpl(const std::vector<NodeSystem::NodeTypeInfo>& registeredNodeTypes)
        : _nodeTypeID(InvalidNodeTypeID)
        , _registeredNodeTypes(registeredNodeTypes)
        , _iterator(registeredNodeTypes.cbegin())
    {
        // Iterator points to first valid type info
        if(_iterator != _registeredNodeTypes.cend())
            ++_iterator;
    }

    virtual bool next(NodeTypeIterator::NodeTypeInfo& nodeType)
    {
        if(_iterator == _registeredNodeTypes.cend())
            return false;

        nodeType.typeID = ++_nodeTypeID;
        nodeType.typeName = _iterator->nodeTypeName;

        ++_iterator;
        return true;
    }

private:
    int _nodeTypeID;
    const std::vector<NodeSystem::NodeTypeInfo>& _registeredNodeTypes;
    std::vector<NodeSystem::NodeTypeInfo>::const_iterator _iterator;
};

std::unique_ptr<NodeTypeIterator> NodeSystem::createNodeTypeIterator() const
{
    return std::unique_ptr<NodeTypeIterator>(new NodeTypeIteratorImpl(_registeredNodeTypes));
}
