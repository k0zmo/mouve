#include "NodeFlowData.h"

#include <opencv2/highgui/highgui.hpp>

NodeFlowData::NodeFlowData()
	: _type(ENodeFlowDataType::Invalid)
	, _data()
{
}

NodeFlowData::NodeFlowData(ENodeFlowDataType dataType)
	: _type(dataType)
	, _data()
{
	switch(dataType)
	{
	case ENodeFlowDataType::Image:
	case ENodeFlowDataType::ImageMono:
	case ENodeFlowDataType::ImageRgb:
		_data = cv::Mat();
		break;
	case ENodeFlowDataType::Array:
		_data = cv::Mat();
		break;
	case ENodeFlowDataType::Keypoints:
		_data = KeyPoints();
		break;
	case ENodeFlowDataType::Matches:
		_data = Matches();
		break;
#if defined(HAVE_OPENCL)
	case ENodeFlowDataType::DeviceImage:
	case ENodeFlowDataType::DeviceImageMono:
	case ENodeFlowDataType::DeviceImageRgb:
		_data = clw::Image2D();
		break;
	case ENodeFlowDataType::DeviceArray:
		_data = DeviceArray();
		break;
#endif
	case ENodeFlowDataType::Invalid:
	default:
		break;
	}
}

NodeFlowData::~NodeFlowData()
{
}

void NodeFlowData::saveToDisk(const std::string& filename) const
{
	if(_type == ENodeFlowDataType::Image
		|| _type == ENodeFlowDataType::ImageMono
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
		|| _type == ENodeFlowDataType::ImageMono
		|| _type == ENodeFlowDataType::ImageRgb)
	{
		const cv::Mat& img = boost::get<cv::Mat>(_data);
		if(!img.empty())
			return true;
	}
	return false;
}

bool NodeFlowData::isConvertible(ENodeFlowDataType from, 
								 ENodeFlowDataType to) const
{
	if(from == to)
		return true;

	switch(from)
	{
	case ENodeFlowDataType::Image:
		if(to == ENodeFlowDataType::ImageMono)
			return boost::get<cv::Mat>(_data).channels() == 1;
		else if(to == ENodeFlowDataType::ImageRgb)
			return boost::get<cv::Mat>(_data).channels() == 3;
		break;
	case ENodeFlowDataType::ImageMono:
		if(to == ENodeFlowDataType::Image)
			return boost::get<cv::Mat>(_data).channels() == 1;
		break;
	case ENodeFlowDataType::ImageRgb:
		if(to == ENodeFlowDataType::Image)
			return boost::get<cv::Mat>(_data).channels() == 3;
		break;
	case ENodeFlowDataType::Invalid:
	case ENodeFlowDataType::Array:
	case ENodeFlowDataType::Keypoints:
	case ENodeFlowDataType::Matches:
#if defined(HAVE_OPENCL)
	case ENodeFlowDataType::DeviceImage:
		if(to == ENodeFlowDataType::DeviceImageMono)
			return boost::get<clw::Image2D>(_data).bytesPerElement() == 1;
		else if(to == ENodeFlowDataType::DeviceImageRgb)
			return boost::get<clw::Image2D>(_data).bytesPerElement() == 4;
		break;
	case ENodeFlowDataType::DeviceImageMono:
		if(to == ENodeFlowDataType::DeviceImage)
			return boost::get<clw::Image2D>(_data).bytesPerElement() == 1;
		break;
	case ENodeFlowDataType::DeviceImageRgb:
		if(to == ENodeFlowDataType::DeviceImage)
			return boost::get<clw::Image2D>(_data).bytesPerElement() == 4;
		break;
	case ENodeFlowDataType::DeviceArray:
#endif
		break;
	}

	return false;
}

cv::Mat& NodeFlowData::getImage()
{ 
	return getType<ENodeFlowDataType::Image, cv::Mat>();
}

const cv::Mat& NodeFlowData::getImage() const 
{ 
	return getType<ENodeFlowDataType::Image, cv::Mat>();
}

cv::Mat& NodeFlowData::getImageMono()
{
	return getType<ENodeFlowDataType::ImageMono, cv::Mat>();
}

const cv::Mat& NodeFlowData::getImageMono() const
{
	return getType<ENodeFlowDataType::ImageMono, cv::Mat>();
}

cv::Mat& NodeFlowData::getImageRgb()
{
	return getType<ENodeFlowDataType::ImageRgb, cv::Mat>();
}

const cv::Mat& NodeFlowData::getImageRgb() const
{
	return getType<ENodeFlowDataType::ImageRgb, cv::Mat>();
}

KeyPoints& NodeFlowData::getKeypoints()
{
	return getType<ENodeFlowDataType::Keypoints, KeyPoints>();
}

const KeyPoints& NodeFlowData::getKeypoints() const
{
	return getType<ENodeFlowDataType::Keypoints, KeyPoints>();
}

Matches& NodeFlowData::getMatches()
{
	return getType<ENodeFlowDataType::Matches, Matches>();
}

const Matches& NodeFlowData::getMatches() const
{
	return getType<ENodeFlowDataType::Matches, Matches>();
}

cv::Mat& NodeFlowData::getArray()
{
	return getType<ENodeFlowDataType::Array, cv::Mat>();
}

const cv::Mat& NodeFlowData::getArray() const
{
	return getType<ENodeFlowDataType::Array, cv::Mat>();
}

#if defined(HAVE_OPENCL)

clw::Image2D& NodeFlowData::getDeviceImage()
{
	return getType<ENodeFlowDataType::DeviceImage, clw::Image2D>();
}

const clw::Image2D& NodeFlowData::getDeviceImage() const
{
	return getType<ENodeFlowDataType::DeviceImage, clw::Image2D>();
}

clw::Image2D& NodeFlowData::getDeviceImageMono()
{
	return getType<ENodeFlowDataType::DeviceImageMono, clw::Image2D>();
}

const clw::Image2D& NodeFlowData::getDeviceImageMono() const
{
	return getType<ENodeFlowDataType::DeviceImageMono, clw::Image2D>();
}

clw::Image2D& NodeFlowData::getDeviceImageRgb()
{
	return getType<ENodeFlowDataType::DeviceImageRgb, clw::Image2D>();
}

const clw::Image2D& NodeFlowData::getDeviceImageRgb() const
{
	return getType<ENodeFlowDataType::DeviceImageRgb, clw::Image2D>();
}

DeviceArray& NodeFlowData::getDeviceArray()
{
	return getType<ENodeFlowDataType::DeviceArray, DeviceArray>();
}

const DeviceArray& NodeFlowData::getDeviceArray() const
{
	return getType<ENodeFlowDataType::DeviceArray, DeviceArray>();
}

#endif

template<ENodeFlowDataType RequestedType, class Type>
Type& NodeFlowData::getType()
{
	if(!isConvertible(_type, RequestedType))
		throw boost::bad_get();
	return boost::get<Type>(_data);
}

template<ENodeFlowDataType RequestedType, class Type> 
const Type& NodeFlowData::getType() const
{
	if(!isConvertible(_type, RequestedType))
		throw boost::bad_get();
	return boost::get<Type>(_data);
}
