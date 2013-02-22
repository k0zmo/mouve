#pragma once

#include "Prerequisites.h"
#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include <QDebug>

class ImageFromFileNodeType : public NodeType
{
public: 
	ImageFromFileNodeType()
		: filePath("lena.jpg")
	{
	}

	bool property(PropertyID propId, const QVariant& newValue) override
	{
		if(propId == 0)
		{
			filePath = newValue.toString().toStdString();
			return true;
		}

		return false;
	}

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		Q_UNUSED(reader);

		qDebug() << "Executing `Image from file` node";		

		cv::Mat& output = writer->lockSocket(0);

		output = cv::imread(filePath, CV_LOAD_IMAGE_GRAYSCALE);
	}

	void configuration(NodeConfig& nodeConfig) const  override
	{
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Filepath, "File path", QVariant(), "" },
			{ EPropertyType::Unknown, "", QVariant(), "" }
		};

		nodeConfig.description = "Loads image from a given location";
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	std::string filePath;
};

class GaussianBlurNodeType : public NodeType
{
public:
	GaussianBlurNodeType()
		: kernelSize(5)
		, sigma(10.0)
	{
	}

	bool property(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_KernelSize:
			{
				int ksize = newValue.toInt();
				if(ksize >=3 && (ksize & 1) == 1)
				{
					kernelSize = ksize;
					return true;
				}
				else
				{
					return false;
				}
			}			
		case ID_Sigma:
			sigma = newValue.toDouble();
			return true;
		}

		return false;
	}

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		qDebug() << "Executing `Gaussian blur` node";

		const cv::Mat& input = reader->readSocket(0);
		cv::Mat& output = writer->lockSocket(0);

		cv::GaussianBlur(input, output, cv::Size(kernelSize,kernelSize), sigma, 0);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ "input", "Input", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Integer, "Kernel size", QVariant(5), "min=3;max=21;step=2" },
			{ EPropertyType::Double, "Sigma", QVariant(10.0), "min=0.0" },
			{ EPropertyType::Unknown, "", QVariant(), "" }
		};

		nodeConfig.description = "Performs Gaussian blur on input image";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	int kernelSize;
	double sigma;

private:
	enum EPropertyID
	{
		ID_KernelSize,
		ID_Sigma
	};
};

class CannyEdgeDetectorNodeType : public NodeType
{
public:
	CannyEdgeDetectorNodeType()
		: threshold(3)
		, ratio(3)
		//, apertureSize(3)
	{
	}

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		qDebug() << "Executing `Canny edge detector` node";

		const cv::Mat& input = reader->readSocket(0);
		cv::Mat& output = writer->lockSocket(0);

		if(input.rows == 0 || input.cols == 0)
		{
			output = cv::Mat();
			return;
		}

		cv::Canny(input, output, threshold, threshold*ratio, 3/*apertureSize*/);
	}

	bool property(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Threshold:
			threshold = newValue.toDouble();
			return true;
		case ID_Ratio:
			ratio = newValue.toDouble();
			return true;
		}

		return false;
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ "input", "Input", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Double, "Threshold", QVariant(3.0), "min=0.0;max=100.0" },
			{ EPropertyType::Double, "Ratio", QVariant(3.0), "min=0.0" },
			//{ EPropertyType::Integer, "Sobel operator size", QVariant(3), "min=1;step=2" },
			{ EPropertyType::Unknown, "", QVariant(), "" }
		};

		nodeConfig.description = "Detects edges in input image using Canny detector";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	double threshold;
	double ratio;
	//int apertureSize;

private:
	enum EPropertyID
	{
		ID_Threshold,
		ID_Ratio,
		//ID_ApertureSize
	};
};

class AddNodeType : public NodeType
{
public:
	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
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

	void configuration(NodeConfig& nodeConfig) const override
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
	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
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

	void configuration(NodeConfig& nodeConfig) const override
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
	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		qDebug() << "Executing `Negate` node";
		const cv::Mat& src = reader->readSocket(0);
		cv::Mat& dst = writer->lockSocket(0);

		if(dst.type() == CV_8U)
			dst = 255 - src;
	}

	void configuration(NodeConfig& nodeConfig) const override
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
