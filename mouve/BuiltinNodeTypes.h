#pragma once

#include "Prerequisites.h"
#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

class ImageFromFileNodeType : public NodeType
{
public: 
	virtual void execute(NodeSocketReader* reader, NodeSocketWriter* writer)
	{
		(void) reader;
		const std::string filePath = "lena.jpg";
		cv::Mat& output = writer->lockSocket(0);

		output = cv::imread(filePath, CV_LOAD_IMAGE_GRAYSCALE);
	}

	virtual SocketID numInputSockets() const { return 0; }
	virtual SocketID numOutputSockets() const { return 1; }
};

class GaussianBlurNodeType : public NodeType
{
public:
	virtual void execute(NodeSocketReader* reader, NodeSocketWriter* writer)
	{
		const cv::Mat& input = reader->readSocket(0);
		cv::Mat& output = writer->lockSocket(0);

		cv::GaussianBlur(input, output, cv::Size(5,5), 10.0);
	}

	virtual SocketID numInputSockets() const { return 1; }
	virtual SocketID numOutputSockets() const { return 1; }
};

class CannyEdgeDetectorNodeType : public NodeType
{
public:
	virtual void execute(NodeSocketReader* reader, NodeSocketWriter* writer)
	{
		const cv::Mat& input = reader->readSocket(0);
		cv::Mat& output = writer->lockSocket(0);

		cv::Canny(input, output, 3, 90);
	}

	virtual SocketID numInputSockets() const { return 1; }
	virtual SocketID numOutputSockets() const { return 1; }
};

REGISTER_NODE("ImageFromFile", ImageFromFileNodeType)
REGISTER_NODE("GaussianBlur", GaussianBlurNodeType)
REGISTER_NODE("CannyEdgeDetector", CannyEdgeDetectorNodeType)
