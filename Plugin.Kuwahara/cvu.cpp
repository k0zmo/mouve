#include "cvu.h"

#undef min
#undef max

#include <cassert>
#include <algorithm>
#include <limits>

#include <opencv2/imgproc/imgproc.hpp>

namespace cvu
{
struct ClampToEdgePolicy
{
	static int clamp(int x, int size)
	{
		//return x < 0 ? 0 : x >= width ? width - 1: x;
		return std::min(std::max(x, 0), size - 1);
	}
};

struct NoClampPolicy
{
	static int clamp(int x, int size)
	{
		(void)size;
		return x;
	}
};

template<typename ClampPolicy>
void KuwaharaFilter_gray_pix(int x, int y, const cv::Mat& src, cv::Mat& dst, int radius)
{
	float mean[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
	float sqmean[4] = { 0.0f, 0.0f, 0.0f, 0.0f };

	const int height = src.rows;
	const int width = src.cols;

	// top-left
	for(int j = -radius; j <= 0; ++j)
	{
		for(int i = -radius; i <= 0; ++i)
		{
			float pix = static_cast<float>(src.at<uchar>(
				ClampPolicy::clamp(y + j, height),
				ClampPolicy::clamp(x + i, width)));
			mean[0] += pix;
			sqmean[0] += pix * pix;
		}
	}

	// top-right
	for(int j = -radius; j <= 0; ++j)
	{
		for(int i = 0; i <= radius; ++i)
		{
			float pix = static_cast<float>(src.at<uchar>(
				ClampPolicy::clamp(y + j, height),
				ClampPolicy::clamp(x + i, width)));
			mean[1] += pix;
			sqmean[1] += pix * pix;
		}
	}

	// bottom-right
	for(int j = 0; j <= radius; ++j)
	{
		for(int i = 0; i <= radius; ++i)
		{
			float pix = static_cast<float>(src.at<uchar>(
				ClampPolicy::clamp(y + j, height),
				ClampPolicy::clamp(x + i, width)));
			mean[2] += pix;
			sqmean[2] += pix * pix;
		}
	}

	// bottom-left
	for(int j = 0; j <= radius; ++j)
	{
		for(int i = -radius; i <= 0; ++i)
		{
			float pix = static_cast<float>(src.at<uchar>(
				ClampPolicy::clamp(y + j, height), 
				ClampPolicy::clamp(x + i, width)));
			mean[3] += pix;
			sqmean[3] += pix * pix;
		}
	}

	float n = static_cast<float>((radius + 1) * (radius + 1));
	float min_sigma = std::numeric_limits<float>::max();

	for(int k = 0; k < 4; ++k)
	{
		mean[k] /= n;
		sqmean[k] = std::abs(sqmean[k] / n -  mean[k]*mean[k]);
		if(sqmean[k] < min_sigma)
		{
			min_sigma = sqmean[k];
			dst.at<uchar>(y, x) = cv::saturate_cast<uchar>(mean[k]);
		}
	}
}

void KuwaharaFilter_gray(const cv::Mat& src, cv::Mat& dst, int radius)
{
#if 1
	for(int y = radius; y < src.rows - radius; ++y)
		for(int x = radius; x < src.cols - radius; ++x)
			KuwaharaFilter_gray_pix<cvu::NoClampPolicy>(x, y, src, dst, radius);

	for(int y = 0; y < radius; ++y)
		for(int x = 0; x < src.cols; ++x)
			KuwaharaFilter_gray_pix<ClampToEdgePolicy>(x, y, src, dst, radius);
	for(int y = src.rows - radius; y < src.rows; ++y)
		for(int x = 0; x < src.cols; ++x)
			KuwaharaFilter_gray_pix<ClampToEdgePolicy>(x, y, src, dst, radius);
	for(int y = radius; y < src.rows - radius; ++y)
		for(int x = 0; x < radius; ++x)
			KuwaharaFilter_gray_pix<ClampToEdgePolicy>(x, y, src, dst, radius);
	for(int y = radius; y < src.rows - radius; ++y)
		for(int x = src.cols - radius; x < src.cols; ++x)
			KuwaharaFilter_gray_pix<ClampToEdgePolicy>(x, y, src, dst, radius);
#else
	for(int y = 0; y < src.rows; ++y)
		for(int x = 0; x < src.cols; ++x)
			KuwaharaFilter_gray_pix<ClampToEdgePolicy>(x, y, src, dst, radius);
#endif
}

template<typename ClampPolicy>
void KuwaharaFilter_bgr_pix(int x, int y, const cv::Mat& src, cv::Mat& dst, int radius)
{
	cv::Vec3f mean[4] = { 0 };
	cv::Vec3f sqmean[4] = { 0 };

	const int height = src.rows;
	const int width = src.cols;

	// top-left
	for(int j = -radius; j <= 0; ++j)
	{
		for(int i = -radius; i <= 0; ++i)
		{
			cv::Vec3f pix = static_cast<cv::Vec3f>(src.at<cv::Vec3b>(
				ClampPolicy::clamp(y + j, height),
				ClampPolicy::clamp(x + i, width)));
			mean[0] += pix;
			sqmean[0] += pix.mul(pix);
		}
	}

	// top-right
	for(int j = -radius; j <= 0; ++j)
	{
		for(int i = 0; i <= radius; ++i)
		{
			cv::Vec3f pix = static_cast<cv::Vec3f>(src.at<cv::Vec3b>(
				ClampPolicy::clamp(y + j, height),
				ClampPolicy::clamp(x + i, width)));
			mean[1] += pix;
			sqmean[1] += pix.mul(pix);
		}
	}

	// bottom-right
	for(int j = 0; j <= radius; ++j)
	{
		for(int i = 0; i <= radius; ++i)
		{
			cv::Vec3f pix = static_cast<cv::Vec3f>(src.at<cv::Vec3b>(
				ClampPolicy::clamp(y + j, height),
				ClampPolicy::clamp(x + i, width)));
			mean[2] += pix;
			sqmean[2] += pix.mul(pix);
		}
	}

	// bottom-left
	for(int j = 0; j <= radius; ++j)
	{
		for(int i = -radius; i <= 0; ++i)
		{
			cv::Vec3f pix = static_cast<cv::Vec3f>(src.at<cv::Vec3b>(
				ClampPolicy::clamp(y + j, height),
				ClampPolicy::clamp(x + i, width)));
			mean[3] += pix;
			sqmean[3] += pix.mul(pix);
		}
	}

	float n = static_cast<float>((radius + 1) * (radius + 1));
	float min_sigma = std::numeric_limits<float>::max();

	for(int k = 0; k < 4; ++k)
	{
		mean[k] /= n;
		sqmean[k] = sqmean[k] / n -  mean[k].mul(mean[k]);
		// HSI model (Lightness is the average of the three components)
		float sigma = std::abs(sqmean[k][0]) + std::abs(sqmean[k][1]) + std::abs(sqmean[k][2]);

		if(sigma < min_sigma)
		{
			min_sigma = sigma;
			dst.at<cv::Vec3b>(y, x) = cv::Vec3b(
				cv::saturate_cast<uchar>(mean[k][0]),
				cv::saturate_cast<uchar>(mean[k][1]),
				cv::saturate_cast<uchar>(mean[k][2]));
		}
	}
}

void KuwaharaFilter_bgr(const cv::Mat& src, cv::Mat& dst, int radius)
{
#if 1
	for(int y = radius; y < src.rows - radius; ++y)
		for(int x = radius; x < src.cols - radius; ++x)
			KuwaharaFilter_bgr_pix<cvu::NoClampPolicy>(x, y, src, dst, radius);

	for(int y = 0; y < radius; ++y)
		for(int x = 0; x < src.cols; ++x)
			KuwaharaFilter_bgr_pix<ClampToEdgePolicy>(x, y, src, dst, radius);
	for(int y = src.rows - radius; y < src.rows; ++y)
		for(int x = 0; x < src.cols; ++x)
			KuwaharaFilter_bgr_pix<ClampToEdgePolicy>(x, y, src, dst, radius);
	for(int y = radius; y < src.rows - radius; ++y)
		for(int x = 0; x < radius; ++x)
			KuwaharaFilter_bgr_pix<ClampToEdgePolicy>(x, y, src, dst, radius);
	for(int y = radius; y < src.rows - radius; ++y)
		for(int x = src.cols - radius; x < src.cols; ++x)
			KuwaharaFilter_bgr_pix<ClampToEdgePolicy>(x, y, src, dst, radius);
#else
	for(int y = 0; y < src.rows; ++y)
		for(int x = 0; x < src.cols; ++x)
			KuwaharaFilter_bgr_pix<ClampToEdgePolicy>(x, y, src, dst, radius);
#endif
}

void KuwaharaFilter(cv::InputArray src_, cv::OutputArray dst_, int radius)
{
	cv::Mat src  = src_.getMat();

	CV_Assert(src.depth() == CV_8U);
	CV_Assert(src.channels() == 3 || src.channels() == 1);

	cv::Mat& dst = dst_.getMatRef();
	dst.create(src.size(), src.type());

	if(src.type() == CV_8UC1)
		KuwaharaFilter_gray(src, dst, radius);
	else if(src.type() == CV_8UC3)
		KuwaharaFilter_bgr(src, dst, radius);
	else
		CV_Assert(false);
}
}