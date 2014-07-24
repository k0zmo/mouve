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

#include <opencv2/features2d/features2d.hpp>
#include <opencv2/imgproc/imgproc.hpp>

enum class EColor
{
    AllRandom,
    Red,
    Green,
    Blue
};

cv::Scalar getColor(EColor color)
{
    switch(color)
    {
    // Order is BGR
    case EColor::AllRandom: return cv::Scalar::all(-1);
    case EColor::Red: return cv::Scalar(36, 28, 237);
    case EColor::Green: return cv::Scalar(76, 177, 34);
    case EColor::Blue: return cv::Scalar(244, 63, 72);
    }
    return cv::Scalar::all(-1);
}

cv::Scalar getAltColor(EColor color)
{
    switch(color)
    {
        // Order is BGR
    case EColor::AllRandom: return cv::Scalar::all(-1);
    case EColor::Red: return cv::Scalar(76, 177, 34);
    case EColor::Green: return cv::Scalar(36, 28, 237);
    case EColor::Blue: return cv::Scalar(0, 255, 242);
    }
    return cv::Scalar::all(-1);
}

EColor getColor(const cv::Scalar& scalar)
{
    if(scalar == cv::Scalar::all(-1))
        return EColor::AllRandom;
    else if(scalar == cv::Scalar(36, 28, 237))
        return EColor::Red;
    else if(scalar == cv::Scalar(76, 177, 34))
        return EColor::Green;
    else if(scalar == cv::Scalar(244, 63, 72))
        return EColor::Blue;
    else
        return EColor::AllRandom;
}

class DrawSomethingLinesNodeType : public NodeType
{
public:
    DrawSomethingLinesNodeType()
        : _ecolor(EColor::Blue)
        , _thickness(2)
        , _type(ELineType::Line_AA)
        , _color(getColor(_ecolor.cast<Enum>().cast<EColor>()))
        , _intLineType(CV_AA)
    {
        addInput("Image", ENodeFlowDataType::Image);
        addInput("Shapes", ENodeFlowDataType::Array);
        addOutput("Output", ENodeFlowDataType::ImageRgb);
        addProperty("Line color", _ecolor)
            .setObserver(make_observer<FuncObserver>([this](const NodeProperty& np) {
                _color = getColor(np.cast<Enum>().cast<EColor>());
            }))
            .setUiHints("item: Random, item: Red, item: Green, item: Blue");
        addProperty("Line thickness", _thickness)
            .setValidator(make_validator<InclRangePropertyValidator<int>>(1, 5))
            .setUiHints("step:1, min:1, max:5");
        addProperty("Line type", _type)
            .setObserver(make_observer<FuncObserver>([this](const NodeProperty& np) {
                _intLineType = intLineType(np.cast<Enum>().cast<ELineType>());
            }))
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
        if(imageSrc.empty())
            return ExecutionStatus(EStatus::Ok);

        // Do stuff
        if(imageSrc.channels() == 1)
            cvtColor(imageSrc, imageDst, CV_GRAY2BGR);
        else
            imageDst = imageSrc.clone();

        return executeImpl(circles, imageSrc, imageDst);
    }

protected:
    enum class ELineType
    {
        Line_4Connected = 4,
        Line_8Connected = 8,
        Line_AA         = CV_AA
    };

    int intLineType(ELineType type)
    {
        switch(type)
        {
        case ELineType::Line_4Connected: return 4;
        case ELineType::Line_8Connected: return 8;
        case ELineType::Line_AA: return CV_AA;
        default: return 8;
        }
    }

    TypedNodeProperty<EColor> _ecolor;
    TypedNodeProperty<int> _thickness;
    TypedNodeProperty<ELineType> _type;

    cv::Scalar _color;
    int _intLineType;

protected:
    virtual ExecutionStatus executeImpl(const cv::Mat& objects, 
        const cv::Mat& imageSrc, cv::Mat& imageDest) = 0;
};

class DrawLinesNodeType : public DrawSomethingLinesNodeType
{
public:
    ExecutionStatus executeImpl(const cv::Mat& objects, 
        const cv::Mat& imageSrc, cv::Mat& imageDest) override
    {
        cv::RNG& rng = cv::theRNG();
        bool isRandColor = _color == cv::Scalar::all(-1);

        for(int lineIdx = 0; lineIdx < objects.rows; ++lineIdx)
        {
            float rho = objects.at<float>(lineIdx, 0);
            float theta = objects.at<float>(lineIdx, 1);
            double cos_t = cos(theta);
            double sin_t = sin(theta);
            double x0 = rho*cos_t;
            double y0 = rho*sin_t;
            double alpha = sqrt(imageSrc.cols*imageSrc.cols + imageSrc.rows*imageSrc.rows);

            cv::Scalar color = isRandColor ? cv::Scalar(rng(256), rng(256), rng(256)) : _color;
            cv::Point pt1(cvRound(x0 + alpha*(-sin_t)), cvRound(y0 + alpha*cos_t));
            cv::Point pt2(cvRound(x0 - alpha*(-sin_t)), cvRound(y0 - alpha*cos_t));
            cv::line(imageDest, pt1, pt2, color, _thickness, _intLineType);
        }

        return ExecutionStatus(EStatus::Ok);
    }
};

class DrawCirclesNodeType : public DrawSomethingLinesNodeType
{
public:
    ExecutionStatus executeImpl(const cv::Mat& objects,
        const cv::Mat& /*imageSrc*/, cv::Mat& imageDest) override
    {
        cv::RNG& rng = cv::theRNG();
        bool isRandColor = _color == cv::Scalar::all(-1);

        for(int circleIdx = 0; circleIdx < objects.cols; ++circleIdx)
        {
            cv::Vec3f circle = objects.at<cv::Vec3f>(circleIdx);
            cv::Point center(cvRound(circle[0]), cvRound(circle[1]));
            int radius = cvRound(circle[2]);
            cv::Scalar color = isRandColor ? cv::Scalar(rng(256), rng(256), rng(256)) : _color;
            // draw the circle center
            cv::circle(imageDest, center, 3, color, _thickness, _intLineType);
            // draw the circle outline
            cv::circle(imageDest, center, radius, color, _thickness, _intLineType);
        }

        return ExecutionStatus(EStatus::Ok);
    }
};

class DrawKeypointsNodeType : public NodeType
{
public:
    DrawKeypointsNodeType()
        : _ecolor(EColor::AllRandom)
        , _richKeypoints(true)
        , _color(getColor(_ecolor.cast<Enum>().cast<EColor>()))
    {
        addInput("Keypoints", ENodeFlowDataType::Keypoints);
        addOutput("Output", ENodeFlowDataType::ImageRgb);
        addProperty("Keypoints color", _ecolor)
            .setObserver(make_observer<FuncObserver>([this](const NodeProperty& np) {
                _color = getColor(np.cast<Enum>().cast<EColor>());
            }))
            .setUiHints("item: Random, item: Red, item: Green, item: Blue");
        addProperty("Rich keypoints", _richKeypoints);
        setDescription("Draws keypoints.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const KeyPoints& kp = reader.readSocket(0).getKeypoints();
        // Acquire output sockets
        cv::Mat& imageDst = writer.acquireSocket(0).getImageRgb();

        // validate input
        if(kp.image.empty())
            return ExecutionStatus(EStatus::Ok);
#if 0
        int drawFlags = _richKeypoints
            ? cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS
            : cv::DrawMatchesFlags::DEFAULT;
        cv::drawKeypoints(kp.image, kp.kpoints, imageDst, _color, drawFlags);
#else
        cvtColor(kp.image, imageDst, CV_GRAY2BGR);

        if(kp.kpoints.empty())
            return ExecutionStatus(EStatus::Ok);

        cv::RNG& rng = cv::theRNG();
        bool isRandColor = _color == cv::Scalar::all(-1);

        for(const auto& it : kp.kpoints)
        {
            cv::Scalar color = isRandColor ? cv::Scalar(rng(256), rng(256), rng(256)) : _color;
            drawKeypoint(imageDst, it, color, _richKeypoints);
        }
#endif
        return ExecutionStatus(EStatus::Ok);
    }

private:
    void drawKeypoint(cv::Mat& img, 
        const cv::KeyPoint& p,
        const cv::Scalar& color,
        bool richKeypoint)
    {
        CV_Assert(!img.empty());

        if(richKeypoint)
        {
            /*
            float radius = p.size;
            float ori = p.angle * (float)CV_PI/180.f;
            float r1 = cvRound(p.pt.y);
            float c1 = cvRound(p.pt.x);
            float c2 = cvRound(radius * cos(ori)) + c1;
            float r2 = cvRound(radius * sin(ori)) + r1;

            cv::line(img, cv::Point(c1, r1), cv::Point(c2, r2), color, 1, CV_AA);
            cv::circle(img, cv::Point(c1, r1), cvRound(radius), color, 1, CV_AA);
            cv::circle(img, cv::Point(c1, r1), 2, color, 1, CV_AA);
            */

            float radius = p.size;
            float ori = p.angle * (float)CV_PI/180.f;
            float r1 = static_cast<float>(cvRound(p.pt.y));
            float c1 = static_cast<float>(cvRound(p.pt.x));

            cv::Point2f x1(-radius, -radius);
            cv::Point2f x2(+radius, -radius);
            cv::Point2f x3(-radius, +radius);
            cv::Point2f x4(+radius, +radius);

            float c = std::cos(ori);
            float s = std::sin(ori);

            auto transform = [=](const cv::Point2f& p)
            {
                return cv::Point2f(c*p.x - s*p.y + c1, s*p.x + c*p.y + r1);
            };

            x1 = transform(x1);
            x2 = transform(x2); 
            x3 = transform(x3);
            x4 = transform(x4);

            float c2 = cvRound(radius * c) + c1;
            float r2 = cvRound(radius * s) + r1;

            cv::line(img, cv::Point(static_cast<int>(c1), static_cast<int>(r1)), cv::Point(static_cast<int>(c2), static_cast<int>(r2)), color, 1, CV_AA);
            cv::line(img, x1, x2, color, 1, CV_AA);
            cv::line(img, x2, x4, color, 1, CV_AA);
            cv::line(img, x4, x3, color, 1, CV_AA);
            cv::line(img, x3, x1, color, 1, CV_AA);
        }
        else
        {
            cv::circle(img, cv::Point(static_cast<int>(p.pt.x), static_cast<int>(p.pt.y)), 3, color, 1, CV_AA);
        }
    }

private:
    TypedNodeProperty<EColor> _ecolor;
    TypedNodeProperty<bool> _richKeypoints;
    cv::Scalar _color;
};

class DrawMatchesNodeType : public NodeType
{
public:
    DrawMatchesNodeType()
        : _ecolor(EColor::AllRandom)
        , _color(getColor(_ecolor.cast<Enum>().cast<EColor>()))
        , _kpColor(cv::Scalar::all(-1))
    {
        addInput("Matches", ENodeFlowDataType::Matches);
        addOutput("Image", ENodeFlowDataType::ImageRgb);
        addProperty("Keypoints color", _ecolor)
            .setObserver(make_observer<FuncObserver>([this](const NodeProperty& np) {
                _color = getColor(np.cast<Enum>().cast<EColor>());
                _kpColor = getAltColor(np.cast<Enum>().cast<EColor>());
             }))
            .setUiHints("item: Random, item: Red, item: Green, item: Blue");
        setDescription("Draws matches.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const Matches& mt = reader.readSocket(0).getMatches();
        // Acquire output sockets
        cv::Mat& imageMatches = writer.acquireSocket(0).getImageRgb();

        // Validate inputs
        if(mt.queryImage.empty() || mt.trainImage.empty())
            return ExecutionStatus(EStatus::Ok);

        if(mt.queryPoints.size() != mt.trainPoints.size())
            return ExecutionStatus(EStatus::Error, 
                "Points from one images doesn't correspond to key points in another one");

        imageMatches = drawMatchesOnImage(mt);
        return ExecutionStatus(EStatus::Ok);
    }

private:
    cv::Mat drawMatchesOnImage(const Matches& mt)
    {
        // mt.queryImage.type() == mt.trainImage.type() for now
        int cols = mt.queryImage.cols + mt.trainImage.cols;
        int rows = std::max(mt.queryImage.rows, mt.trainImage.rows);
        cv::Mat matDraw(rows, cols, mt.queryImage.type(), cv::Scalar(0));

        cv::Rect roi1(0, 0, mt.queryImage.cols, mt.queryImage.rows);
        cv::Rect roi2(mt.queryImage.cols, 0, mt.trainImage.cols, mt.trainImage.rows);

        mt.queryImage.copyTo(matDraw(roi1));
        mt.trainImage.copyTo(matDraw(roi2));
        if(matDraw.channels() == 1)
            cvtColor(matDraw, matDraw, cv::COLOR_GRAY2BGR);

        cv::Mat matDraw1 = matDraw(roi1);
        cv::Mat matDraw2 = matDraw(roi2);
        cv::RNG& rng = cv::theRNG();
        bool isRandColor = _color == cv::Scalar::all(-1);

        for(size_t i = 0; i < mt.queryPoints.size(); ++i)
        {
            const auto& kp1 = mt.queryPoints[i];
            const auto& kp2 = mt.trainPoints[i];

            cv::Scalar color = isRandColor ? cv::Scalar(rng(256), rng(256), rng(256)) : _color;
            cv::Scalar kpColor = isRandColor ? color : _kpColor;
            
            auto drawKeypoint = [](cv::Mat& matDraw, const cv::Point2f& pt, 
                const cv::Scalar& color)
            {
                int x = cvRound(pt.x);
                int y = cvRound(pt.y);
                int s = 10;

                cv::circle(matDraw, cv::Point(x, y), s, color, 1, CV_AA);
            };

            drawKeypoint(matDraw1, kp1, kpColor);
            drawKeypoint(matDraw2, kp2, kpColor);

            int x1 = cvRound(kp1.x) + roi1.tl().x;
            int y1 = cvRound(kp1.y) + roi1.tl().y;
            int x2 = cvRound(kp2.x) + roi2.tl().x;
            int y2 = cvRound(kp2.y) + roi2.tl().y;
            cv::line(matDraw, cv::Point(x1, y1), cv::Point(x2, y2), color, 1, CV_AA);
        }

        return matDraw;
    }

private:
    TypedNodeProperty<EColor> _ecolor;
    cv::Scalar _color;
    cv::Scalar _kpColor;
};

class DrawHomographyNodeType : public NodeType
{
public:
    DrawHomographyNodeType()
    {
        addInput("Homography", ENodeFlowDataType::Array);
        addInput("Matches", ENodeFlowDataType::Matches);
        addOutput("Output", ENodeFlowDataType::ImageRgb);
        setDescription("Draws homography.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        //inputs
        const cv::Mat& homography = reader.readSocket(0).getArray();
        // in fact we need only one image and size of another but this solution is way cleaner
        const Matches& mt = reader.readSocket(1).getMatches();
        // Acquire output sockets
        cv::Mat& img_matches = writer.acquireSocket(0).getImageRgb();

        // Validate inputs
        if(mt.queryImage.empty() || mt.trainImage.empty() || homography.empty())
            return ExecutionStatus(EStatus::Ok);

        std::vector<cv::Point2f> obj_corners(4);
        obj_corners[0] = cvPoint(0,0);
        obj_corners[1] = cvPoint(mt.queryImage.cols, 0);
        obj_corners[2] = cvPoint(mt.queryImage.cols, mt.queryImage.rows);
        obj_corners[3] = cvPoint(0, mt.queryImage.rows);

        std::vector<cv::Point2f> scene_corners(4);

        cv::perspectiveTransform(obj_corners, scene_corners, homography);

        //img_matches = img_scene.clone();
        cvtColor(mt.trainImage, img_matches, CV_GRAY2BGR);

        cv::line(img_matches, scene_corners[0], scene_corners[1], cv::Scalar(255, 0, 0), 4);
        cv::line(img_matches, scene_corners[1], scene_corners[2], cv::Scalar(255, 0, 0), 4);
        cv::line(img_matches, scene_corners[2], scene_corners[3], cv::Scalar(255, 0, 0), 4);
        cv::line(img_matches, scene_corners[3], scene_corners[0], cv::Scalar(255, 0, 0), 4);

        return ExecutionStatus(EStatus::Ok);
    }
};

class PaintMaskNodeType : public NodeType
{
public:
    PaintMaskNodeType()
        : _ecolor(EColor::Green)
        , _color(getColor(_ecolor.cast<Enum>().cast<EColor>()))
    {
        addInput("Mask", ENodeFlowDataType::ImageMono);
        addInput("Source", ENodeFlowDataType::ImageMono);
        addOutput("Image", ENodeFlowDataType::ImageRgb);
        addProperty("Mask color", _ecolor)
            .setObserver(make_observer<FuncObserver>([this](const NodeProperty& np) {
                _color = getColor(np.cast<Enum>().cast<EColor>());
            }))
            .setUiHints("item: Random, item: Red, item: Green, item: Blue");
        setDescription("Paints image using binary mask.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const cv::Mat& mask = reader.readSocket(0).getImageMono();
        const cv::Mat& source = reader.readSocket(1).getImageMono();
        // Acquire output sockets
        cv::Mat& imagePainted = writer.acquireSocket(0).getImageRgb();

        // Validate inputs
        if(mask.empty() || source.empty())
            return ExecutionStatus(EStatus::Ok);

        if(mask.size() != source.size())
            return ExecutionStatus(EStatus::Error, 
            "Mask must be the same size as source image");

        // Do stuff
        cv::Vec3b tmp(
            cv::saturate_cast<uchar>(_color[0]), 
            cv::saturate_cast<uchar>(_color[1]), 
            cv::saturate_cast<uchar>(_color[2]));
        cv::RNG& rng = cv::theRNG();
        cv::Vec3b color = (_color == cv::Scalar::all(-1)) 
            ? cv::Vec3b(rng(256), rng(256), rng(256))
            : tmp;

        cv::cvtColor(source, imagePainted, CV_GRAY2BGR);

        for(int y = 0; y < source.rows; ++y)
        {
            const uchar* mask_ptr = mask.ptr<uchar>(y);
            cv::Vec3b* dst_ptr = imagePainted.ptr<cv::Vec3b>(y);

            for(int x = 0; x < source.cols; ++x)
            {
                if(mask_ptr[x] != 0)
                    dst_ptr[x] = color;
            }
        }
        
        return ExecutionStatus(EStatus::Ok);
    }

private:
    TypedNodeProperty<EColor> _ecolor;
    cv::Scalar _color;
};

REGISTER_NODE("Draw features/Paint mask", PaintMaskNodeType)
REGISTER_NODE("Draw features/Draw homography", DrawHomographyNodeType)
REGISTER_NODE("Draw features/Draw matches", DrawMatchesNodeType)
REGISTER_NODE("Draw features/Draw keypoints", DrawKeypointsNodeType)
REGISTER_NODE("Draw features/Draw circles", DrawCirclesNodeType)
REGISTER_NODE("Draw features/Draw lines", DrawLinesNodeType)
