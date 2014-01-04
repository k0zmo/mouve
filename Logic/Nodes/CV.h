#pragma once

#include "../Prerequisites.h"

#include <opencv2/core/core.hpp>

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
	GR
};

int bayerCodeGray(EBayerCode code);
int bayerCodeRgb(EBayerCode code);

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
	SobelVertical,

	/// [-3 -10 -3]
	/// [ 0  0   0]
	/// [ 3  10  3]
	ScharrHorizontal,

	/// [-3  0 3 ]
	/// [-10 0 10]
	/// [-3  0 3 ]
	ScharrVertical
};

cv::Mat predefinedConvolutionKernel(EPredefinedConvolutionType type);

// Lambda-aware parallel loop invoker based on cv::parallel_for
template<typename Body>
struct ParallelLoopInvoker : public cv::ParallelLoopBody
{
	ParallelLoopInvoker(const Body& body)
		: _body(body)
	{
	}

	void operator()(const cv::Range& range) const override
	{
		_body(range);
	}

private:
	Body _body;
};

template<typename Body>
void parallel_for(const cv::Range& range, const Body& body)
{
	ParallelLoopInvoker<Body> loopInvoker(body);
	cv::parallel_for_(range, loopInvoker);
}

}
