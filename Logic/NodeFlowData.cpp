#include "NodeFlowData.h"

#include <opencv2/highgui/highgui.hpp>

NodeFlowData::NodeFlowData()
	: _type(ENodeFlowDataType::Invalid)
	, _width(0)
	, _height(0)
	, _elemSize(0)
	, _data()
{
}

NodeFlowData::NodeFlowData(ENodeFlowDataType dataType)
	: _type(dataType)
	, _width(0)
	, _height(0)
	, _elemSize(0)
	, _data()
{
	switch(dataType)
	{
	case ENodeFlowDataType::Image:
		_data = cv::Mat();
		break;
	case ENodeFlowDataType::ImageRgb:
		_data = cv::Mat();
		break;
	case ENodeFlowDataType::Array:
		_data = cv::Mat();
		break;
	case ENodeFlowDataType::Keypoints:
		_data = cv::KeyPoints();
		break;
	case ENodeFlowDataType::Matches:
		_data = cv::DMatches();
		break;
	case ENodeFlowDataType::Invalid:
	default:
		break;
	}
}

//NodeFlowData::NodeFlowData(ENodeFlowDataType type, const cv::Mat& image)
//	: _type(type)
//	, _width(image.cols)
//	, _height(image.rows)
//	, _elemSize(image.elemSize())
//	, _data(image)
//{
//	/// TODO: There's no run-time checks
//}

NodeFlowData::~NodeFlowData()
{
}

void NodeFlowData::saveToDisk(const std::string& filename) const
{
	if(_type == ENodeFlowDataType::Image
		|| _type == ENodeFlowDataType::ImageRgb)
	{
		const cv::Mat& img = boost::get<cv::Mat>(_data);
		if(!img.empty())
			cv::imwrite(filename, img);
	}
}

bool NodeFlowData::isValidImage() const
{
	if(_type == ENodeFlowDataType::Image
		|| _type == ENodeFlowDataType::ImageRgb)
	{
		const cv::Mat& img = boost::get<cv::Mat>(_data);
		if(!img.empty())
			return true;
	}
	return false;
}

cv::Mat& NodeFlowData::getImage()
{
	if(_type != ENodeFlowDataType::Image)
		throw boost::bad_get();

	return boost::get<cv::Mat>(_data);
}

const cv::Mat& NodeFlowData::getImage() const
{
	if(_type != ENodeFlowDataType::Image)
		throw boost::bad_get();

	return boost::get<cv::Mat>(_data);
}

cv::Mat& NodeFlowData::getImageRgb()
{
	if(_type != ENodeFlowDataType::ImageRgb)
		throw boost::bad_get();

	return boost::get<cv::Mat>(_data);
}

const cv::Mat& NodeFlowData::getImageRgb() const
{
	if(_type != ENodeFlowDataType::ImageRgb)
		throw boost::bad_get();

	return boost::get<cv::Mat>(_data);
}

cv::KeyPoints& NodeFlowData::getKeypoints()
{
	if(_type != ENodeFlowDataType::Keypoints)
		throw boost::bad_get();

	return boost::get<cv::KeyPoints>(_data);
}

const cv::KeyPoints& NodeFlowData::getKeypoints() const
{
	if(_type != ENodeFlowDataType::Keypoints)
		throw boost::bad_get();

	return boost::get<cv::KeyPoints>(_data);
}

cv::DMatches& NodeFlowData::getMatches()
{
	if(_type != ENodeFlowDataType::Matches)
		throw boost::bad_get();

	return boost::get<cv::DMatches>(_data);
}

const cv::DMatches& NodeFlowData::getMatches() const
{
	if(_type != ENodeFlowDataType::Matches)
		throw boost::bad_get();

	return boost::get<cv::DMatches>(_data);
}

cv::Mat& NodeFlowData::getArray()
{
	if(_type != ENodeFlowDataType::Array)
		throw boost::bad_get();

	return boost::get<cv::Mat>(_data);
}

const cv::Mat& NodeFlowData::getArray() const
{
	if(_type != ENodeFlowDataType::Array)
		throw boost::bad_get();

	return boost::get<cv::Mat>(_data);
}