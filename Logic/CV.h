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
}