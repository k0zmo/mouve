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

#include <opencv2/imgproc/imgproc.hpp>
#include <fmt/core.h>

class ShiTomasiCornerDetectorNodeType : public NodeType
{
public:
    ShiTomasiCornerDetectorNodeType() : _maxCorners(100), _qualityLevel(0.01), _minDistance(10)
    {
        addInput("Image", ENodeFlowDataType::ImageMono);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addProperty("Max corners", _maxCorners)
            .setValidator(make_validator<MinPropertyValidator<int>>(0))
            .setUiHints("min:0");
        addProperty("Quality level of octaves", _qualityLevel)
            .setValidator(make_validator<MinPropertyValidator<double>>(0))
            .setUiHints("min:0");
        addProperty("Minimum distance", _minDistance)
            .setValidator(make_validator<MinPropertyValidator<double>>(0))
            .setUiHints("min:0");
        setDescription("Shi-Tomasi corner detector");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // inputs
        const cv::Mat& src = reader.readSocket(0).getImageMono();
        // outputs
        KeyPoints& kp = writer.acquireSocket(0).getKeypoints();

        // validate inputs
        if (src.empty())
            return ExecutionStatus(EStatus::Ok);

        std::vector<cv::Point2f> corners;
        int blockSize = 3;
        bool useHarrisDetector = false;
        double k = 0.04;

        cv::goodFeaturesToTrack(src, corners, _maxCorners, _qualityLevel, _minDistance,
                                cv::noArray(), blockSize, useHarrisDetector, k);

        for (const auto& corner : corners)
            kp.kpoints.emplace_back(corner, 8.0f);
        kp.image = src;

        return ExecutionStatus(EStatus::Ok, fmt::format("Corners detected: {}", kp.kpoints.size()));
    }

private:
    TypedNodeProperty<int> _maxCorners;
    TypedNodeProperty<double> _qualityLevel;
    TypedNodeProperty<double> _minDistance;
};

REGISTER_NODE("Features/Shi-Tomasi corner detector", ShiTomasiCornerDetectorNodeType);
