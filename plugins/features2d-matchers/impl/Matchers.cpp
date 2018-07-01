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

#include "impl/Matchers.h"

namespace {

std::vector<cv::DMatch> distanceRatioTest(const std::vector<std::vector<cv::DMatch>>& knMatches,
                                          float distanceRatioThreshold)
{
    std::vector<cv::DMatch> positiveMatches;
    positiveMatches.reserve(knMatches.size());

    for (const auto& knMatch : knMatches)
    {
        if (knMatch.size() != 2)
            continue;

        const auto& best = knMatch[0];
        const auto& good = knMatch[1];

        if (best.distance <= distanceRatioThreshold * good.distance)
            positiveMatches.push_back(best);
    }

    return positiveMatches;
}

std::vector<cv::DMatch> symmetryTest(const std::vector<cv::DMatch>& matches1to2,
                                     const std::vector<cv::DMatch>& matches2to1)
{
    std::vector<cv::DMatch> bothMatches;

    for (const auto& match1to2 : matches1to2)
    {
        for (const auto& match2to1 : matches2to1)
        {
            if (match1to2.queryIdx == match2to1.trainIdx &&
                match2to1.queryIdx == match1to2.trainIdx)
            {
                bothMatches.push_back(match1to2);
                break;
            }
        }
    }

    return bothMatches;
}

std::vector<cv::DMatch> nndrMatch32F(const cv::Mat& queryDescriptors,
                                     const cv::Mat& trainDescriptors, float distanceRatio)
{
    std::vector<cv::DMatch> nndrMatches;

    // for each descriptors in query array
    for (int queryIdx = 0; queryIdx < queryDescriptors.rows; ++queryIdx)
    {
        const float* query_row = queryDescriptors.ptr<float>(queryIdx);

        float bestDistance1 = std::numeric_limits<float>::max();
        float bestDistance2 = std::numeric_limits<float>::max();
        int bestTrainIdx1 = -1;

        // for each descriptors in train array
        for (int trainIdx = 0; trainIdx < trainDescriptors.rows; ++trainIdx)
        {
            const float* train_row = trainDescriptors.ptr<float>(trainIdx);

            cv::L2<float> op;
            cv::L2<float>::ResultType dist = op(train_row, query_row, trainDescriptors.cols);

            if (dist < bestDistance1)
            {
                bestDistance2 = bestDistance1;
                bestDistance1 = dist;
                bestTrainIdx1 = trainIdx;
            }
            else if (dist < bestDistance2)
            {
                bestDistance2 = dist;
            }
        }

        if (bestDistance1 <= distanceRatio * bestDistance2)
        {
            nndrMatches.push_back(cv::DMatch(queryIdx, bestTrainIdx1, bestDistance1));
        }
    }

    return nndrMatches;
}

std::vector<cv::DMatch> nndrMatch8U(const cv::Mat& queryDescriptors,
                                    const cv::Mat& trainDescriptors, float distanceRatio)
{
    std::vector<cv::DMatch> nndrMatches;

    // for each descriptors in query array
    for (int queryIdx = 0; queryIdx < queryDescriptors.rows; ++queryIdx)
    {
        const uchar* query_row = queryDescriptors.ptr<uchar>(queryIdx);

        int bestDistance1 = std::numeric_limits<int>::max();
        int bestDistance2 = std::numeric_limits<int>::max();
        int bestTrainIdx1 = -1;

        // for each descriptors in train array
        for (int trainIdx = 0; trainIdx < trainDescriptors.rows; ++trainIdx)
        {
            const uchar* train_row = trainDescriptors.ptr<uchar>(trainIdx);

            cv::Hamming op;
            cv::Hamming::ResultType dist = op(train_row, query_row, trainDescriptors.cols);

            if (dist < bestDistance1)
            {
                bestDistance2 = bestDistance1;
                bestDistance1 = dist;
                bestTrainIdx1 = trainIdx;
            }
            else if (dist < bestDistance2)
            {
                bestDistance2 = dist;
            }
        }

        if (bestDistance1 <= distanceRatio * bestDistance2)
        {
            nndrMatches.emplace_back(cv::DMatch(queryIdx, bestTrainIdx1, (float)bestDistance1));
        }
    }

    return nndrMatches;
}

std::vector<cv::DMatch> nndrMatch(const cv::Mat& queryDescriptors, const cv::Mat& trainDescriptors,
                                  float distanceRatio)
{
    if (queryDescriptors.type() == CV_32F)
    {
        return nndrMatch32F(queryDescriptors, trainDescriptors, distanceRatio);
    }
    else if (queryDescriptors.type() == CV_8U)
    {
        return nndrMatch8U(queryDescriptors, trainDescriptors, distanceRatio);
    }
    else
    {
        assert(false && "unsupported type, must be CV_32F or CV_8U");
        return {};
    }
}
} // namespace

std::vector<cv::DMatch> bruteForceMatch(const cv::Mat& queryDescriptors,
                                        const cv::Mat& trainDescriptors, float distanceRatio,
                                        bool testSymmetry)
{
    if (testSymmetry)
    {
        const auto matches1to2 = nndrMatch(queryDescriptors, trainDescriptors, distanceRatio);
        const auto matches2to1 = nndrMatch(trainDescriptors, queryDescriptors, distanceRatio);
        return symmetryTest(matches1to2, matches2to1);
    }
    else
    {
        return nndrMatch(queryDescriptors, trainDescriptors, distanceRatio);
    }
}

std::vector<cv::DMatch> flannMatch(const cv::Mat& queryDescriptors, const cv::Mat& trainDescriptors,
                                   float distanceRatio, bool testSymmetry)
{
    assert(trainDescriptors.depth() == CV_8U || trainDescriptors.depth() == CV_32F ||
           trainDescriptors.depth() == CV_64F);

    // Do stuff
    auto indexParams = [&]() -> cv::Ptr<cv::flann::IndexParams> {
        switch (trainDescriptors.depth())
        {
        case CV_8U:
            return new cv::flann::LshIndexParams(12, 20, 2);
            // For now doesn't work for binary descriptors :(
            // return new cv::flann::HierarchicalClusteringIndexParams();
        case CV_32F:
        case CV_64F:
            return new cv::flann::HierarchicalClusteringIndexParams();
        default:
            return nullptr;
        }
    }();

    cv::FlannBasedMatcher matcher{indexParams};
    if (testSymmetry)
    {
        std::vector<std::vector<cv::DMatch>> knMatches1to2, knMatches2to1;
        matcher.knnMatch(queryDescriptors, trainDescriptors, knMatches1to2, 2);
        matcher.knnMatch(trainDescriptors, queryDescriptors, knMatches2to1, 2);

        const auto matches1to2 = distanceRatioTest(knMatches1to2, distanceRatio);
        const auto matches2to1 = distanceRatioTest(knMatches2to1, distanceRatio);
        return symmetryTest(matches1to2, matches2to1);
    }
    else
    {
        std::vector<std::vector<cv::DMatch>> knMatches;
        matcher.knnMatch(queryDescriptors, trainDescriptors, knMatches, 2);

        return distanceRatioTest(knMatches, distanceRatio);
    }
}
