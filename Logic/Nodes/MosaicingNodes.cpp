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

class SimpleMosaicNodeType : public NodeType
{
public:
    SimpleMosaicNodeType()
    {
        addInput("Homography", ENodeFlowDataType::Array);
        addInput("Matches", ENodeFlowDataType::Matches);
        addOutput("Mosaic", ENodeFlowDataType::ImageRgb);
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        //inputs
        const cv::Mat& homography = reader.readSocket(0).getArray();
        const Matches& mt = reader.readSocket(1).getMatches();
        // output
        cv::Mat& mosaic = writer.acquireSocket(0).getImageRgb();

        // Validate inputs
        if(mt.queryImage.empty() || mt.trainImage.empty() || homography.empty())
            return ExecutionStatus(EStatus::Ok);

        planarWarper_warp(mt, homography);
        cv::Rect roi = simpleBlender_findRoi();

        cv::Mat dst32, maskDest;
        dst32.create(roi.size(), CV_32SC1);
        dst32.setTo(cv::Scalar::all(0));
        maskDest.create(roi.size(), CV_8U);
        maskDest.setTo(cv::Scalar::all(0));

        simpleBlender_feed(1, roi, dst32, maskDest);
        simpleBlender_feed(0, roi, dst32, maskDest);

        dst32.convertTo(mosaic, CV_8UC1);

        imgWarped[0] = cv::Mat(); imgWarped[1] = cv::Mat();
        maskWarped[0] = cv::Mat(); maskWarped[1] = cv::Mat();

        return ExecutionStatus(EStatus::Ok);
    }

private:
    cv::Matx<double, 3, 3> planarWarper_findCorners(const Matches& mt, const cv::Mat& homography) 
    {
        vector<cv::Point2f> corners_2f;
        corners_2f.push_back(cv::Point2f(0.0f,0.0f));
        corners_2f.push_back(cv::Point2f(static_cast<float>(mt.queryImage.cols),0.0f));
        corners_2f.push_back(cv::Point2f(0.0f,static_cast<float>(mt.queryImage.rows)));
        corners_2f.push_back(cv::Point2f(static_cast<float>(mt.queryImage.cols),static_cast<float>(mt.queryImage.rows)));

        cv::perspectiveTransform(corners_2f, corners_2f, homography);

        float xmin = std::numeric_limits<float>::max();
        float ymin = std::numeric_limits<float>::max();
        float xmax = -std::numeric_limits<float>::max();
        float ymax = -std::numeric_limits<float>::max();

        for (int i = 0; i < 4; ++i)
        {
            xmin = std::min(xmin, corners_2f[i].x);
            ymin = std::min(ymin, corners_2f[i].y);
            xmax = std::max(xmax, corners_2f[i].x);
            ymax = std::max(ymax, corners_2f[i].y);
        }

        corners[0] = cv::Point(0, 0);
        corners[1] = cv::Point(static_cast<int>(xmin),static_cast<int>(ymin));

        sizes[0] = cv::Size(mt.trainImage.size());
        sizes[1] = cv::Size(static_cast<int>(xmax-xmin),static_cast<int>(ymax-ymin));

        return cv::Matx<double, 3, 3>(
            1, 0, static_cast<double>(-corners[1].x),
            0, 1, static_cast<double>(-corners[1].y),
            0, 0, 1);
    }

    void planarWarper_warp(const Matches &mt, const cv::Mat& homography) 
    {
        cv::Matx<double, 3, 3> _H = planarWarper_findCorners(mt, homography);

        // prepare masks
        cv::Mat mask0(mt.trainImage.size(), CV_8U);
        mask0.setTo(cv::Scalar::all(255));

        cv::Mat mask1(mt.trainImage.size(), CV_8U);
        mask1.setTo(cv::Scalar::all(255));

        imgWarped[0] = mt.trainImage;
        maskWarped[0] = mask0;
        cv::warpPerspective(mt.queryImage, imgWarped[1], cv::Mat(_H)*homography, sizes[1], cv::INTER_CUBIC);
        cv::warpPerspective(mask1, maskWarped[1], cv::Mat(_H)*homography, sizes[1], cv::INTER_CUBIC);
    }

    cv::Rect simpleBlender_findRoi() 
    {
        // findRoi
        cv::Point topLeft(std::numeric_limits<int>::max(), std::numeric_limits<int>::max());
        cv::Point bottomRight(std::numeric_limits<int>::min(), std::numeric_limits<int>::min());

        for (size_t i = 0; i < 2; ++i)
        {
            topLeft.x = std::min(topLeft.x, corners[i].x);
            topLeft.y = std::min(topLeft.y, corners[i].y);
            bottomRight.x = std::max(bottomRight.x, corners[i].x + sizes[i].width);
            bottomRight.y = std::max(bottomRight.y, corners[i].y + sizes[i].height);
        }

        return cv::Rect(topLeft, bottomRight);
    }

    void simpleBlender_feed(int i, cv::Rect& roi, cv::Mat& dst, cv::Mat& maskDest)
    {
        int origin_x = corners[i].x - roi.x;
        int origin_y = corners[i].y - roi.y;
        cv::Mat img32S;
        imgWarped[i].convertTo(img32S, CV_32S);

        for (int y = 0; y < img32S.rows; ++y)
        {
            //const cv::Point3i* ptr_img32S = img32S.ptr<cv::Point3i>(y);
            const int* ptr_img32S = img32S.ptr<int>(y);
            const uchar* ptr_msk_wrpd = maskWarped[i].ptr<uchar>(y);

            //cv::Point3i* ptr_img_dst = dst.ptr<cv::Point3i>(origin_y + y);
            int* ptr_img_dst = dst.ptr<int>(origin_y + y);
            uchar* ptr_msk_dst = maskDest.ptr<uchar>(origin_y + y);

            for (int x = 0; x < img32S.cols; ++x)
            {
                if (ptr_msk_wrpd[x])
                    ptr_img_dst[origin_x + x] = ptr_img32S[x];

                ptr_msk_dst[origin_x + x] |= ptr_msk_wrpd[x];
            }
        }
    }

private:
    cv::Point corners[2];
    cv::Size sizes[2];
    cv::Mat imgWarped[2];
    cv::Mat maskWarped[2];
};

REGISTER_NODE("Multi-images/Simple mosaic", SimpleMosaicNodeType)
