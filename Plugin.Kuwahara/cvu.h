#pragma once

#include <opencv2/core/core.hpp>

namespace cvu
{
void KuwaharaFilter(cv::InputArray src, cv::OutputArray dst, int radius);
// Based on paper Artistic edge and corner enhancing smoothing, G. Papari, N. Petkov, P. Campisi
void generalizedKuwaharaFilter(cv::InputArray src_, cv::OutputArray dst_,
							   int radius = 6, int N = 8,
							   float smoothing = 0.3333f);
// Based on paper Image and Video Abstraction by Anisotropic Kuwahara Filtering,
// J. E. Kyprianidis, H. Kang, J. Dollner
void anisotropicKuwaharaFilter(cv::InputArray src_, cv::OutputArray dst_,
							   int radius = 6, int N = 8,
							   float smoothing = 0.3333f);

void getGeneralizedKuwaharaKernel(cv::OutputArray kernel, int N, float smoothing);

}