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

#include "impl/Matchers.h"

#include <opencv2/features2d/features2d.hpp>
#include <fmt/core.h>

namespace {

void convertToMatches(const KeyPoints& queryKeypoints, const KeyPoints& trainKeypoints,
                             const std::vector<cv::DMatch>& matches, Matches& out)
{
    // Convert to 'Matches' data type
    out.queryPoints.clear();
    out.trainPoints.clear();

    out.queryImage = queryKeypoints.image;
    out.trainImage = trainKeypoints.image;

    for (const auto& match : matches)
    {
        out.queryPoints.push_back(queryKeypoints.kpoints[match.queryIdx].pt);
        out.trainPoints.push_back(trainKeypoints.kpoints[match.trainIdx].pt);
    }
}
} // namespace

class MatcherBaseNodeType : public NodeType
{
public:
    MatcherBaseNodeType()
        : _distanceRatio(0.8f)
        , _symmetryTest(false)
    {
        addInput("Query keypoints", ENodeFlowDataType::Keypoints);
        addInput("Query descriptors", ENodeFlowDataType::Array);
        addInput("Train keypoints", ENodeFlowDataType::Keypoints);
        addInput("Train descriptors", ENodeFlowDataType::Array);
        addOutput("Matches", ENodeFlowDataType::Matches);
        addProperty("Distance ratio", _distanceRatio)
            .setValidator(make_validator<InclRangePropertyValidator<double>>(0.0, 1.0))
            .setUiHints("min:0.0, max:1.0, step:0.1, decimals:2");
        addProperty("Symmetry test", _symmetryTest);
        setDescription("Finds best matches between query and train descriptors "
            "using nearest neighbour distance ratio and/or symmetry test.");
    }

protected:
    TypedNodeProperty<float> _distanceRatio;
    TypedNodeProperty<bool> _symmetryTest;
};

class BruteForceMatcherNodeType : public MatcherBaseNodeType
{
public:
    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const KeyPoints& queryKp = reader.readSocket(0).getKeypoints();
        const cv::Mat& query = reader.readSocket(1).getArray();
        const KeyPoints& trainKp = reader.readSocket(2).getKeypoints();
        const cv::Mat& train = reader.readSocket(3).getArray();
        // Acquire output sockets
        Matches& mt = writer.acquireSocket(0).getMatches();

        // Validate inputs
        if (train.empty() || query.empty())
            return ExecutionStatus(EStatus::Ok);

        if (query.cols != train.cols || query.type() != train.type())
        {
            return ExecutionStatus(EStatus::Error,
                                   "Query and train descriptors are different types");
        }
        if (query.type() != CV_32F && query.type() != CV_8U)
        {
            return ExecutionStatus(
                EStatus::Error,
                "Unsupported descriptor data type (must be float for L2 norm or Uint8 for Hamming norm");
        }

        // Do stuff
        const std::vector<cv::DMatch> matches =
            bruteForceMatch(query, train, _distanceRatio, _symmetryTest);
        convertToMatches(queryKp, trainKp, matches, mt);

        return ExecutionStatus(EStatus::Ok,
                               fmt::format("Matches found: {}", mt.queryPoints.size()));
    }
};

class AproximateNearestNeighborMatcherNodeType : public MatcherBaseNodeType
{
public:
    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const KeyPoints& queryKp = reader.readSocket(0).getKeypoints();
        const cv::Mat& queryDesc = reader.readSocket(1).getArray();
        const KeyPoints& trainKp = reader.readSocket(2).getKeypoints();
        const cv::Mat& trainDesc = reader.readSocket(3).getArray();
        // Acquire output sockets
        Matches& mt = writer.acquireSocket(0).getMatches();

        // Validate inputs
        if (trainDesc.empty() || queryDesc.empty())
            return ExecutionStatus(EStatus::Ok);

        if (static_cast<size_t>(trainDesc.rows) != trainKp.kpoints.size() ||
            static_cast<size_t>(queryDesc.rows) != queryKp.kpoints.size())
        {
            return ExecutionStatus(EStatus::Error, "Keypoints and descriptors mismatched");
        }

        const auto matches = flannMatch(queryDesc, trainDesc, _distanceRatio, _symmetryTest);
        convertToMatches(queryKp, trainKp, matches, mt);

        return ExecutionStatus(EStatus::Ok,
                               fmt::format("Matches found: {}", mt.queryPoints.size()));
    }
};

/// Only BFMatcher as LSH (FLANN) for 1-bit descriptor isn't supported
class RadiusBruteForceMatcherNodeType : public NodeType
{
public:
    RadiusBruteForceMatcherNodeType() : _maxDistance(100.0)
    {
        addInput("Query keypoints", ENodeFlowDataType::Keypoints);
        addInput("Query descriptors", ENodeFlowDataType::Array);
        addInput("Train keypoints", ENodeFlowDataType::Keypoints);
        addInput("Train descriptors", ENodeFlowDataType::Array);
        addOutput("Matches", ENodeFlowDataType::Matches);
        addProperty("Max distance", _maxDistance)
            .setValidator(make_validator<MinPropertyValidator<double>>(0.0))
            .setUiHints("min:0.0, decimals:2");
        setDescription("Finds the training descriptors not farther than the specified distance.");
    }

    ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
    {
        // Read input sockets
        const KeyPoints& queryKp = reader.readSocket(0).getKeypoints();
        const cv::Mat& queryDesc = reader.readSocket(1).getArray();
        const KeyPoints& trainKp = reader.readSocket(2).getKeypoints();
        const cv::Mat& trainDesc = reader.readSocket(3).getArray();
        // Acquire output sockets
        Matches& mt = writer.acquireSocket(0).getMatches();

        // Validate inputs
        if (trainDesc.empty() || queryDesc.empty())
            return ExecutionStatus(EStatus::Ok);

        if (static_cast<size_t>(trainDesc.rows) != trainKp.kpoints.size() ||
            static_cast<size_t>(queryDesc.rows) != queryKp.kpoints.size())
        {
            return ExecutionStatus(EStatus::Error, "Keypoints and descriptors mismatched");
        }

        // Do stuff
        const auto normType = [&] {
            return
                // If BRISK, ORB or FREAK was used as a descriptor
                queryDesc.depth() == CV_8U && trainDesc.depth() == CV_8U ? cv::NORM_HAMMING
                                                                         : cv::NORM_L2;
        }();
        std::vector<std::vector<cv::DMatch>> radiusMatches;
        cv::BFMatcher{normType}.radiusMatch(queryDesc, trainDesc, radiusMatches, _maxDistance);

        // Remove unmatched ones and pick only first match (closes one)
        std::vector<cv::DMatch> matches;
        for (const auto& rmatches : radiusMatches)
        {
            if (!rmatches.empty())
                matches.push_back(rmatches[0]);
        }

        convertToMatches(queryKp, trainKp, matches, mt);

        return ExecutionStatus(EStatus::Ok,
                               fmt::format("Matches found: {}", mt.queryPoints.size()));
    }

private:
    TypedNodeProperty<float> _maxDistance;
};

REGISTER_NODE("Features/Radius BForce Matcher", RadiusBruteForceMatcherNodeType);
REGISTER_NODE("Features/BForce Matcher", BruteForceMatcherNodeType);
REGISTER_NODE("Features/ANN Matcher", AproximateNearestNeighborMatcherNodeType)
