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

	virtual void configuration(NodeConfig& nodeConfig) const
	{
		static const InputSocketConfig* in_config = nullptr;
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
		};

		nodeConfig.description = "Loads image from a given location";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
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

	virtual void configuration(NodeConfig& nodeConfig) const
	{
		static const InputSocketConfig in_config[] = {
			{ "input", "Input", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
		};

		nodeConfig.description = "Performs Gaussian blur on input image";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class CannyEdgeDetectorNodeType : public NodeType
{
public:
	virtual void execute(NodeSocketReader* reader, NodeSocketWriter* writer)
	{
		const cv::Mat& input = reader->readSocket(0);
		cv::Mat& output = writer->lockSocket(0);

		if(input.rows == 0 || input.cols == 0)
		{
			output = cv::Mat();
			return;
		}

		cv::Canny(input, output, 3, 90);
	}

	virtual void configuration(NodeConfig& nodeConfig) const
	{
		static const InputSocketConfig in_config[] = {
			{ "input", "Input", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
		};

		nodeConfig.description = "Detects edges in input image using Canny detector";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

REGISTER_NODE("CannyEdgeDetector", CannyEdgeDetectorNodeType)
REGISTER_NODE("GaussianBlur", GaussianBlurNodeType)
REGISTER_NODE("ImageFromFile", ImageFromFileNodeType)
