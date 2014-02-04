#include "Logic/NodeType.h"
#include "Logic/NodeFactory.h"

#include <opencv2/imgproc/imgproc.hpp>

class EqualizeHistogramNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImageMono();
		cv::Mat& dst = writer.acquireSocket(0).getImageMono();

		if(src.rows == 0 || src.cols == 0)
			return ExecutionStatus(EStatus::Ok);

		cv::equalizeHist(src, dst);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::ImageMono, "source", "Source", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::ImageMono, "output", "Output", "" },
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

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::ClipLimit:
			_clipLimit = newValue.toDouble();
			return true;
		case pid::TilesGridSize:
			_tilesGridSize = newValue.toInt();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(static_cast<pid>(propId))
		{
		case pid::ClipLimit: return _clipLimit;
		case pid::TilesGridSize: return _tilesGridSize;
		}

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		const cv::Mat& src = reader.readSocket(0).getImageMono();
		cv::Mat& dst = writer.acquireSocket(0).getImageMono();

		cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE();
		clahe->setClipLimit(_clipLimit);
		clahe->setTilesGridSize(cv::Size(_tilesGridSize, _tilesGridSize));

		clahe->apply(src, dst);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::ImageMono, "source", "Source", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::ImageMono, "output", "Output", "" },
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
	enum class pid
	{
		ClipLimit,
		TilesGridSize
	};

	double _clipLimit;
	int _tilesGridSize;
};

class DrawHistogramNodeType : public NodeType
{
public:
	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& src = reader.readSocket(0).getImage();
		// Acquire output sockets
		cv::Mat& canvas = writer.acquireSocket(0).getImageRgb();

		// Validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		if(src.channels() == 1)
		{
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
					cv::Point(j, hist_h - cvRound(hist.at<float>(j))),
					cv::Scalar(200,200,200),
					1, 8, 0);
			}
		}
		else if(src.channels() == 3)
		{
			// Quantization
			const int bins = 256;
			// Range of given channel (0-255 for BGR)
			const float range[] = { 0, 256 } ;
			const float* histRange = { range };
			const bool uniform = true; 
			const bool accumulate = false;

			cv::Mat bhist, ghist, rhist;
			int bchannels[] = {0}, gchannels[] = {1}, rchannels[] = {2};

			cv::calcHist(&src, 1, bchannels, cv::noArray(), bhist, 1, &bins, &histRange, uniform, accumulate);
			cv::calcHist(&src, 1, gchannels, cv::noArray(), ghist, 1, &bins, &histRange, uniform, accumulate);
			cv::calcHist(&src, 1, rchannels, cv::noArray(), rhist, 1, &bins, &histRange, uniform, accumulate);

			// Draw histogram
			const int hist_h = 125;
			canvas = cv::Mat::zeros(hist_h, bins, CV_8UC3);
			cv::normalize(bhist, bhist, 0, canvas.rows, cv::NORM_MINMAX, -1, cv::noArray());
			cv::normalize(ghist, ghist, 0, canvas.rows, cv::NORM_MINMAX, -1, cv::noArray());
			cv::normalize(rhist, rhist, 0, canvas.rows, cv::NORM_MINMAX, -1, cv::noArray());

			for(int j = 1; j < bins; ++j)
			{
				cv::line(canvas, 
					cv::Point(j-1, hist_h - cvRound(bhist.at<float>(j-1))),
					cv::Point(j,   hist_h - cvRound(bhist.at<float>(j))), 
					cv::Scalar(255, 0, 0), 
					1, 8, 0);
				cv::line(canvas, 
					cv::Point(j-1, hist_h - cvRound(ghist.at<float>(j-1))),
					cv::Point(j,   hist_h - cvRound(ghist.at<float>(j))), 
					cv::Scalar(0, 255, 0), 
					1, 8, 0);
				cv::line(canvas, 
					cv::Point(j-1, hist_h - cvRound(rhist.at<float>(j-1))),
					cv::Point(j,   hist_h - cvRound(rhist.at<float>(j))), 
					cv::Scalar(0, 0, 255), 
					1, 8, 0);
			}
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

REGISTER_NODE("Histogram/CLAHE", ClaheNodeType)
REGISTER_NODE("Histogram/Equalize histogram", EqualizeHistogramNodeType)
REGISTER_NODE("Histogram/Draw histogram", DrawHistogramNodeType)
