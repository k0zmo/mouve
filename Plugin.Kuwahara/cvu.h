#pragma once

#include <opencv2/core/core.hpp>

#undef min
#undef max

#include <algorithm>
#include <limits>

namespace cvu
{
void KuwaharaFilter(cv::InputArray src, cv::OutputArray dst, int radius);
}