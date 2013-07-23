#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/video/video.hpp>

#include "CV.h"

class MixtureOfGaussiansNodeType : public NodeType
{
public:
	MixtureOfGaussiansNodeType()
		: _history(200)
		, _nmixtures(5)
		, _backgroundRatio(0.7)
		, _learningRate(-1)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_History:
			_history = newValue.toInt();
			return true;
		case ID_NMixtures:
			_nmixtures = newValue.toInt();
			return true;
		case ID_BackgroundRatio:
			_backgroundRatio = newValue.toDouble();
			return true;
		case ID_LearningRate:
			_learningRate = newValue.toDouble();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_History: return _history;
		case ID_NMixtures: return _nmixtures;
		case ID_BackgroundRatio: return _backgroundRatio;
		case ID_LearningRate: return _learningRate;
		}

		return QVariant();
	}

	bool restart() override
	{
		_mog = cv::BackgroundSubtractorMOG(_history, _nmixtures, _backgroundRatio);
		return true;
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& source = reader.readSocket(0).getImage();
		cv::Mat& output = writer.acquireSocket(0).getImage();

		if(!source.data)
			return ExecutionStatus(EStatus::Ok);

		_mog(source, output, _learningRate);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "input", "Input", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Integer, "History frames", "min:1, max:500" },
			{ EPropertyType::Integer, "Number of mixtures",  "min:1, max:9" },
			{ EPropertyType::Double, "Background ratio", "min:0.01, max:0.99, step:0.01" },
			{ EPropertyType::Double, "Learning rate", "min:-1, max:1, step:0.01, decimals:3" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Gaussian Mixture-based Background/Foreground Segmentation Algorithm.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
		nodeConfig.flags = Node_HasState;
	}

private:
	enum EPropertyID
	{
		ID_History,
		ID_NMixtures,
		ID_BackgroundRatio,
		ID_LearningRate
	};

	cv::BackgroundSubtractorMOG _mog;
	int _history;
	int _nmixtures;
	double _backgroundRatio;
	double _learningRate;
};




class CannyEdgeDetectorNodeType : public NodeType
{
public:
	CannyEdgeDetectorNodeType()
		: _threshold(10)
		, _ratio(3)
	{
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_Threshold: return _threshold;
		case ID_Ratio: return _ratio;
		}

		return QVariant();
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Threshold:
			_threshold = newValue.toDouble();
			return true;
		case ID_Ratio:
			_ratio = newValue.toDouble();
			return true;
		}

		return false;
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& input = reader.readSocket(0).getImage();
		cv::Mat& output = writer.acquireSocket(0).getImage();

		if(input.rows == 0 || input.cols == 0)
			return ExecutionStatus(EStatus::Ok);;

		cv::Canny(input, output, _threshold, _threshold*_ratio, 3);

		return ExecutionStatus(EStatus::Ok);
	}


	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "input", "Input", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Double, "Threshold", "min:0.0, max:100.0, decimals:3" },
			{ EPropertyType::Double, "Ratio", "min:0.0, decimals:3" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Detects edges in input image using Canny detector";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		ID_Threshold,
		ID_Ratio,
	};

	double _threshold;
	double _ratio;
};


class BinarizationNodeType : public NodeType
{
public:
	BinarizationNodeType()
		: _threshold(128)
		, _inv(false)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Threshold:
			_threshold = newValue.toInt();
			return true;
		case ID_Invert:
			_inv = newValue.toBool();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_Threshold: return _threshold;
		case ID_Invert: return _inv;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		if(src.rows == 0 || src.cols == 0)
			return ExecutionStatus(EStatus::Ok);

		if(_threshold < 0 || _threshold > 255)
			return ExecutionStatus(EStatus::Error, "Bad threshold value");

		int type = _inv ? cv::THRESH_BINARY_INV : cv::THRESH_BINARY;
		cv::threshold(src, dst, (double) _threshold, 255, type);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Source", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Integer, "Threshold", "min:0, max:255" },
			{ EPropertyType::Boolean, "Inverted", "" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Creates a binary image by segmenting pixel values to 0 or 1 depending on threshold value";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		ID_Threshold,
		ID_Invert
	};

	int _threshold;
	bool _inv;
};

class OtsuThresholdingNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		if(src.rows == 0 || src.cols == 0 || src.type() != CV_8U)
			return ExecutionStatus(EStatus::Ok);

		cv::threshold(src, dst, 0, 255, cv::THRESH_OTSU);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Source", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "Creates a binary image by segmenting pixel values to 0 or 1 using histogram";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class EqualizeHistogramNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		if(src.rows == 0 || src.cols == 0)
			return ExecutionStatus(EStatus::Ok);

		cv::equalizeHist(src, dst);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Source", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};


class ClaheNodeType : public NodeType
{
public:
	ClaheNodeType()
		: _clipLimit(2.0)
		, _tilesGridSize(8)
	{
	}


	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_ClipLimit:
			_clipLimit = newValue.toDouble();
			return true;
		case ID_TilesGridSize:
			_tilesGridSize = newValue.toUInt();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_ClipLimit: return _clipLimit;
		case ID_TilesGridSize: return _tilesGridSize;
		}

		return QVariant();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
		clahe->setClipLimit(_clipLimit);
		clahe->setTilesGridSize(cv::Size(_tilesGridSize, _tilesGridSize));

		clahe->apply(src, dst);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Source", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Double, "Clip limit", "min:0.0" },
			{ EPropertyType::Integer, "Tiles size", "min:1" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Performs CLAHE (Contrast Limited Adaptive Histogram Equalization)";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		ID_ClipLimit,
		ID_TilesGridSize
	};

	double _clipLimit;
	int _tilesGridSize;
};

class DrawHistogramNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// inputs
		const cv::Mat& src = reader.readSocket(0).getImage();
		// outputs
		cv::Mat& canvas = writer.acquireSocket(0).getImageRgb();

		// validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		// Compute histogram
		const int bins = 256;
		const float range[] = { 0, 256 } ;
		const float* histRange = { range };
		const bool uniform = true; 
		const bool accumulate = false;
		cv::Mat hist;
		cv::calcHist(&src, 1, 0, cv::noArray(), hist, 1, &bins, &histRange, uniform, accumulate);

		// Draw histogram
		const int hist_h = 125;
		canvas = cv::Mat::zeros(hist_h, bins, CV_8UC3);
		cv::normalize(hist, hist, 0, canvas.rows, cv::NORM_MINMAX, -1, cv::noArray());

		for(int j = 0; j < bins-1; ++j)
		{
			cv::line(canvas, 
				cv::Point(j, hist_h),
				cv::Point(j, hist_h - (hist.at<float>(j))),
				cv::Scalar(200,200,200),
				1, 8, 0);
		}

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Source", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::ImageRgb, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class DownsampleNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		if(src.rows == 0 || src.cols == 0)
			return ExecutionStatus(EStatus::Ok);

		cv::pyrDown(src, dst, cv::Size(src.cols/2, src.rows/2));

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Source", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class UpsampleNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImage();
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		if(src.rows == 0 || src.cols == 0)
			return ExecutionStatus(EStatus::Ok);

		cv::pyrUp(src, dst, cv::Size(src.cols*2, src.rows*2));

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Source", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
	}
};

class BackgroundSubtractorNodeType : public NodeType
{
public:
	BackgroundSubtractorNodeType()
		: _alpha(0.92f)
		, _threshCoeff(3)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_Alpha:
			_alpha = newValue.toDouble();
			return true;
		case ID_ThresholdCoeff:
			_threshCoeff = newValue.toDouble();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_Alpha: return _alpha;
		case ID_ThresholdCoeff: return _threshCoeff;
		}
		return QVariant();
	}

	bool restart() override
	{
		_frameN1 = cv::Mat();
		_frameN2 = cv::Mat();
		return true;
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& frame = reader.readSocket(0).getImage();

		cv::Mat& background = writer.acquireSocket(0).getImage();
		cv::Mat& movingPixels = writer.acquireSocket(1).getImage();
		cv::Mat& threshold = writer.acquireSocket(2).getImage();

		if(frame.empty())
			return ExecutionStatus(EStatus::Ok);

		if(!_frameN2.empty()
			&& _frameN2.size() == frame.size())
		{
			if(background.empty()
				|| background.size() != frame.size())
			{
				background.create(frame.size(), CV_8UC1);
				background = cv::Scalar(0);
				movingPixels.create(frame.size(), CV_8UC1);
				movingPixels = cv::Scalar(0);
				threshold.create(frame.size(), CV_8UC1);
				threshold = cv::Scalar(127);
			}

			cvu::parallel_for(cv::Range(0, frame.rows), [&](const cv::Range& range)
			{
				for(int y = range.start; y < range.end; ++y)
				{
					for(int x = 0; x < frame.cols; ++x)
					{
						uchar thresh = threshold.at<uchar>(y, x);
						uchar pix = frame.at<uchar>(y, x);

						// Find moving pixels
						bool moving = std::abs(pix - _frameN1.at<uchar>(y, x)) > thresh
							&& std::abs(pix - _frameN2.at<uchar>(y, x)) > thresh;
						movingPixels.at<uchar>(y, x) = moving ? 255 : 0;

						const int minThreshold = 20;

						if(!moving)
						{
							// Update background image
							uchar newBackground = _alpha*float(background.at<uchar>(y, x)) + (1-_alpha)*float(pix);
							background.at<uchar>(y, x) = newBackground;

							// Update threshold image
							float thresh = _alpha*float(threshold.at<uchar>(y, x)) + 
								(1-_alpha)*(_threshCoeff * std::abs(pix - newBackground));
							if(thresh > float(minThreshold))
								threshold.at<uchar>(y, x) = thresh;
						}
						else
						{
							// Update threshold image
							threshold.at<uchar>(y, x) = minThreshold;
						}
					}
				}
			});
		}

		_frameN2 = _frameN1;
		_frameN1 = frame.clone();

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Source", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "background", "Background", "" },
			{ ENodeFlowDataType::Image, "movingPixels", "Moving pixels", "" },
			{ ENodeFlowDataType::Image, "threshold", "Threshold image", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Double, "Alpha", "min:0.0, max:1.0, decimals:3" },
			{ EPropertyType::Double, "Threshold coeff.", "min:0.0, decimals:3" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
		nodeConfig.flags = Node_HasState;
	}

private:
	enum EPropertyID
	{
		ID_Alpha,
		ID_ThresholdCoeff
	};

	cv::Mat _frameN1; // I_{n-1}
	cv::Mat _frameN2; // I_{n-2}
	float _alpha;
	float _threshCoeff;
};

REGISTER_NODE("Video/Background subtraction", BackgroundSubtractorNodeType)
REGISTER_NODE("Video/Mixture of Gaussians", MixtureOfGaussiansNodeType)

REGISTER_NODE("Edge/Canny edge detector", CannyEdgeDetectorNodeType)

REGISTER_NODE("Image/Upsample", UpsampleNodeType)
REGISTER_NODE("Image/Downsample", DownsampleNodeType)
REGISTER_NODE("Image/CLAHE", ClaheNodeType)
REGISTER_NODE("Image/Equalize histogram", EqualizeHistogramNodeType)
REGISTER_NODE("Image/Draw histogram", DrawHistogramNodeType)

REGISTER_NODE("Segmentation/Otsu's thresholding", OtsuThresholdingNodeType)
REGISTER_NODE("Segmentation/Binarization", BinarizationNodeType)
