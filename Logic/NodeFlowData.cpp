#include "NodeFlowData.h"

NodeFlowData::NodeFlowData()
	: _type(ENodeFlowDataType::Invalid)
	, _width(0)
	, _height(0)
	, _elemSize(0)
	, _data()
{
}

NodeFlowData::NodeFlowData(ENodeFlowDataType type, const cv::Mat& image)
	: _type(type)
	, _width(image.cols)
	, _height(image.rows)
	, _elemSize(image.elemSize())
	, _data(image)
{
	/// TODO: There's no run-time checks
}

NodeFlowData::~NodeFlowData()
{
}

cv::Mat& NodeFlowData::getImage()
{
	Q_ASSERT(_type != ENodeFlowDataType::Invalid);
	return boost::get<cv::Mat>(_data);
}

const cv::Mat& NodeFlowData::getImage() const
{
	Q_ASSERT(_type != ENodeFlowDataType::Invalid);
	return boost::get<cv::Mat>(_data);
}