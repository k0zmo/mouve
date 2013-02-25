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

	bool setProperty(PropertyID propId, const QVariant& newValue) override
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

		qDebug() << "Executing 'Image from file' node";		

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
			{ EPropertyType::Filepath, "File path", QVariant(QString::fromStdString(filePath)), "" },
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
		: kernelRadius(2)
		, sigma(10.0)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_KernelSize:
			kernelRadius = newValue.toInt();
			return true;
		case ID_Sigma:
			sigma = newValue.toDouble();
			return true;
		}

		return false;
	}

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		qDebug() << "Executing 'Gaussian blur' node";

		const cv::Mat& input = reader->readSocket(0);
		cv::Mat& output = writer->lockSocket(0);

		cv::GaussianBlur(input, output, cv::Size(kernelRadius*2+1,kernelRadius*2+1), sigma, 0);
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
			// TODO: In future we might use slider
			{ EPropertyType::Integer, "Kernel radius", QVariant(kernelRadius), "min=1;max=20;step=1" },
			{ EPropertyType::Double, "Sigma", QVariant(sigma), "min=0.0" },
			{ EPropertyType::Unknown, "", QVariant(), "" }
		};

		nodeConfig.description = "Performs Gaussian blur on input image";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	int kernelRadius;
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
		: threshold(10)
		, ratio(3)
		//, apertureSize(3)
	{
	}

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		qDebug() << "Executing 'Canny edge detector' node";

		const cv::Mat& input = reader->readSocket(0);
		cv::Mat& output = writer->lockSocket(0);

		if(input.rows == 0 || input.cols == 0)
		{
			output = cv::Mat();
			return;
		}

		cv::Canny(input, output, threshold, threshold*ratio, 3/*apertureSize*/);
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
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
			{ EPropertyType::Double, "Threshold", QVariant(threshold), "min=0.0;max=100.0" },
			{ EPropertyType::Double, "Ratio", QVariant(ratio), "min=0.0" },
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
		qDebug() << "Executing 'Add' node";
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
		qDebug() << "Executing 'Subtract' node";
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
		qDebug() << "Executing 'Negate' node";
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

class BinarizationNodeType : public NodeType
{
public:
	BinarizationNodeType()
		: threshold(128)
		, inv(false)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Threshold:
			threshold = newValue.toInt();
			return true;
		case ID_Invert:
			inv = newValue.toBool();
			return true;
		}

		return false;
	}

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		qDebug() << "Executing 'Binarization' node";

		const cv::Mat& src = reader->readSocket(0);
		cv::Mat& dst = writer->lockSocket(0);

		if(src.rows == 0 || src.cols == 0 || threshold < 0 || threshold > 255)
		{
			dst = cv::Mat();
			return;
		}

		int type = inv ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY;
		cv::threshold(src, dst, (double) threshold, 255, type);
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
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Integer, "Threshold", QVariant(threshold), "min=0;max=255" },
			{ EPropertyType::Boolean, "Inverted", QVariant(inv), "" },
			{ EPropertyType::Unknown, "", QVariant(), "" }
		};

		nodeConfig.description = "Creates a binary image by segmenting pixel values to 0 or 1 depending on threshold value";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	int threshold;
	bool inv;

private:
	enum EPropertyID
	{
		ID_Threshold,
		ID_Invert
	};
};

#include "CV.h"

class StructuringElementNodeType : public NodeType
{
public:

	StructuringElementNodeType()
		: se(cvu::EStructuringElementType::Rectangle)
		, xradius(1)
		, yradius(1)
		, rotation(0)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_StructuringElementType:
			se = cvu::EStructuringElementType(newValue.toInt());
			return true;
		case ID_XRadius:
			xradius = newValue.toInt();
			return true;
		case ID_YRadius:
			yradius = newValue.toInt();
			return true;
		case ID_Rotation:
			rotation = newValue.toInt();
			return true;
		}

		return false;
	}

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		qDebug() << "Executing 'Structuring element' node";

		cv::Mat& kernel = writer->lockSocket(0);

		if(xradius == 0 || yradius == 0)
		{
			kernel = cv::Mat();
			return;
		}

		kernel = cvu::standardStructuringElement(xradius, yradius, se, rotation);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const OutputSocketConfig out_config[] = {
			{ "structuringElement", "Structuring element", "" },
			{ "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Enum, "SE shape", 
				QVariant(QStringList() << "Rectangle" << "Ellipse" << "Cross"), "" },
			{ EPropertyType::Integer, "Horizontal radius", QVariant(xradius), "min=1;max=75" },
			{ EPropertyType::Integer, "Vertical radius", QVariant(yradius), "min=1;max=75" },
			{ EPropertyType::Integer, "Rotation", QVariant(rotation), "min=0;max=359" },
			{ EPropertyType::Unknown, "", QVariant(), "" }
		};

		nodeConfig.description = "Generates a structuring element for a morphology "
			"operation with a given parameters describing its shape, size and rotation";
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		ID_StructuringElementType,
		ID_XRadius,
		ID_YRadius,
		ID_Rotation
	};

private:
	cvu::EStructuringElementType se;
	int xradius;
	int yradius;
	int rotation;
};

class MorphologyNodeType : public NodeType
{
public:
	MorphologyNodeType()
		: op(Erode)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Operation:
			op = EMorphologyOperation(newValue.toInt());
			return true;
		}

		return false;
	}

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		qDebug() << "Executing 'Morphology' node";

		const cv::Mat& src = reader->readSocket(0);
		const cv::Mat& se = reader->readSocket(1);
		cv::Mat& dst = writer->lockSocket(0);

		if(se.cols == 0 || se.rows == 0 || src.rows == 0 || src.cols == 0)
		{
			dst = cv::Mat();
			return;
		}
		
		cv::morphologyEx(src, dst, op, se);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ "source", "Source", "" },
			{ "source", "Structuring element", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			{ "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Enum, "Operation type", 
			QVariant(QStringList() << "Erode" << "Dilate" << "Open" << 
			"Close" << "Gradient" << "Top Hat" << "Black Hat"),
			"" },
			{ EPropertyType::Unknown, "", QVariant(), "" }
		};

		nodeConfig.description = "Performs a morphology operation on a given image";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EMorphologyOperation
	{
		Erode    = cv::MORPH_ERODE,
		Dilate   = cv::MORPH_DILATE,
		Open     = cv::MORPH_OPEN,
		Close    = cv::MORPH_CLOSE,
		Gradient = cv::MORPH_GRADIENT,
		TopHat   = cv::MORPH_TOPHAT,
		BlackHat = cv::MORPH_BLACKHAT
	};

	enum EPropertyID
	{
		ID_Operation
	};

	EMorphologyOperation op;
};

/*class MorphologyNodeType : public NodeType
{
public:
	MorphologyNodeType()
		: op(Erode)
		, se(cvu::EStructuringElementType::Rectangle)
		, xradius(1)
		, yradius(1)
		, rotation(0)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Operation:
			op = EMorphologyOperation(newValue.toInt());
			return true;
		case ID_StructuringElementType:
			se = cvu::EStructuringElementType(newValue.toInt());
			return true;
		case ID_XRadius:
			xradius = newValue.toInt();
			return true;
		case ID_YRadius:
			yradius = newValue.toInt();
			return true;
		case ID_Rotation:
			rotation = newValue.toInt();
			return true;
		}

		return false;
	}

	void execute(NodeSocketReader* reader, NodeSocketWriter* writer) override
	{
		qDebug() << "Executing 'Morphology' node";

		const cv::Mat& src = reader->readSocket(0);
		cv::Mat& dst = writer->lockSocket(0);
		cv::Mat& kernel = writer->lockSocket(1);

		if(xradius == 0 || yradius == 0)
		{
			dst = cv::Mat();
			kernel = cv::Mat();
			return;
		}

		kernel = cvu::standardStructuringElement(xradius, yradius, se, rotation);

		if(src.rows == 0 || src.cols == 0)
		{
			dst = cv::Mat();
			return;
		}
		
		cv::morphologyEx(src, dst, op, kernel);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ "source", "Source", "" },
			{ "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ "output", "Output", "" },
			// still need to think how to preview binary 0-1 image such as structuring element
			{ "structuringElement", "Structuring element", "" },
			{ "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Enum, "Operation type", 
			QVariant(QStringList() << "Erode" << "Dilate" << "Open" << 
				"Close" << "Gradient" << "Top Hat" << "Black Hat"),
			"" },
			{ EPropertyType::Enum, "SE shape", 
			QVariant(QStringList() << "Rectangle" << "Ellipse" << "Cross"), "" },
			{ EPropertyType::Integer, "Horizontal radius", QVariant(xradius), "min=1;max=75" },
			{ EPropertyType::Integer, "Vertical radius", QVariant(yradius), "min=1;max=75" },
			{ EPropertyType::Integer, "Rotation", QVariant(rotation), "min=0;max=359" },
			{ EPropertyType::Unknown, "", QVariant(), "" }
		};

		nodeConfig.description = "Performs a morphology operation on a given image";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EMorphologyOperation
	{
		Erode    = cv::MORPH_ERODE,
		Dilate   = cv::MORPH_DILATE,
		Open     = cv::MORPH_OPEN,
		Close    = cv::MORPH_CLOSE,
		Gradient = cv::MORPH_GRADIENT,
		TopHat   = cv::MORPH_TOPHAT,
		BlackHat = cv::MORPH_BLACKHAT
	};

	enum EPropertyID
	{
		ID_Operation,
		ID_StructuringElementType,
		ID_XRadius,
		ID_YRadius,
		ID_Rotation
	};

private:
	EMorphologyOperation op;
	cvu::EStructuringElementType se;
	int xradius;
	int yradius;
	int rotation;
};
*/

REGISTER_NODE("Negate", NegateNodeType)
REGISTER_NODE("Subtract", SubtractNodeType)
REGISTER_NODE("Add", AddNodeType)
REGISTER_NODE("Binarization", BinarizationNodeType)
REGISTER_NODE("Structuring element", StructuringElementNodeType)
REGISTER_NODE("Morphology op.", MorphologyNodeType)
REGISTER_NODE("Canny edge detector", CannyEdgeDetectorNodeType)
REGISTER_NODE("Gaussian blur", GaussianBlurNodeType)
REGISTER_NODE("Image from file", ImageFromFileNodeType)
