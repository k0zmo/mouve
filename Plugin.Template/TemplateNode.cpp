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

#include "Logic/NodePlugin.h"
#include "Logic/NodeSystem.h"
#include "Logic/NodeType.h"

#include "Kommon/StringUtils.h"

class TemplateNodeType : public NodeType
{
public:
    TemplateNodeType()
        : _property(true)
    {
        addInput("Image", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::Image);
        addProperty("Property", _property);
        setDescription("Template node type");
        setFlags(ENodeConfig::NoFlags);
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // inputs
        const cv::Mat& src = reader.readSocket(0).getImage();
        // outputs
        cv::Mat& dst = writer.acquireSocket(0).getImage();

        // validate inputs
        if(src.empty())
            return ExecutionStatus(EStatus::Ok);

        if(_property)
        {
            dst = src;
        }

        return ExecutionStatus(EStatus::Ok, 
            string_format("Property value: %s", _property ? "true" : "false"));
    }

private:
    TypedNodeProperty<bool> _property;
};

void registerTemplate(NodeSystem* nodeSystem)
{
    typedef DefaultNodeFactory<TemplateNodeType> TemplateFactory;
    nodeSystem->registerNodeType("Test/Template",
        std::unique_ptr<NodeFactory>(new TemplateFactory()));
}