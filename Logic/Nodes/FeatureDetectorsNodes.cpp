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
#include "Logic/NodeFactory.h"
#include "Kommon/StringUtils.h"

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/features2d/features2d.hpp>

using std::vector;

class FastFeatureDetector : public NodeType
{
public:
    FastFeatureDetector()
        : _threshold(10)
        , _type(cv::FastFeatureDetector::TYPE_9_16)
        , _nonmaxSupression(true)
    {
        addInput("Image", ENodeFlowDataType::ImageMono);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        addProperty("Threshold", _threshold)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(0, 255))
            .setUiHints("min:0, max:255");
        addProperty("Nonmax supression", _nonmaxSupression);
        addProperty("Neighborhoods type", _type)
            .setUiHints("item: 5_8, item: 7_12, item: 9_16");
        setDescription("Detects corners using the FAST algorithm.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImageMono();
        // Acquire output sockets
        KeyPoints& kp = writer.acquireSocket(0).getKeypoints();

        // Validate inputs
        if(src.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        cv::FASTX(src, kp.kpoints, _threshold, _nonmaxSupression, _type.cast<Enum>().data());
        kp.image = src;

        return ExecutionStatus(EStatus::Ok, 
            string_format("Keypoints detected: %d", (int) kp.kpoints.size()));
    }

private:
    TypedNodeProperty<int> _threshold;
    TypedNodeProperty<Enum> _type;
    TypedNodeProperty<bool> _nonmaxSupression;
};

class MserSalientRegionDetectorNodeType : public NodeType
{
public:
    MserSalientRegionDetectorNodeType()
    {
        addInput("Image", ENodeFlowDataType::ImageMono);
        addOutput("Output", ENodeFlowDataType::ImageRgb);
        setDescription("Maximally stable extremal region extractor.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImageMono();
        // Acquire output sockets
        cv::Mat& img = writer.acquireSocket(0).getImageRgb();

        // Validate inputs
        if(src.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        cvtColor(src, img, CV_GRAY2BGR);

        cv::MserFeatureDetector mser;
        vector<vector<cv::Point>> msers;
        mser(src, msers);

        for(int i = (int)msers.size()-1; i >= 0; i--)
        {
            // fit ellipse
            cv::RotatedRect box = cv::fitEllipse(msers[i]);
            box.angle = (float)CV_PI/2 - box.angle;
            cv::ellipse(img, box, cv::Scalar(196,255,255), 1, CV_AA);
        }		

        return ExecutionStatus(EStatus::Ok);
    }
};

class StarFeatureDetectorNodeType : public NodeType
{
public:
    StarFeatureDetectorNodeType()
    {
        addInput("Image", ENodeFlowDataType::ImageMono);
        addOutput("Keypoints", ENodeFlowDataType::Keypoints);
        setDescription("CenSurE keypoint detector.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImageMono();
        // Acquire output sockets
        KeyPoints& kp = writer.acquireSocket(0).getKeypoints();

        // Validate inputs
        if(src.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        cv::StarFeatureDetector star;
        star(src, kp.kpoints);
        kp.image = src;

        return ExecutionStatus(EStatus::Ok, 
            string_format("Keypoints detected: %d", (int) kp.kpoints.size()));
    }
};

REGISTER_NODE("Features/STAR", StarFeatureDetectorNodeType)
REGISTER_NODE("Features/MSER", MserSalientRegionDetectorNodeType)
REGISTER_NODE("Features/FAST", FastFeatureDetector)
