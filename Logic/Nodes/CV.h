#pragma once

#include "Prerequisites.h"

namespace cv {
class Mat;
}

namespace cvu  {

enum class EStructuringElementType
{
	Rectangle,
	Ellipse,
	Cross
};

enum class EBayerCode
{
	BG,
	GB,
	RG,
	GR,
};

inline int bayerCodeGray(EBayerCode code)
{
	switch(code)
	{
	case EBayerCode::BG: return CV_BayerBG2GRAY;
	case EBayerCode::GB: return CV_BayerGB2GRAY;
	case EBayerCode::RG: return CV_BayerRG2GRAY;
	case EBayerCode::GR: return CV_BayerGR2GRAY;
	default: return CV_BayerBG2GRAY;
	}
}

inline int bayerCodeRgb(EBayerCode code)
{
	switch(code)
	{
	case EBayerCode::BG: return CV_BayerBG2BGR;
	case EBayerCode::GB: return CV_BayerGB2BGR;
	case EBayerCode::RG: return CV_BayerRG2BGR;
	case EBayerCode::GR: return CV_BayerGR2BGR;
	default: return CV_BayerBG2GRAY;
	}
}

cv::Mat standardStructuringElement(int xradius, int yradius, 
	EStructuringElementType type, int rotation);

enum class EPredefinedConvolutionType
{
	/// [0 0 0]
	/// [0 1 0]
	/// [0 0 0]
	NoOperation,

	/// [1 1 1]
	/// [1 1 1]
	/// [1 1 1]
	Average,

	/// [1 2 1]
	/// [2 4 2]
	/// [1 2 1]
	Gauss,

	/// [-1 -1 -1]
	/// [-1  9 -1]
	/// [-1 -1 -1]
	MeanRemoval,

	/// [0 0  0]
	/// [0 1  0]
	/// [0 0 -1]
	RobertsCross45,

	/// [0  0 0]
	/// [0  0 1]
	/// [0 -1 0]
	RobertsCross135,

	/// [ 0 -1  0]
	/// [-1  4 -1]
	/// [ 0 -1  0]
	Laplacian,

	/// [ 1  1  1]
	/// [ 0  0  0]
	/// [-1 -1 -1]
	PrewittHorizontal,

	/// [-1 0 1]
	/// [-1 0 1]
	/// [-1 0 1]
	PrewittVertical,

	/// [-1 -2 -1]
	/// [ 0  0  0]
	/// [ 1  2  1]
	SobelHorizontal,

	/// [-1 0 1]
	/// [-2 0 2]
	/// [-1 0 1]
	SobelVertical
};

cv::Mat predefinedConvolutionKernel(EPredefinedConvolutionType type);

}