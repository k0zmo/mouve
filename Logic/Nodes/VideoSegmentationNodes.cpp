#include "NodeType.h"
#include "NodeFactory.h"

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
		// Read input sockets
		const cv::Mat& source = reader.readSocket(0).getImage();
		// Acquire output sockets
		cv::Mat& output = writer.acquireSocket(0).getImage();

		// Validate inputs
		if(source.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff - single step
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

		nodeConfig.description = "Gaussian Mixture-based image sequence background/foreground segmentation.";
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

class AdaptiveMixtureOfGaussiansNodeType : public NodeType
{
public:
	AdaptiveMixtureOfGaussiansNodeType()
		: _history(200)
		, _varThreshold(16.0f)
		, _shadowDetection(true)
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
		case ID_VarianceThreshold:
			_varThreshold = newValue.toInt();
			return true;
		case ID_ShadowDetection:
			_shadowDetection = newValue.toDouble();
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
		case ID_VarianceThreshold: return _varThreshold;
		case ID_ShadowDetection: return _shadowDetection;
		case ID_LearningRate: return _learningRate;
		}

		return QVariant();
	}

	bool restart() override
	{
		_mog2 = cv::BackgroundSubtractorMOG2(_history, _varThreshold, _shadowDetection);
		return true;
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& source = reader.readSocket(0).getImage();
		// Acquire output sockets
		cv::Mat& output = writer.acquireSocket(0).getImage();

		// Validate inputs
		if(source.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff - single step
		_mog2(source, output, _learningRate);

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
			{ EPropertyType::Double, "Mahalanobis distance threshold",  "" },
			{ EPropertyType::Boolean, "Detect shadow", "" },
			{ EPropertyType::Double, "Learning rate", "min:-1, max:1, step:0.01, decimals:3" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Improved adaptive Gausian mixture model for background subtraction.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
		nodeConfig.flags = Node_HasState;
	}

private:
	enum EPropertyID
	{
		ID_History,
		ID_VarianceThreshold,
		ID_ShadowDetection,
		ID_LearningRate
	};

	double _learningRate;
	int _history;
	float _varThreshold;
	cv::BackgroundSubtractorMOG2 _mog2;
	bool _shadowDetection;
};

class BackgroundSubtractorGMGNodeType : public NodeType
{
public:
	BackgroundSubtractorGMGNodeType()
		: _learningRate(-1)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
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
		case ID_LearningRate: return _learningRate;
		}

		return QVariant();
	}

	bool restart() override
	{
		_gmg = cv::BackgroundSubtractorGMG();
		return true;
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& source = reader.readSocket(0).getImage();
		// Acquire output sockets
		cv::Mat& output = writer.acquireSocket(0).getImage();

		// Validate inputs
		if(source.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff - single step
		_gmg(source, output, _learningRate);

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
			{ EPropertyType::Double, "Learning rate", "min:-1, max:1, step:0.01, decimals:3" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "GMG background subtractor.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
		nodeConfig.flags = Node_HasState;
	}

private:
	enum EPropertyID { ID_LearningRate };

	double _learningRate;
	cv::BackgroundSubtractorGMG _gmg;
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
		// Read input sockets
		const cv::Mat& frame = reader.readSocket(0).getImage();

		// Acquire output sockets
		cv::Mat& background = writer.acquireSocket(0).getImage();
		cv::Mat& movingPixels = writer.acquireSocket(1).getImage();
		cv::Mat& threshold = writer.acquireSocket(2).getImage();

		// Validate inputs
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

			// Do stuff - single step
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

REGISTER_NODE("Video segmentation/Background subtractor", BackgroundSubtractorNodeType)
REGISTER_NODE("Video segmentation/GMG background subtractor", BackgroundSubtractorGMGNodeType)
REGISTER_NODE("Video segmentation/Adaptive mixture of Gaussians", AdaptiveMixtureOfGaussiansNodeType)
REGISTER_NODE("Video segmentation/Mixture of Gaussians", MixtureOfGaussiansNodeType)