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

#include "impl/Drawing.h"

#include <opencv2/imgproc/imgproc.hpp>

namespace {

void drawKeypoint(cv::Mat& img, const cv::KeyPoint& p, const cv::Scalar& color, bool richKeypoint)
{
    CV_Assert(!img.empty());

    constexpr auto thickness = 1;
    constexpr auto lineType = CV_AA;

    if (richKeypoint)
    {
        const float radius = p.size;
        cv::Point2f x1{-radius, -radius};
        cv::Point2f x2{+radius, -radius};
        cv::Point2f x3{-radius, +radius};
        cv::Point2f x4{+radius, +radius};

        const float ori = p.angle * (float)CV_PI / 180.f;
        const float c = std::cos(ori);
        const float s = std::sin(ori);

        const float r1 = static_cast<float>(cvRound(p.pt.y));
        const float c1 = static_cast<float>(cvRound(p.pt.x));

        auto transform = [=](const cv::Point2f& p) {
            return cv::Point2f(c * p.x - s * p.y + c1, s * p.x + c * p.y + r1);
        };

        x1 = transform(x1);
        x2 = transform(x2);
        x3 = transform(x3);
        x4 = transform(x4);

        cv::line(img, x1, x2, color, thickness, lineType);
        cv::line(img, x2, x4, color, thickness, lineType);
        cv::line(img, x4, x3, color, thickness, lineType);
        cv::line(img, x3, x1, color, thickness, lineType);

        if (p.angle != -1)
        {
            const float c2 = cvRound(radius * c) + c1;
            const float r2 = cvRound(radius * s) + r1;

            cv::line(img, cv::Point2f{c1, r1}, cv::Point2f{c2, r2}, color, thickness, lineType);
        }
    }
    else
    {
        constexpr auto radius = 3;
        cv::circle(img, cv::Point2f{p.pt.x, p.pt.y}, radius, color, thickness, lineType);
    }
}
} // namespace

void drawKeypoints(const cv::Mat& srcImage, const std::vector<cv::KeyPoint>& keypoints,
                   cv::Mat& dst, EColor colorName, bool richKeypoint)
{
#if 0
    const auto drawFlags =
        richKeypoint ? cv::DrawMatchesFlags::DRAW_RICH_KEYPOINTS : cv::DrawMatchesFlags::DEFAULT;
    cv::drawKeypoints(srcImage, keypoints, dst, getColor(colorName), drawFlags);
#else
    cvtColor(srcImage, dst, CV_GRAY2BGR);

    for (const auto& it : keypoints)
        drawKeypoint(dst, it, getColor(colorName), richKeypoint);
#endif
}
