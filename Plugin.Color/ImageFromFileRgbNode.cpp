#include "Logic/NodePlugin.h"
#include "Logic/NodeSystem.h"

#include "Kommon/StringUtils.h"

#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

class ImageFromFileRgbNodeType : public NodeType
{
public: 
	ImageFromFileRgbNodeType()
#ifdef QT_DEBUG
		: _filePath("lena.jpg")
#else
		: _filePath("")
#endif
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		if(propId == 0)
		{
			_filePath = newValue.toFilepath();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		if(propId == 0)
			return _filePath;

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader&, NodeSocketWriter& writer) override
	{
		cv::Mat& output = writer.acquireSocket(0).getImageRgb();

		output = cv::imread(_filePath.data(), CV_LOAD_IMAGE_COLOR);

		if(output.empty())
			return ExecutionStatus(EStatus::Error, "File not found");
		return ExecutionStatus(EStatus::Ok, 
			string_format("Image image width: %d\nImage image height: %d\nImage size in kbytes: %d",
				output.cols, output.rows, output.cols * output.rows * sizeof(uchar) * output.channels() / 1024));
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::ImageRgb, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Filepath, "File path", "filter:"
			"Popular image formats (*.bmp *.jpeg *.jpg *.png *.tiff);;"
			"Windows bitmaps (*.bmp *.dib);;"
			"JPEG files (*.jpeg *.jpg *.jpe);;"
			"JPEG 2000 files (*.jp2);;"
			"Portable Network Graphics (*.png);;"
			"Portable image format (*.pbm *.pgm *.ppm);;"
			"Sun rasters (*.sr *.ras);;"
			"TIFF files (*.tiff *.tif);;"
			"All files (*.*)" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Loads image from a given location.";
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

protected:
	Filepath _filePath;
};

void registerImageFromFileRgb(NodeSystem* nodeSystem)
{
	typedef DefaultNodeFactory<ImageFromFileRgbNodeType> ImageFromFileRgbFactory;
	nodeSystem->registerNodeType("Sources/Image from file RGB",
		std::unique_ptr<NodeFactory>(new ImageFromFileRgbFactory()));
}