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

#include "impl/Drawing.h"

#include <opencv2/imgproc/imgproc.hpp>

template <typename Impl>
class DrawNodeType : public NodeType
{
public:
    DrawNodeType() : _ecolor(EColor::Blue), _thickness(2), _type(ELineType::Line_AA)
    {
        addInput("Image", ENodeFlowDataType::Image);
        addInput("Shapes", ENodeFlowDataType::Array);
        addOutput("Output", ENodeFlowDataType::ImageRgb);
        addProperty("Line color", _ecolor)
            .setUiHints("item: Random, item: Red, item: Green, item: Blue");
        addProperty("Line thickness", _thickness)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 5))
            .setUiHints("step:1, min:1, max:5");
        addProperty("Line type", _type)
            .setUiHints("item: 4-connected, item: 8-connected, item: AA");
        setDescription("Draws simple geometric shapes.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& imageSrc = reader.readSocket(0).getImage();
        const cv::Mat& circles = reader.readSocket(1).getArray();
        // ouputs
        cv::Mat& imageDst = writer.acquireSocket(0).getImageRgb();

        // Validate inputs
        if (imageSrc.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        if (imageSrc.channels() == 1)
            cvtColor(imageSrc, imageDst, CV_GRAY2BGR);
        else
            imageDst = imageSrc.clone();

        return static_cast<Impl*>(this)->executeImpl(circles, imageSrc, imageDst);
    }

protected:
    TypedNodeProperty<EColor> _ecolor;
    TypedNodeProperty<int> _thickness;
    TypedNodeProperty<ELineType> _type;
};

class DrawLinesNodeType : public DrawNodeType<DrawLinesNodeType>
{
    friend class DrawNodeType<DrawLinesNodeType>;

private:
    ExecutionStatus executeImpl(const cv::Mat& objects, const cv::Mat& imageSrc, cv::Mat& imageDest)
    {
        cv::RNG& rng = cv::theRNG();
        const EColor colorName = _ecolor.cast<Enum>().cast<EColor>();
        const int lineType = static_cast<int>(_type.cast<Enum>().cast<ELineType>());

        for (int lineIdx = 0; lineIdx < objects.rows; ++lineIdx)
        {
            float rho = objects.at<float>(lineIdx, 0);
            float theta = objects.at<float>(lineIdx, 1);
            double cos_t = cos(theta);
            double sin_t = sin(theta);
            double x0 = rho * cos_t;
            double y0 = rho * sin_t;
            double alpha = sqrt(imageSrc.cols * imageSrc.cols + imageSrc.rows * imageSrc.rows);

            const cv::Scalar color =
                colorName == EColor::AllRandom ? getRandomColor(rng) : getColor(colorName);
            cv::Point pt1(cvRound(x0 + alpha * (-sin_t)), cvRound(y0 + alpha * cos_t));
            cv::Point pt2(cvRound(x0 - alpha * (-sin_t)), cvRound(y0 - alpha * cos_t));
            cv::line(imageDest, pt1, pt2, color, _thickness, lineType);
        }

        return ExecutionStatus(EStatus::Ok);
    }
};

class DrawCirclesNodeType : public DrawNodeType<DrawLinesNodeType>
{
    friend class DrawNodeType<DrawLinesNodeType>;

private:
    ExecutionStatus executeImpl(const cv::Mat& objects, const cv::Mat& /*imageSrc*/,
                                cv::Mat& imageDest)
    {
        cv::RNG& rng = cv::theRNG();
        const EColor colorName = _ecolor.cast<Enum>().cast<EColor>();
        const int lineType = static_cast<int>(_type.cast<Enum>().cast<ELineType>());

        for (int circleIdx = 0; circleIdx < objects.cols; ++circleIdx)
        {
            cv::Vec3f circle = objects.at<cv::Vec3f>(circleIdx);
            cv::Point center(cvRound(circle[0]), cvRound(circle[1]));
            int radius = cvRound(circle[2]);
            const cv::Scalar color =
                colorName == EColor::AllRandom ? getRandomColor(rng) : getColor(colorName);
            // draw the circle center
            cv::circle(imageDest, center, 3, color, _thickness, lineType);
            // draw the circle outline
            cv::circle(imageDest, center, radius, color, _thickness, lineType);
        }

        return ExecutionStatus(EStatus::Ok);
    }
};

REGISTER_NODE("Draw features/Draw circles", DrawCirclesNodeType)
REGISTER_NODE("Draw features/Draw lines", DrawLinesNodeType)
