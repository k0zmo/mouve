#pragma once

#include <opencv2/core/core.hpp>

namespace cvu
{
void KuwaharaFilter(cv::InputArray src, cv::OutputArray dst, int radius);
}