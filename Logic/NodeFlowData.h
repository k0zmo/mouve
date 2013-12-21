#pragma once

#include "Prerequisites.h"

#include "boost/variant.hpp"

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
	Image,
	ImageRgb,
	Array,
	Keypoints,
	Matches
#if defined(HAVE_OPENCL)
	// Gpu part
	/// TODO: Could merge this with above ones and add bool flag
	///       indicating if it's in device or host memory
	,DeviceImage,
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

	DeviceArray& getDeviceArray();
	const DeviceArray& getDeviceArray() const;
#endif

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

