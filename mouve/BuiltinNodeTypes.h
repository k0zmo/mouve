#pragma once

#include "Prerequisites.h"
#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <qglobal.h>

class ImageFromFileNodeType : public NodeType
{
public: 
	virtual void execute(NodeSocketReader* reader, NodeSocketWriter* writer)
	{
		Q_UNUSED(reader);

		qDebug() << "Executing `Image from file` node";		

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
		qDebug() << "Executing `Gaussian blur` node";

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
		qDebug() << "Executing `Canny edge detector` node";

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

class AddNodeType : public NodeType
{
public:
	virtual void execute(NodeSocketReader* reader, NodeSocketWriter* writer)
	{
		qDebug() << "Executing `Add` node";
		const cv::Mat& src1 = reader->readSocket(0);
		const cv::Mat& src2 = reader->readSocket(1);
		cv::Mat& dst = writer->lockSocket(0);

		if((src1.rows == 0 || src1.cols == 0) ||
			(src2.rows == 0 || src2.cols == 0))
		{
			dst = cv::Mat();
			return;
		}

		cv::add(src1, src2, dst);
	}

	virtual void configuration(NodeConfig& nodeConfig) const
	{
		static const InputSocketConfig in_config[] = {
			{ "source1", "Source 1", "" },
			{ "source2", "Source 2", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
		};

		nodeConfig.description = "Adds one image from another";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class SubtractNodeType : public NodeType
{
public:
	virtual void execute(NodeSocketReader* reader, NodeSocketWriter* writer)
	{
		qDebug() << "Executing `Subtract` node";
		const cv::Mat& src1 = reader->readSocket(0);
		const cv::Mat& src2 = reader->readSocket(1);
		cv::Mat& dst = writer->lockSocket(0);

		if((src1.rows == 0 || src1.cols == 0) ||
		   (src2.rows == 0 || src2.cols == 0))
		{
			dst = cv::Mat();
			return;
		}

		cv::subtract(src1, src2, dst);
	}

	virtual void configuration(NodeConfig& nodeConfig) const
	{
		static const InputSocketConfig in_config[] = {
			{ "source1", "Source 1", "" },
			{ "source2", "Source 2", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
		};

		nodeConfig.description = "Subtracts one image from another";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class NegateNodeType : public NodeType
{
public:
	virtual void execute(NodeSocketReader* reader, NodeSocketWriter* writer)
	{
		qDebug() << "Executing `Negate` node";
		const cv::Mat& src = reader->readSocket(0);
		cv::Mat& dst = writer->lockSocket(0);

		if(dst.type() == CV_8U)
			dst = 255 - src;
	}

	virtual void configuration(NodeConfig& nodeConfig) const
	{
		static const InputSocketConfig in_config[] = {
			{ "source", "Source", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
		};

		nodeConfig.description = "Negates image";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

REGISTER_NODE("Negate", NegateNodeType)
REGISTER_NODE("Subtract", SubtractNodeType)
REGISTER_NODE("Add", AddNodeType)
REGISTER_NODE("Canny edge detector", CannyEdgeDetectorNodeType)
REGISTER_NODE("Gaussian blur", GaussianBlurNodeType)
REGISTER_NODE("Image from file", ImageFromFileNodeType)
