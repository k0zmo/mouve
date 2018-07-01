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

cv::Scalar getColor(EColor color)
{
    switch (color)
    {
    // Order is BGR
    case EColor::Red:
        return {36, 28, 237};
    case EColor::Green:
        return {76, 177, 34};
    case EColor::Blue:
        return {244, 63, 72};
    case EColor::Yellow:
        return {0, 255, 242};
    }
    return cv::Scalar::all(-1);
}

cv::Scalar getRandomColor(cv::RNG& rng)
{
    return cv::Scalar(rng(256), rng(256), rng(256));
}
} // namespace

cv::Mat drawMatches(const Matches& mt, EColor colorName, EColor kpColorName)
{
    constexpr auto thickness = 1;
    constexpr auto lineType = CV_AA;

    // mt.queryImage.type() == mt.trainImage.type() for now

    const int cols = mt.queryImage.cols + mt.trainImage.cols;
    const int rows = std::max(mt.queryImage.rows, mt.trainImage.rows);
    cv::Mat matDraw(rows, cols, mt.queryImage.type(), cv::Scalar(0));

    const cv::Rect roi1(0, 0, mt.queryImage.cols, mt.queryImage.rows);
    const cv::Rect roi2(mt.queryImage.cols, 0, mt.trainImage.cols, mt.trainImage.rows);

    mt.queryImage.copyTo(matDraw(roi1));
    mt.trainImage.copyTo(matDraw(roi2));
    if (matDraw.channels() == 1)
        cvtColor(matDraw, matDraw, cv::COLOR_GRAY2BGR);

    cv::Mat matDraw1 = matDraw(roi1);
    cv::Mat matDraw2 = matDraw(roi2);
    cv::RNG& rng = cv::theRNG();

    for (size_t i = 0; i < mt.queryPoints.size(); ++i)
    {
        const auto& kp1 = mt.queryPoints[i];
        const auto& kp2 = mt.trainPoints[i];

        const cv::Scalar lineColor =
            colorName == EColor::AllRandom ? getRandomColor(rng) : getColor(colorName);
        const cv::Scalar pointColor =
            colorName == EColor::AllRandom ? lineColor : getColor(kpColorName);

        constexpr auto radius = 10;

        cv::circle(matDraw1, kp1, radius, pointColor, thickness, lineType);
        cv::circle(matDraw2, kp2, radius, pointColor, thickness, lineType);

        const auto pt1 = cv::Point{cvRound(kp1.x) + roi1.tl().x, cvRound(kp1.y) + roi1.tl().y};
        const auto pt2 = cv::Point{cvRound(kp2.x) + roi2.tl().x, cvRound(kp2.y) + roi2.tl().y};
        cv::line(matDraw, pt1, pt2, lineColor, thickness, lineType);
    }

    return matDraw;
}
