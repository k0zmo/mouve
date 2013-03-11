#pragma once

#include "Prerequisites.h"

#include "boost/variant.hpp"

// OpenCV
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>

namespace cv {
typedef std::vector<KeyPoint> KeyPoints;
typedef std::vector<DMatch> DMatches;
}

typedef boost::variant<
	cv::Mat,
	cv::KeyPoints,
	cv::DMatches
> flow_data;

enum class ENodeFlowDataType : int
{
	Invalid,
	Image,
	ImageRgb,
	Array,
	Keypoints,
	Matches	
	/// TODO: OpenCL
	//DeviceImage,
	//DeviceKeypoints, ...
};

class MOUVE_LOGIC_EXPORT NodeFlowData
{
public:
	NodeFlowData();
	NodeFlowData(ENodeFlowDataType dataType);
	
	//NodeFlowData& operator=(const NodeFlowData& other);
	//NodeFlowData& operator=(NodeFlowData&& other);
	//NodeFlowData(const NodeFlowData& other);
	//NodeFlowData(NodeFlowData&& other);

	~NodeFlowData();

	cv::Mat& getImage();
	const cv::Mat& getImage() const;

	cv::Mat& getImageRgb();
	const cv::Mat& getImageRgb() const;

	cv::KeyPoints& getKeypoints();
	const cv::KeyPoints& getKeypoints() const;

	cv::DMatches& getMatches();
	const cv::DMatches& getMatches() const;

	cv::Mat& getArray();
	const cv::Mat& getArray() const;

	bool isValid() const;
	bool canReturn(ENodeFlowDataType type) const;

	ENodeFlowDataType type() const;

	size_t memoryConsumption() const;

private:
	ENodeFlowDataType _type;
	/// TODO: These are always 0 for now
	int _width;
	int _height;
	int _elemSize;
	flow_data _data;
};

inline bool NodeFlowData::isValid() const
{ return _type != ENodeFlowDataType::Invalid; }

inline bool NodeFlowData::canReturn(ENodeFlowDataType type) const
{ return _type == type && isValid(); }

inline ENodeFlowDataType NodeFlowData::type() const
{ return _type; }

inline size_t NodeFlowData::memoryConsumption() const
{ return _width * _height * _elemSize; }

