/*
 * Copyright (c) 2013-2018 Kajetan Swierk <k0zmo@outlook.com>
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

#include <opencv2/video/video.hpp>

class AdaptiveMixtureOfGaussiansNodeType : public NodeType
{
public:
    AdaptiveMixtureOfGaussiansNodeType()
        : _history(200), _varThreshold(16.0f), _shadowDetection(true), _learningRate(-1)
    {
        addInput("Input", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::ImageMono);
        addProperty("History frames", _history)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 500))
            .setUiHints("min:1, max:500");
        addProperty("Mahalanobis distance threshold", _varThreshold);
        addProperty("Detect shadow", _shadowDetection);
        addProperty("Learning rate", _learningRate)
            .setValidator(make_validator<InclRangePropertyValidator<double>>(-1.0, 1.0))
            .setUiHints("min:-1, max:1, step:0.01, decimals:3");
        setDescription("Improved adaptive Gausian mixture model for background subtraction.");
        setFlags(kl::make_flags(ENodeConfig::HasState));
    }

    bool restart() override
    {
        _mog2 = cv::BackgroundSubtractorMOG2(_history, _varThreshold, _shadowDetection);
        return true;
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& source = reader.readSocket(0).getImage();
        // Acquire output sockets
        cv::Mat& output = writer.acquireSocket(0).getImageMono();

        // Validate inputs
        if (source.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff - single step
        _mog2(source, output, _learningRate);

        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<int> _history;
    TypedNodeProperty<float> _varThreshold;
    TypedNodeProperty<bool> _shadowDetection;
    TypedNodeProperty<double> _learningRate;
    cv::BackgroundSubtractorMOG2 _mog2;
};

void registerAdaptiveMOG(NodeSystem& system)
{
    system.registerNodeType("Video segmentation/Adaptive mixture of Gaussians",
                            makeDefaultNodeFactory<AdaptiveMixtureOfGaussiansNodeType>());
}
