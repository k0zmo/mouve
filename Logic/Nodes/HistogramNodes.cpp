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

#include <opencv2/imgproc/imgproc.hpp>

class EqualizeHistogramNodeType : public NodeType
{
public:
    EqualizeHistogramNodeType()
    {
        addInput("Source", ENodeFlowDataType::ImageMono);
        addOutput("Output", ENodeFlowDataType::ImageMono);
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        const cv::Mat& src = reader.readSocket(0).getImageMono();
        cv::Mat& dst = writer.acquireSocket(0).getImageMono();

        if(src.rows == 0 || src.cols == 0)
            return ExecutionStatus(EStatus::Ok);

        cv::equalizeHist(src, dst);

        return ExecutionStatus(EStatus::Ok);
    }
};

class ClaheNodeType : public NodeType
{
public:
    ClaheNodeType()
        : _clipLimit(2.0)
        , _tilesGridSize(8)
    {
        addInput("Source", ENodeFlowDataType::ImageMono);
        addOutput("Output", ENodeFlowDataType::ImageMono);
        addProperty("Clip limit", _clipLimit)
            .setValidator(make_validator<MinPropertyValidator<double>>(0.0))
            .setUiHints("min:0.0");
        addProperty("Tiles size", _tilesGridSize)
            .setValidator(make_validator<MinPropertyValidator<int>>(1))
            .setUiHints("min:1");
        setDescription("Performs CLAHE (Contrast Limited Adaptive Histogram Equalization)");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        const cv::Mat& src = reader.readSocket(0).getImageMono();
        cv::Mat& dst = writer.acquireSocket(0).getImageMono();

        cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
        clahe->setClipLimit(_clipLimit);
        clahe->setTilesGridSize(cv::Size(_tilesGridSize, _tilesGridSize));

        clahe->apply(src, dst);

        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<double> _clipLimit;
    TypedNodeProperty<int> _tilesGridSize;
};

class DrawHistogramNodeType : public NodeType
{
public:
    DrawHistogramNodeType()
    {
        addInput("Source", ENodeFlowDataType::Image);
        addOutput("Output", ENodeFlowDataType::ImageRgb);
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& src = reader.readSocket(0).getImage();
        // Acquire output sockets
        cv::Mat& canvas = writer.acquireSocket(0).getImageRgb();

        // Validate inputs
        if(src.empty())
            return ExecutionStatus(EStatus::Ok);

        if(src.channels() == 1)
        {
            // Compute histogram
            const int bins = 256;
            const float range[] = { 0, 256 } ;
            const float* histRange = { range };
            const bool uniform = true; 
            const bool accumulate = false;
            cv::Mat hist;
            cv::calcHist(&src, 1, 0, cv::noArray(), hist, 1, &bins, &histRange, uniform, accumulate);

            // Draw histogram
            const int hist_h = 125;
            canvas = cv::Mat::zeros(hist_h, bins, CV_8UC3);
            cv::normalize(hist, hist, 0, canvas.rows, cv::NORM_MINMAX, -1, cv::noArray());

            for(int j = 0; j < bins-1; ++j)
            {
                cv::line(canvas, 
                    cv::Point(j, hist_h),
                    cv::Point(j, hist_h - cvRound(hist.at<float>(j))),
                    cv::Scalar(200,200,200),
                    1, 8, 0);
            }
        }
        else if(src.channels() == 3)
        {
            // Quantization
            const int bins = 256;
            // Range of given channel (0-255 for BGR)
            const float range[] = { 0, 256 } ;
            const float* histRange = { range };
            const bool uniform = true; 
            const bool accumulate = false;

            cv::Mat bhist, ghist, rhist;
            int bchannels[] = {0}, gchannels[] = {1}, rchannels[] = {2};

            cv::calcHist(&src, 1, bchannels, cv::noArray(), bhist, 1, &bins, &histRange, uniform, accumulate);
            cv::calcHist(&src, 1, gchannels, cv::noArray(), ghist, 1, &bins, &histRange, uniform, accumulate);
            cv::calcHist(&src, 1, rchannels, cv::noArray(), rhist, 1, &bins, &histRange, uniform, accumulate);

            // Draw histogram
            const int hist_h = 125;
            canvas = cv::Mat::zeros(hist_h, bins, CV_8UC3);
            cv::normalize(bhist, bhist, 0, canvas.rows, cv::NORM_MINMAX, -1, cv::noArray());
            cv::normalize(ghist, ghist, 0, canvas.rows, cv::NORM_MINMAX, -1, cv::noArray());
            cv::normalize(rhist, rhist, 0, canvas.rows, cv::NORM_MINMAX, -1, cv::noArray());

            for(int j = 1; j < bins; ++j)
            {
                cv::line(canvas, 
                    cv::Point(j-1, hist_h - cvRound(bhist.at<float>(j-1))),
                    cv::Point(j,   hist_h - cvRound(bhist.at<float>(j))), 
                    cv::Scalar(255, 0, 0), 
                    1, 8, 0);
                cv::line(canvas, 
                    cv::Point(j-1, hist_h - cvRound(ghist.at<float>(j-1))),
                    cv::Point(j,   hist_h - cvRound(ghist.at<float>(j))), 
                    cv::Scalar(0, 255, 0), 
                    1, 8, 0);
                cv::line(canvas, 
                    cv::Point(j-1, hist_h - cvRound(rhist.at<float>(j-1))),
                    cv::Point(j,   hist_h - cvRound(rhist.at<float>(j))), 
                    cv::Scalar(0, 0, 255), 
                    1, 8, 0);
            }
        }

        return ExecutionStatus(EStatus::Ok);
    }
};

REGISTER_NODE("Histogram/CLAHE", ClaheNodeType)
REGISTER_NODE("Histogram/Equalize histogram", EqualizeHistogramNodeType)
REGISTER_NODE("Histogram/Draw histogram", DrawHistogramNodeType)
