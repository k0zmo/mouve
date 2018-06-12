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

#include "Prerequisites.h"

#if defined(CPP_AMP_SUPPORTED)

#include "Logic/NodePlugin.h"
#include "Logic/NodeSystem.h"
#include "Logic/NodeType.h"

class AmpAddNodeType : public NodeType
{
public:
    AmpAddNodeType()
        : _alpha(0.5)
        , _beta(0.5)
    {
        addInput("X", ENodeFlowDataType::ImageMono);
        addInput("Y", ENodeFlowDataType::ImageMono);
        addOutput("Output", ENodeFlowDataType::ImageMono);
        addProperty("Alpha", _alpha)
            .setValidator(make_validator<InclRangePropertyValidator<double>>(0.0, 1.0))
            .setUiHints("min:0.0, max:1.0, decimals:3, step:0.1");
        addProperty("Beta", _beta)
            .setValidator(make_validator<InclRangePropertyValidator<double>>(0.0, 1.0))
            .setUiHints("min:0.0, max:1.0, decimals:3, step:0.1");
        setDescription("Adds one image to another with specified weights: alpha * x + beta * y.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src1 = reader.readSocket(0).getImageMono();
        const cv::Mat& src2 = reader.readSocket(1).getImageMono();
        // Acquire output sockets
        cv::Mat& dst = writer.acquireSocket(0).getImageMono();

        // Validate inputs
        if(src1.empty() || src2.empty())
            return ExecutionStatus(EStatus::Ok);
        if(src1.size() != src2.size())
            return ExecutionStatus(EStatus::Error, "Inputs with different sizes");
        if(src1.channels() != src2.channels())
            return ExecutionStatus(EStatus::Error, "Inputs with different number of channels");
        if(src1.type() != src2.type() && src1.type() != CV_8UC1)
            return ExecutionStatus(EStatus::Error, "Inputs' format not supported");

        using namespace concurrency;
        using namespace concurrency::graphics;

        // assumues images are same the same size and format 
        extent<2> image_extent(src1.rows, src1.cols);
        unsigned int sizeInBytes = static_cast<unsigned int>(src1.cols * src1.rows * sizeof(uchar) * src1.channels());

        texture<uint, 2> src1Tx(image_extent, const_cast<const uchar*>(src1.data), sizeInBytes, 8U);
        texture<uint, 2> src2Tx(image_extent, const_cast<const uchar*>(src2.data), sizeInBytes, 8U);
        texture<uint, 2> dstTx(image_extent, 8U);
        texture_view<uint, 2> dstTv(dstTx);

        float alpha = static_cast<float>(_alpha);
        float beta  = static_cast<float>(_beta);

        parallel_for_each(image_extent, 
            [&src1Tx, &src2Tx, dstTv, alpha, beta]
            (index<2> idx) restrict(amp)
        {
            uint x = src1Tx[idx];
            uint y = src2Tx[idx];

            uint z = static_cast<uint>((x * alpha) + (y * beta));
            dstTv.set(idx, z);
        });

        dst.create(src1.size(), src1.type());
        copy(dstTx, dst.data, sizeInBytes);

        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<double> _alpha;
    TypedNodeProperty<double> _beta;
};

void registerAmpArithmeticNodes(NodeSystem& system)
{
    system.registerNodeType("Arithmetic/Add AMP", makeDefaultNodeFactory<AmpAddNodeType>());
}

#else

void registerAmpArithmeticNodes(class NodeSystem&)
{
}

#endif
