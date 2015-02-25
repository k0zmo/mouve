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

#include "Logic/NodeType.h"
#include "Logic/NodeSystem.h"

#include "cvu.h"

class KuwaharaFilterNodeType : public NodeType
{
public:
    KuwaharaFilterNodeType()
        : _radius(2)
    {
        addInput("Image", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::Image);
        addProperty("Radius", _radius)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 15))
            .setUiHints("min:1, max:15");
        setDescription("Kuwahara filter");
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

        cvu::KuwaharaFilter(src, dst, _radius);
        return ExecutionStatus(EStatus::Ok);
    }

protected:
    TypedNodeProperty<int> _radius;
};

class GeneralizedKuwaharaFilterNodeType : public NodeType
{
public:
    GeneralizedKuwaharaFilterNodeType()
        : _radius(6)
        , _N(8)
        , _smoothing(0.3333f)
    {
        addInput("Image", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::Image);
        addProperty("Radius", _radius)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 20))
            .setUiHints("min:1, max:20");
        addProperty("N", _N)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(3, 8))
            .setUiHints("min:3, max:8");
        addProperty("Smoothing", _smoothing)
            .setValidator(make_validator<InclRangePropertyValidator<double>>(0.0, 1.0))
            .setUiHints("min:0.00, max:1.00");
        setDescription("Generalized Kuwahara filter");
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

        cvu::generalizedKuwaharaFilter(src, dst, _radius, _N, _smoothing);
        return ExecutionStatus(EStatus::Ok);
    }

protected:
    TypedNodeProperty<int> _radius;
    TypedNodeProperty<int> _N;
    TypedNodeProperty<float> _smoothing;
};

class AnisotropicKuwaharaFilterNodeType : public NodeType
{
public:
    AnisotropicKuwaharaFilterNodeType()
        : _radius(6)
        , _N(8)
        , _smoothing(0.3333f)
    {
        addInput("Image", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::Image);
        addProperty("Radius", _radius)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 20))
            .setUiHints("min:1, max:20");
        addProperty("N", _N)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(3, 8))
            .setUiHints("min:3, max:8");
        addProperty("Smoothing", _smoothing)
            .setValidator(make_validator<InclRangePropertyValidator<double>>(0.0, 1.0))
            .setUiHints("min:0.00, max:1.00");
        setDescription("Anisotropic Kuwahara filter");
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

        cvu::anisotropicKuwaharaFilter(src, dst, _radius, _N, _smoothing);
        return ExecutionStatus(EStatus::Ok);
    }

protected:
    TypedNodeProperty<int> _radius;
    TypedNodeProperty<int> _N;
    TypedNodeProperty<float> _smoothing;
};

void registerKuwaharaFilter(NodeSystem* nodeSystem)
{
    typedef DefaultNodeFactory<KuwaharaFilterNodeType> KuwaharaFilterFactory;
    typedef DefaultNodeFactory<GeneralizedKuwaharaFilterNodeType> GeneralizedKuwaharaFilterFactory;
    typedef DefaultNodeFactory<AnisotropicKuwaharaFilterNodeType> AnisotropicKuwaharaFilterFactory;

    nodeSystem->registerNodeType("Filters/Kuwahara filter", 
        std::unique_ptr<NodeFactory>(new KuwaharaFilterFactory()));
    nodeSystem->registerNodeType("Filters/Generalized Kuwahara filter", 
        std::unique_ptr<NodeFactory>(new GeneralizedKuwaharaFilterFactory()));
    nodeSystem->registerNodeType("Filters/Anisotropic Kuwahara filter",
        std::unique_ptr<NodeFactory>(new AnisotropicKuwaharaFilterFactory()));
}