#pragma once

#include "Prerequisites.h"

#include "boost/variant.hpp"

// OpenCV
#include <opencv2/core/core.hpp>

typedef boost::variant<cv::Mat> 
	flow_data;

enum class ENodeFlowDataType
{
	Invalid,
	ImageRgb,
	ImageGray,
	ImageBinary
};

class MOUVE_LOGIC_EXPORT NodeFlowData
{
public:
	NodeFlowData();
	NodeFlowData(ENodeFlowDataType type, const cv::Mat& value);
	
	//NodeFlowData& operator=(const NodeFlowData& other);
	//NodeFlowData& operator=(NodeFlowData&& other);
	//NodeFlowData(const NodeFlowData& other);
	//NodeFlowData(NodeFlowData&& other);

	~NodeFlowData();

	cv::Mat& getImage();
	const cv::Mat& getImage() const;

	bool isValid() const;
	bool canReturn(ENodeFlowDataType type) const;

	ENodeFlowDataType type() const;

	size_t memoryConsumption() const;

private:
	ENodeFlowDataType _type;
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

