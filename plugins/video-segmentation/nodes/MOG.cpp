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
#include "Logic/NodeFactory.h"

#include <opencv2/video/video.hpp>

class MixtureOfGaussiansNodeType : public NodeType
{
public:
    MixtureOfGaussiansNodeType()
        : _history(200), _nmixtures(5), _backgroundRatio(0.7), _learningRate(-1)
    {
        addInput("Input", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::ImageMono);
        addProperty("History frames", _history)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 500))
            .setUiHints("min:1, max:500");
        addProperty("Number of mixtures", _nmixtures)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 9))
            .setUiHints("min:1, max:9");
        addProperty("Background ratio", _backgroundRatio)
            .setValidator(make_validator<ExclRangePropertyValidator<double>>(0.0, 1.0))
            .setUiHints("min:0.01, max:0.99, step:0.01");
        addProperty("Learning rate", _learningRate)
            .setValidator(make_validator<InclRangePropertyValidator<double>>(-1.0, 1.0))
            .setUiHints("min:-1, max:1, step:0.01, decimals:3");
        setDescription("Gaussian Mixture-based image sequence background/foreground segmentation.");
        setFlags(kl::make_flags(ENodeConfig::HasState));
    }

    bool restart() override
    {
        _mog = cv::BackgroundSubtractorMOG(_history, _nmixtures, _backgroundRatio);
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
        _mog(source, output, _learningRate);

        return ExecutionStatus(EStatus::Ok);
    }

private:
    cv::BackgroundSubtractorMOG _mog;
    TypedNodeProperty<int> _history;
    TypedNodeProperty<int> _nmixtures;
    TypedNodeProperty<double> _backgroundRatio;
    TypedNodeProperty<double> _learningRate;
};

REGISTER_NODE("Video segmentation/Mixture of Gaussians", MixtureOfGaussiansNodeType);
