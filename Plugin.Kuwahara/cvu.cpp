#include "cvu.h"

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
void KuwaharaFilter_impl(int x, int y, const cv::Mat& src, cv::Mat& dst, int radius)
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

void KuwaharaFilter(cv::InputArray src_, cv::OutputArray dst_, int radius)
{
	cv::Mat src  = src_.getMat();
	cv::Mat& dst = dst_.getMatRef();
	dst.create(src.size(), src.type());

#if 1
		for(int y = radius; y < src.rows - radius; ++y)
			for(int x = radius; x < src.cols - radius; ++x)
				KuwaharaFilter_impl<cvu::NoClampPolicy>(x, y, src, dst, radius);

		for(int y = 0; y < radius; ++y)
			for(int x = 0; x < src.cols; ++x)
				KuwaharaFilter_impl<ClampToEdgePolicy>(x, y, src, dst, radius);
		for(int y = src.rows - radius; y < src.rows; ++y)
			for(int x = 0; x < src.cols; ++x)
				KuwaharaFilter_impl<ClampToEdgePolicy>(x, y, src, dst, radius);
		for(int y = radius; y < src.rows - radius; ++y)
			for(int x = 0; x < radius; ++x)
				KuwaharaFilter_impl<ClampToEdgePolicy>(x, y, src, dst, radius);
		for(int y = radius; y < src.rows - radius; ++y)
			for(int x = src.cols - radius; x < src.cols; ++x)
				KuwaharaFilter_impl<ClampToEdgePolicy>(x, y, src, dst, radius);
#else
		for(int y = 0; y < src.rows; ++y)
			for(int x = 0; x < src.cols; ++x)
				KuwaharaFilter_impl<ClampToEdgePolicy>(x, y, src, dst, radius);
#endif
}
}