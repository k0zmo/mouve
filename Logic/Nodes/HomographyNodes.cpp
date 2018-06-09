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

#include <fmt/core.h>
#include <opencv2/calib3d/calib3d.hpp>

#include <fstream>

using std::vector;

class EstimateHomographyNodeType : public NodeType
{
public:
    EstimateHomographyNodeType()
        : _reprojThreshold(3.0)
    {
        addInput("Matches", ENodeFlowDataType::Matches);
        addOutput("Homography", ENodeFlowDataType::Array);
        addOutput("Inliers", ENodeFlowDataType::Matches);
        addProperty("Reprojection error threshold", _reprojThreshold)
            .setValidator(make_validator<InclRangePropertyValidator<double>>(1.0, 50.0))
            .setUiHints("min:1.0, max:50.0");
        setDescription("Finds a perspective transformation between two planes.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const Matches& mt = reader.readSocket(0).getMatches();
        // Acquire output sockets
        cv::Mat& H = writer.acquireSocket(0).getArray();
        Matches& outMt = writer.acquireSocket(1).getMatches();

        // Validate inputs
        if(mt.queryPoints.empty() || mt.trainPoints.empty()
        || mt.queryImage.empty() || mt.trainImage.empty())
            return ExecutionStatus(EStatus::Ok);

        if(mt.queryPoints.size() != mt.trainPoints.size())
            return ExecutionStatus(EStatus::Error, 
                "Points from one images doesn't correspond to key points in another one");

        // Do stuff
        outMt.queryImage = mt.queryImage;
        outMt.trainImage = mt.trainImage;
        outMt.queryPoints.clear();
        outMt.trainPoints.clear();
        size_t kpSize = mt.queryPoints.size();

        if(mt.queryPoints.size() < 4)
        {
            H = cv::Mat::ones(3, 3, CV_64F);
            return ExecutionStatus(EStatus::Ok, "Not enough matches for homography estimation");
        }

        vector<uchar> inliersMask;
        cv::Mat homography = cv::findHomography(mt.queryPoints, mt.trainPoints, 
            CV_RANSAC, _reprojThreshold, inliersMask);
        int inliersCount = (int) std::count(begin(inliersMask), end(inliersMask), 1);
        if(inliersCount < 4)
            return ExecutionStatus(EStatus::Ok);

        vector<cv::Point2f> queryPoints(inliersCount), trainPoints(inliersCount);

        // Create vector of inliers only
        for(size_t inlier = 0, idx = 0; inlier < kpSize; ++inlier)
        {
            if(inliersMask[inlier] == 0)
                continue;

            queryPoints[idx] = mt.queryPoints[inlier];
            trainPoints[idx] = mt.trainPoints[inlier];
            ++idx;
        }

        // Use only good points to find refined homography
        H = cv::findHomography(queryPoints, trainPoints, 0);

        // Reproject again
        vector<cv::Point2f> srcReprojected;
        cv::perspectiveTransform(trainPoints, srcReprojected, H.inv());
        kpSize = queryPoints.size();

        for (size_t i = 0; i < kpSize; i++)
        {
            cv::Point2f actual = queryPoints[i];
            cv::Point2f expect = srcReprojected[i];
            cv::Point2f v = actual - expect;
            float distanceSquared = v.dot(v);

            if (distanceSquared <= _reprojThreshold * _reprojThreshold)
            {
                outMt.queryPoints.emplace_back(queryPoints[i]);
                outMt.trainPoints.emplace_back(trainPoints[i]);
            }
        }

        return ExecutionStatus(
            EStatus::Ok,
            fmt::format("Inliers: {}\nOutliers: {}\nPercent of correct matches: {}%",
                        outMt.queryPoints.size(), mt.queryPoints.size() - outMt.queryPoints.size(),
                        static_cast<double>(outMt.queryPoints.size()) / mt.queryPoints.size() *
                            100.0));
    }

private:
    TypedNodeProperty<double> _reprojThreshold;
};

class KnownHomographyInliersNodeType : public NodeType
{
public:
    KnownHomographyInliersNodeType()
        : _homographyPath(Filepath{})
        , _reprojThreshold(3.0)
    {
        addInput("Matches", ENodeFlowDataType::Matches);
        addOutput("Homography", ENodeFlowDataType::Array);
        addOutput("Inliers", ENodeFlowDataType::Matches);
        addProperty("Homography path", _homographyPath);
        addProperty("Reprojection error threshold", _reprojThreshold)
            .setValidator(make_validator<InclRangePropertyValidator<double>>(1.0, 50.0))
            .setUiHints("min:1.0, max:50.0");
        setDescription("Rejects outliers using known homography.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const Matches& mt = reader.readSocket(0).getMatches();
        // Acquire output sockets
        cv::Mat& H = writer.acquireSocket(0).getArray();
        Matches& outMt = writer.acquireSocket(1).getMatches();

        // Validate inputs
        if(mt.queryPoints.empty() || mt.trainPoints.empty()
        || mt.queryImage.empty() || mt.trainImage.empty())
            return ExecutionStatus(EStatus::Ok);

        if(mt.queryPoints.size() != mt.trainPoints.size())
            return ExecutionStatus(EStatus::Error, 
            "Points from one images doesn't correspond to key points in another one");

        // Do stuff - Load real homography
        std::ifstream homographyFile(_homographyPath.cast<Filepath>().data(), std::ios::in);
        if (!homographyFile.is_open())
        {
            return ExecutionStatus(
                EStatus::Error,
                fmt::format("Can't load {}\n", _homographyPath.cast<Filepath>().data()));
        }

        cv::Mat homography(3, 3, CV_32F);
        homographyFile >> homography.at<float>(0, 0) >> homography.at<float>(0, 1) >> homography.at<float>(0, 2);
        homographyFile >> homography.at<float>(1, 0) >> homography.at<float>(1, 1) >> homography.at<float>(1, 2);
        homographyFile >> homography.at<float>(2, 0) >> homography.at<float>(2, 1) >> homography.at<float>(2, 2);

        // Normalize if h[2][2] isnt close to 1.0 and 0.0
        float h22 = homography.at<float>(2,2);
        if(std::fabs(h22) > 1e-5 && std::fabs(h22 - 1.0) > 1e-5)
        {
            h22 = 1.0f / h22;
            homography.at<float>(0,0) *= h22; homography.at<float>(0,1) *= h22; homography.at<float>(0,2) *= h22;
            homography.at<float>(1,0) *= h22; homography.at<float>(1,1) *= h22; homography.at<float>(1,2) *= h22;
            homography.at<float>(2,0) *= h22; homography.at<float>(2,1) *= h22; homography.at<float>(2,2) *= h22;
        }

        outMt.queryImage = mt.queryImage;
        outMt.trainImage = mt.trainImage;
        outMt.queryPoints.clear();
        outMt.trainPoints.clear();

        for(size_t i = 0; i < mt.queryPoints.size(); ++i)
        {
            const auto& pt1 = mt.queryPoints[i];
            const auto& pt2 = mt.trainPoints[i];

            float invW = 1.0f / (homography.at<float>(2,0)*pt1.x + homography.at<float>(2,1)*pt1.y + homography.at<float>(2,2));
            float xt = (homography.at<float>(0,0)*pt1.x + homography.at<float>(0,1)*pt1.y + homography.at<float>(0,2)) * invW;
            float yt = (homography.at<float>(1,0)*pt1.x + homography.at<float>(1,1)*pt1.y + homography.at<float>(1,2)) * invW;
            
            float xdist = xt - pt2.x;
            float ydist = yt - pt2.y;
            float dist = std::sqrt(xdist*xdist + ydist*ydist);

            if(dist <= _reprojThreshold)
            {
                outMt.queryPoints.push_back(pt1);
                outMt.trainPoints.push_back(pt2);
            }
        }

        H = homography;

        return ExecutionStatus(
            EStatus::Ok,
            fmt::format("Inliers: {}\nOutliers: {}\nPercent of correct matches: {}%",
                        outMt.queryPoints.size(), mt.queryPoints.size() - outMt.queryPoints.size(),
                        static_cast<double>(outMt.queryPoints.size()) / mt.queryPoints.size() *
                            100.0));
    }

private:
    TypedNodeProperty<Filepath> _homographyPath;
    TypedNodeProperty<double> _reprojThreshold;
};

REGISTER_NODE("Features/Known homography inliers", KnownHomographyInliersNodeType)
REGISTER_NODE("Features/Estimate homography", EstimateHomographyNodeType)
