#pragma once

#include "Prerequisites.h"
#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <iostream>

class ZeroInputOneOutputNodeType : NodeType // 1
{
public:
	virtual void execute(NodeSocketReader*, NodeSocketWriter*)
	{
		std::cout << "[ZeroInputOneOutputNodeType] Processing... \n";
		std::cout << "[ZeroInputOneOutputNodeType] Writing to 0 output socket... \n\n";
	}

	virtual SocketID numInputSockets() const { return 0; }
	virtual SocketID numOutputSockets() const { return 1; }
};

class ZeroInputTwoOutputNodeType : NodeType // 2
{
public:
	virtual void execute(NodeSocketReader*, NodeSocketWriter*)
	{
		std::cout << "[ZeroInputTwoOutputNodeType] Processing... \n";
		std::cout << "[ZeroInputTwoOutputNodeType] Writing to 0 output socket... \n";
		std::cout << "[ZeroInputTwoOutputNodeType] Writing to 1 output socket... \n\n";
	}

	virtual SocketID numInputSockets() const { return 0; }
	virtual SocketID numOutputSockets() const { return 2; }
};

class OneInputOneOutputNodeType : NodeType // 3
{
public:
	virtual void execute(NodeSocketReader*, NodeSocketWriter*)
	{
		std::cout << "[OneInputOneOutputNodeType] Reading from 0 output socket... \n";
		std::cout << "[OneInputOneOutputNodeType] Processing... \n";
		std::cout << "[OneInputOneOutputNodeType] Writing to 0 output socket... \n\n";
	}

	virtual SocketID numInputSockets() const { return 1; }
	virtual SocketID numOutputSockets() const { return 1; }
};

class OneInputTwoOutputNodeType : NodeType // 4
{
public:
	virtual void execute(NodeSocketReader*, NodeSocketWriter*)
	{
		std::cout << "[OneInputTwoOutputNodeType] Reading from 0 output socket... \n";
		std::cout << "[OneInputTwoOutputNodeType] Processing... \n";
		std::cout << "[OneInputTwoOutputNodeType] Writing to 0 output socket... \n";
		std::cout << "[OneInputTwoOutputNodeType] Writing to 1 output socket... \n\n";
	}

	virtual SocketID numInputSockets() const { return 1; }
	virtual SocketID numOutputSockets() const { return 2; }
};

class TwoInputOneOutputNodeType : NodeType // 5
{
public:
	virtual void execute(NodeSocketReader*, NodeSocketWriter*)
	{
		std::cout << "[TwoInputOneOutputNodeType] Reading from 0 output socket... \n";
		std::cout << "[TwoInputOneOutputNodeType] Reading from 1 output socket... \n";
		std::cout << "[TwoInputOneOutputNodeType] Processing... \n";
		std::cout << "[TwoInputOneOutputNodeType] Writing to 0 output socket... \n\n";
	}

	virtual SocketID numInputSockets() const { return 2; }
	virtual SocketID numOutputSockets() const { return 1; }
};

class TwoInputZeroOutputNodeType : NodeType // 6
{
public:
	virtual void execute(NodeSocketReader*, NodeSocketWriter*)
	{
		std::cout << "[TwoInputZeroOutputNodeType] Reading from 0 output socket... \n";
		std::cout << "[TwoInputZeroOutputNodeType] Reading from 1 output socket... \n";
		std::cout << "[TwoInputZeroOutputNodeType] Processing... \n\n";
	}

	virtual SocketID numInputSockets() const { return 2; }
	virtual SocketID numOutputSockets() const { return 0; }
};

// -----------------------------------------------------------------------------

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

		// lub
		// cv::Mat output;
		// ..
		// .. process output
		// ..
		// writer->writeSocket(0, output);
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