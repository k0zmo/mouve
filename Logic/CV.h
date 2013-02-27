#pragma once

#include "Prerequisites.h"

namespace cvu  {

enum class EStructuringElementType
{
	Rectangle,
	Ellipse,
	Cross
};

MOUVE_LOGIC_EXPORT cv::Mat standardStructuringElement(int xradius,
								   int yradius,
								   EStructuringElementType type, 
								   int rotation);

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

MOUVE_LOGIC_EXPORT cv::Mat predefinedConvolutionKernel(EPredefinedConvolutionType type);

}