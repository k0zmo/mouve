#pragma once

#include "Prerequisites.h"

#include <boost/variant.hpp>

// OpenCV
#include <opencv2/core/core.hpp>
#include <opencv2/features2d/features2d.hpp>

#if defined(HAVE_OPENCL)
// Device data
#  include "OpenCL/DeviceArray.h"
#endif

using std::vector;

struct KeyPoints
{
	vector<cv::KeyPoint> kpoints;
	// could use shared_ptr BUT cv::Mat already
	// got reference counting under a table
	cv::Mat image;
};

struct Matches
{
	vector<cv::Point2f> queryPoints;
	vector<cv::Point2f> trainPoints;
	// same here
	cv::Mat queryImage;
	cv::Mat trainImage;
};

enum class ENodeFlowDataType : int
{
	Invalid,
	/// Image - 1 or 3 channels
	/// Node with such sockets supports both formats
	Image,
	/// Image with only 1 channel (monochromatic)
	ImageMono,
	/// Image with 3 channels (RGB)
	ImageRgb,
	Array,
	Keypoints,
	Matches
#if defined(HAVE_OPENCL)
	// Gpu part
	/// TODO: Could merge this with above ones and add bool flag
	///       indicating if it's in device or host memory
	,DeviceImage,
	DeviceImageMono,
	// In fact, its RGBA (BGRA, ARGB), as RGB noone seems to support
	DeviceImageRgb,
	DeviceArray,
#endif
};

class LOGIC_EXPORT NodeFlowData
{
	typedef boost::variant<
		cv::Mat,
		KeyPoints,
		Matches
	#if defined(HAVE_OPENCL)
		,clw::Image2D, // TODO - replace this with some thin wrapper
		DeviceArray
	#endif
	> flow_data;

public:
	NodeFlowData();
	NodeFlowData(ENodeFlowDataType dataType);
	
	//NodeFlowData& operator=(const NodeFlowData& other);
	//NodeFlowData& operator=(NodeFlowData&& other);
	//NodeFlowData(const NodeFlowData& other);
	//NodeFlowData(NodeFlowData&& other);

	~NodeFlowData();

	void saveToDisk(const std::string& filename) const;
	bool isValidImage() const;

	// Host data
	cv::Mat& getImage();
	const cv::Mat& getImage() const;

	cv::Mat& getImageMono();
	const cv::Mat& getImageMono() const;

	cv::Mat& getImageRgb();
	const cv::Mat& getImageRgb() const;

	KeyPoints& getKeypoints();
	const KeyPoints& getKeypoints() const;

	Matches& getMatches();
	const Matches& getMatches() const;

	cv::Mat& getArray();
	const cv::Mat& getArray() const;

	// Device data
#if defined(HAVE_OPENCL)
	clw::Image2D& getDeviceImage();
	const clw::Image2D& getDeviceImage() const;

	clw::Image2D& getDeviceImageMono();
	const clw::Image2D& getDeviceImageMono() const;

	clw::Image2D& getDeviceImageRgb();
	const clw::Image2D& getDeviceImageRgb() const;

	DeviceArray& getDeviceArray();
	const DeviceArray& getDeviceArray() const;
#endif

	bool isValid() const;
	bool canReturn(ENodeFlowDataType type) const;

	ENodeFlowDataType type() const;

private:
	ENodeFlowDataType _type;
	flow_data _data;

	bool isConvertible(ENodeFlowDataType from, 
		ENodeFlowDataType to) const;

	template<ENodeFlowDataType RequestedType, class Type> Type& getType();
	template<ENodeFlowDataType RequestedType, class Type> const Type& getType() const;
};

inline bool NodeFlowData::isValid() const
{ return _type != ENodeFlowDataType::Invalid; }

inline bool NodeFlowData::canReturn(ENodeFlowDataType type) const
{ return _type == type && isValid(); }

inline ENodeFlowDataType NodeFlowData::type() const
{ return _type; }


