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

#include "Kommon/StringUtils.h"

class AmpMorphologyOperatorNodeType : public NodeType
{
public:
    AmpMorphologyOperatorNodeType()
    {
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImageMono();
        const cv::Mat& se = reader.readSocket(1).getImageMono();
        // Acquire output sockets
        cv::Mat& dst = writer.acquireSocket(0).getImageMono();

        // Validate inputs
        if(se.empty() || src.empty())
            return ExecutionStatus(EStatus::Ok);

        using namespace concurrency;
        using namespace concurrency::graphics;

        // Prepare structuring element
        std::vector<index<2>> coords = structuringElementCoords(se);
        array_view<index<2>> coordsAv(static_cast<int>(coords.size()), coords);
        uint coordsSize = static_cast<uint>(coords.size());

        extent<2> image_extent(src.rows, src.cols);
        unsigned int sizeInBytes = static_cast<unsigned int>(src.cols * src.rows * sizeof(uchar) * src.channels());

        texture<uint, 2> srcTx(image_extent, const_cast<const uchar*>(src.data), sizeInBytes, 8U);
        texture<uint, 2> dstTx(image_extent, 8U);
        writeonly_texture_view<uint, 2> dstTv(dstTx); // deprecated in VS2013 in favour of texture_view

        parallel_for_each(image_extent, 
            [&srcTx, dstTv, coordsAv, coordsSize]
            (index<2> idx) restrict(amp)
        {
            uint maxv = 0;

            for(uint i = 0; i < coordsSize; ++i)
            {
                index<2> coord = coordsAv[i] + idx;
                uint pix = srcTx[coord];
                maxv = concurrency::direct3d::imax(maxv, pix);
            }

            dstTv.set(idx, maxv);
        });

        dst.create(src.size(), src.type());
        copy(dstTx, dst.data, sizeInBytes);

        return ExecutionStatus(EStatus::Ok);
    }

    void configuration(NodeConfig& nodeConfig) const override
    {
        static const InputSocketConfig in_config[] = {
            { ENodeFlowDataType::Image, "source", "Source", "" },
            { ENodeFlowDataType::Image, "source", "Structuring element", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };
        static const OutputSocketConfig out_config[] = {
            { ENodeFlowDataType::Image, "output", "Output", "" },
            { ENodeFlowDataType::Invalid, "", "", "" }
        };

        nodeConfig.description = "Performs morphological operation on a given image.";
        nodeConfig.pInputSockets = in_config;
        nodeConfig.pOutputSockets = out_config;
    }

private:
    std::vector<concurrency::index<2>> structuringElementCoords(const cv::Mat& sElem)
    {
        std::vector<concurrency::index<2>> coords;
        int seRadiusX = (sElem.cols - 1) / 2;
        int seRadiusY = (sElem.rows - 1) / 2;
        for(int y = 0; y < sElem.rows; ++y)
        {
            const uchar* krow = sElem.ptr<uchar>(y);;
            for(int x = 0; x < sElem.cols; ++x)
            {
                if(krow[x] == 0)
                    continue;
                concurrency::index<2> c(x - seRadiusX, y - seRadiusY);
                coords.push_back(c);
            }
        }
        return coords;
    }
};

void registerAmpMorphologyNodes(NodeSystem* nodeSystem)
{
    typedef DefaultNodeFactory<AmpMorphologyOperatorNodeType> AmpMorphologyOperatorFactory;

    nodeSystem->registerNodeType("Morphology/Operator AMP",
        std::unique_ptr<NodeFactory>(new AmpMorphologyOperatorFactory()));
}

#else

void registerAmpMorphologyNodes(NodeSystem*)
{
}

#endif