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

#include "NodeSystem.h"

#include <boost/config.hpp>
#include <memory>

class NodePlugin
{
public:
    virtual ~NodePlugin() {}

    virtual int logicVersion() const = 0;
    virtual int pluginVersion() const = 0;

    // TODO Rename to registerNodeTypes
    virtual void registerPlugin(NodeSystem& system) = 0;
};

//std::unique_ptr<NodeType>

#define MOUVE_DECLARE_PLUGIN(version)                                                              \
private:                                                                                           \
    int logicVersion() const override { return LOGIC_VERSION; }                                    \
    int pluginVersion() const override { return version; }

#define MOUVE_INSTANTIATE_PLUGIN(name)                                                             \
    extern "C" BOOST_SYMBOL_EXPORT name plugin_instance;                                           \
    name plugin_instance;

/*
 * Example of usage:
 *
 * class XPlugin : public NodePlugin
 * {
 *     MOUVE_DECLARE_PLUGIN(1); // plugin version
 *
 * public:
 *     void registerPlugin(NodeSystem& system) override
 *     {
 *         system.registerNodeType("a/b/c", makeDefaultNodeFactory<YNodeType>());
 *     }
 * };
 *
 * MOUVE_INSTANTIATE_PLUGIN(XPlugin)
 *
 */
