#include "NodeType.h"
#include "NodeFactory.h"

#include <opencv2/imgproc/imgproc.hpp>

#include "CV.h"

class StructuringElementNodeType : public NodeType
{
public:

	StructuringElementNodeType()
		: _se(cvu::EStructuringElementType::Ellipse)
		, _xradius(1)
		, _yradius(1)
		, _rotation(0)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(propId)
		{
		case ID_StructuringElementType:
			_se = newValue.toEnum().convert<cvu::EStructuringElementType>();
			return true;
		case ID_XRadius:
			_xradius = newValue.toInt();
			return true;
		case ID_YRadius:
			_yradius = newValue.toInt();
			return true;
		case ID_Rotation:
			_rotation = newValue.toInt();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_StructuringElementType: return _se;
		case ID_XRadius: return _xradius;
		case ID_YRadius: return _yradius;
		case ID_Rotation: return _rotation;
		}

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader&, NodeSocketWriter& writer) override
	{
		cv::Mat& kernel = writer.acquireSocket(0).getImage();

		if(_xradius == 0 || _yradius == 0)
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
		kernel = cvu::standardStructuringElement(_xradius, _yradius, _se, _rotation);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "structuringElement", "Structuring element", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Enum, "SE shape", "item: Rectangle, item: Ellipse, item: Cross" },
			{ EPropertyType::Integer, "Horizontal radius", "min:1, max:50" },
			{ EPropertyType::Integer, "Vertical radius", "min:1, max:50" },
			{ EPropertyType::Integer, "Rotation", "min:0, max:359, wrap:true" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Generates a structuring element for a morphological "
			"operations with a given parameters describing its shape, size and rotation.";
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum EPropertyID
	{
		ID_StructuringElementType,
		ID_XRadius,
		ID_YRadius,
		ID_Rotation
	};

	cvu::EStructuringElementType _se;
	int _xradius;
	int _yradius;
	int _rotation;
};

class MorphologyOperatorNodeType : public NodeType
{
public:
	MorphologyOperatorNodeType()
		: _op(EMorphologyOperation::Erode)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(propId)
		{
		case ID_Operation:
			_op = newValue.toEnum().convert<EMorphologyOperation>();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_Operation: return _op;
		}

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& src = reader.readSocket(0).getImage();
		const cv::Mat& se = reader.readSocket(1).getImage();
		// Acquire output sockets
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		// Validate inputs
		if(se.empty() || src.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
		cv::morphologyEx(src, dst, int(_op), se);

		return ExecutionStatus(EStatus::Ok);
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Image, "source", "Source", "" },
			{ ENodeFlowDataType::Image, "source", "Structuring element", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Enum, "Operation type", 
			"item: Erode, item: Dilate, item: Open, item: Close,"
			"item: Gradient, item: Top Hat, item: Black Hat"},
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Performs morphological operation on a given image.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	enum class EMorphologyOperation
	{
		Erode    = cv::MORPH_ERODE,
		Dilate   = cv::MORPH_DILATE,
		Open     = cv::MORPH_OPEN,
		Close    = cv::MORPH_CLOSE,
		Gradient = cv::MORPH_GRADIENT,
		TopHat   = cv::MORPH_TOPHAT,
		BlackHat = cv::MORPH_BLACKHAT
	};

	enum EPropertyID
	{
		ID_Operation
	};

	EMorphologyOperation _op;
};

static const int OBJ = 255;
static const int BCK = 0;

static int skeletonZHLutTable[256]  = {
	0,0,0,1,0,0,1,3,0,0,3,1,1,0,1,3,0,0,0,0,0,0,0,0,2,0,2,0,3,0,3,3,
	0,0,0,0,0,0,0,0,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,3,0,2,2,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	2,0,0,0,0,0,0,0,2,0,0,0,2,0,0,0,3,0,0,0,0,0,0,0,3,0,0,0,3,0,2,0,
	0,0,3,1,0,0,1,3,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,
	3,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	2,3,1,3,0,0,1,3,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	2,3,0,1,0,0,0,1,0,0,0,0,0,0,0,0,3,3,0,1,0,0,0,0,2,2,0,0,2,0,0,0
};

class HitMissOperatorNodeType : public NodeType
{
public:
	HitMissOperatorNodeType()
		: _op(EHitMissOperation::Outline)
	{
	}

	bool setProperty(PropertyID propId, const NodeProperty& newValue) override
	{
		switch(propId)
		{
		case ID_Operation:
			_op = newValue.toEnum().convert<EHitMissOperation>();
			return true;
		}

		return false;
	}

	NodeProperty property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_Operation: return _op;
		}

		return NodeProperty();
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// Read input sockets
		const cv::Mat& src = reader.readSocket(0).getImage();
		// Acquire output sockets
		cv::Mat& dst = writer.acquireSocket(0).getImage();

		// Validate inputs
		if(src.empty())
			return ExecutionStatus(EStatus::Ok);

		// Do stuff
		switch(_op)
		{
		case EHitMissOperation::Outline:
			hitmissOutline(src, dst);
			break;
		case EHitMissOperation::Skeleton:
			hitmissSkeleton(src, dst);
			break;
		case EHitMissOperation::SkeletonZhang:
			hitmissSkeletonZhangSuen(src, dst);
			break;
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
			{ ENodeFlowDataType::Image, "output", "Output", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Enum, "Operation type", 
			"item: Outline, item: Skeleton, item: Skeleton (Zhang-Suen)"},
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "Performs hit-miss operation on a given image.";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
	}

private:
	void hitmissOutline(const cv::Mat& src, cv::Mat& dst)
	{
		dst = src.clone();

		// 1 - obiekt (bialy)
		// 0 - tlo (czarny)
		// X - dowolny

		// Element strukturalny
		// 1|1|1
		// 1|X|1
		// 1|1|1
		//

		const uchar* pixels2 = src.ptr<uchar>();
		uchar* pixels = dst.ptr<uchar>();
		int rowOffset = src.cols;

		cvu::parallel_for(cv::Range(1, src.rows - 1), [&](const cv::Range& range)
		{
			for(int y = range.start; y < range.end; ++y)
			{
				int offset = 1 + y * rowOffset;
				for(int x = 1; x < src.cols - 1; ++x)
				{
					uchar v = pixels2[offset];

					if(v == OBJ)
					{
						uchar p1 = pixels2[offset-rowOffset-1];
						uchar p2 = pixels2[offset-rowOffset];
						uchar p3 = pixels2[offset-rowOffset+1];
						uchar p4 = pixels2[offset-1];
						uchar p6 = pixels2[offset+1];
						uchar p7 = pixels2[offset+rowOffset-1];
						uchar p8 = pixels2[offset+rowOffset];
						uchar p9 = pixels2[offset+rowOffset+1];

						if (p1==OBJ && p2==OBJ && p3==OBJ && p4==OBJ && 
							p6==OBJ && p7==OBJ && p8==OBJ && p9==OBJ)
						{
							v = BCK;
						}
					}			

					pixels[offset++] = v;
				}
			}
		});
	}

	inline int _skeleton_iter1(const cv::Mat& src, cv::Mat& dst)
	{
		// Element strukturalny pierwszy
		// 1|1|1
		// X|1|X
		// 0|0|0
		//

		int d = 0;

		cvu::parallel_for(cv::Range(1, src.rows - 1), [&](const cv::Range& range)
		{
			for(int y = range.start; y < range.end; ++y)
			{
				for(int x = 1; x < src.cols - 1; ++x)
				{
					if (src.at<uchar>(y-1, x-1) == OBJ &&
						src.at<uchar>(y-1, x  ) == OBJ &&
						src.at<uchar>(y-1, x+1) == OBJ &&
						src.at<uchar>(y  , x  ) == OBJ &&
						src.at<uchar>(y+1, x-1) == BCK &&
						src.at<uchar>(y+1, x  ) == BCK &&
						src.at<uchar>(y+1, x+1) == BCK)
					{
						dst.at<uchar>(y, x) = BCK;
						d++;
					}
				}
			}
		});

		return d;
	}

	inline int _skeleton_iter2(const cv::Mat& src, cv::Mat& dst)
	{
		// Element strukturalny pierwszy - 90 w lewo
		// 1|X|0
		// 1|1|0
		// 1|x|0
		//

		int d = 0;

		cvu::parallel_for(cv::Range(1, src.rows - 1), [&](const cv::Range& range)
		{
			for(int y = range.start; y < range.end; ++y)
			{
				for(int x = 1; x < src.cols - 1; ++x)
				{
					if (src.at<uchar>(y-1, x-1) == OBJ &&
						src.at<uchar>(y-1, x+1) == BCK &&
						src.at<uchar>(y  , x-1) == OBJ &&
						src.at<uchar>(y  , x  ) == OBJ &&
						src.at<uchar>(y  , x+1) == BCK &&
						src.at<uchar>(y+1, x-1) == OBJ &&
						src.at<uchar>(y+1, x+1) == BCK)
					{
						dst.at<uchar>(y, x) = BCK;
						d++;
					}
				}
			}
		});

		return d;
	}

	inline int _skeleton_iter3(const cv::Mat& src, cv::Mat& dst)
	{
		// Element strukturalny pierwszy - 180 w lewo
		// 0|0|0
		// X|1|X
		// 1|1|1
		//

		int d = 0;

		cvu::parallel_for(cv::Range(1, src.rows - 1), [&](const cv::Range& range)
		{
			for(int y = range.start; y < range.end; ++y)
			{
				for(int x = 1; x < src.cols - 1; ++x)
				{
					if (src.at<uchar>(y-1, x-1) == BCK &&
						src.at<uchar>(y-1, x  ) == BCK &&
						src.at<uchar>(y-1, x+1) == BCK &&
						src.at<uchar>(y  , x  ) == OBJ &&
						src.at<uchar>(y+1, x-1) == OBJ &&
						src.at<uchar>(y+1, x  ) == OBJ &&
						src.at<uchar>(y+1, x+1) == OBJ)
					{
						dst.at<uchar>(y, x) = BCK;
						d++;
					}
				}
			}
		});
		return d;
	}

	inline int _skeleton_iter4(const cv::Mat& src, cv::Mat& dst)
	{
		// Element strukturalny pierwszy - 270 w lewo
		// 0|X|1
		// 0|1|1
		// 0|X|1
		//

		int d = 0;

		cvu::parallel_for(cv::Range(1, src.rows - 1), [&](const cv::Range& range)
		{
			for(int y = range.start; y < range.end; ++y)
			{
				for(int x = 1; x < src.cols - 1; ++x)
				{
					if (src.at<uchar>(y-1, x-1) == BCK &&
						src.at<uchar>(y-1, x+1) == OBJ &&
						src.at<uchar>(y  , x-1) == BCK &&
						src.at<uchar>(y  , x  ) == OBJ &&
						src.at<uchar>(y  , x+1) == OBJ &&
						src.at<uchar>(y+1, x-1) == BCK &&
						src.at<uchar>(y+1, x+1) == OBJ)
					{
						dst.at<uchar>(y, x) = BCK;
						d++;
					}
				}
			}
		});

		return d;
	}

	inline int _skeleton_iter5(const cv::Mat& src, cv::Mat& dst)
	{
		// Element strukturalny drugi
		// X|1|X
		// 0|1|1
		// 0|0|X
		//

		int d = 0;

		cvu::parallel_for(cv::Range(1, src.rows - 1), [&](const cv::Range& range)
		{
			for(int y = range.start; y < range.end; ++y)
			{
				for(int x = 1; x < src.cols - 1; ++x)
				{
					if (src.at<uchar>(y-1, x  ) == OBJ &&
						src.at<uchar>(y  , x-1) == BCK &&
						src.at<uchar>(y  , x  ) == OBJ &&
						src.at<uchar>(y  , x+1) == OBJ &&
						src.at<uchar>(y+1, x-1) == BCK &&
						src.at<uchar>(y+1, x  ) == BCK)
					{
						dst.at<uchar>(y, x) = BCK;
						d++;
					}
				}
			}
		});

		return d;
	}

	inline int _skeleton_iter6(const cv::Mat& src, cv::Mat& dst)
	{
		// Element strukturalny drugi - 90 stopni w lewo
		// X|1|X
		// 1|1|0
		// X|0|0
		//

		int d = 0;

		cvu::parallel_for(cv::Range(1, src.rows - 1), [&](const cv::Range& range)
		{
			for(int y = range.start; y < range.end; ++y)
			{
				for(int x = 1; x < src.cols - 1; ++x)
				{
					if (src.at<uchar>(y-1, x  ) == OBJ &&
						src.at<uchar>(y  , x-1) == OBJ &&
						src.at<uchar>(y  , x  ) == OBJ &&
						src.at<uchar>(y  , x+1) == BCK &&
						src.at<uchar>(y+1, x  ) == BCK &&
						src.at<uchar>(y+1, x+1) == BCK)
					{
						dst.at<uchar>(y, x) = BCK;
						d++;
					}
				}
			}
		});

		return d;
	}

	inline int _skeleton_iter7(const cv::Mat& src, cv::Mat& dst)
	{
		// Element strukturalny drugi - 180 stopni w lewo
		// X|0|0
		// 1|1|0
		// X|1|X
		//

		int d = 0;

		cvu::parallel_for(cv::Range(1, src.rows - 1), [&](const cv::Range& range)
		{
			for(int y = range.start; y < range.end; ++y)
			{
				for(int x = 1; x < src.cols - 1; ++x)
				{
					if (src.at<uchar>(y-1, x  ) == BCK &&
						src.at<uchar>(y-1, x+1) == BCK &&
						src.at<uchar>(y  , x-1) == OBJ &&
						src.at<uchar>(y  , x  ) == OBJ &&
						src.at<uchar>(y  , x+1) == BCK &&
						src.at<uchar>(y+1, x  ) == OBJ)
					{
						dst.at<uchar>(y, x) = BCK;
						d++;
					}
				}
			}
		});

		return d;
	}

	inline int _skeleton_iter8(const cv::Mat& src, cv::Mat& dst)
	{
		// Element strukturalny drugi - 270 stopni w lewo
		// 0|0|X
		// 0|1|1
		// X|1|X
		//

		int d = 0;

		cvu::parallel_for(cv::Range(1, src.rows - 1), [&](const cv::Range& range)
		{
			for(int y = range.start; y < range.end; ++y)
			{
				for(int x = 1; x < src.cols - 1; ++x)
				{
					if (src.at<uchar>(y-1, x-1) == BCK &&
						src.at<uchar>(y-1, x  ) == BCK &&
						src.at<uchar>(y  , x-1) == BCK &&
						src.at<uchar>(y  , x  ) == OBJ &&
						src.at<uchar>(y  , x+1) == OBJ &&
						src.at<uchar>(y+1, x  ) == OBJ)
					{
						dst.at<uchar>(y, x) = BCK;
						d++;
					}
				}
			}
		});

		return d;
	}

	int hitmissSkeleton(const cv::Mat& _src, cv::Mat &dst)
	{
		int niters = 0;

		cv::Mat src = _src.clone();
		dst = src.clone();

		while(true) 
		{
			// iteracja
			++niters;
			int d = 0;

			d += _skeleton_iter1(src, dst);
			dst.copyTo(src);

			d += _skeleton_iter2(src, dst);
			dst.copyTo(src);

			d += _skeleton_iter3(src, dst);
			dst.copyTo(src);

			d += _skeleton_iter4(src, dst);
			dst.copyTo(src);

			d += _skeleton_iter5(src, dst);
			dst.copyTo(src);

			d += _skeleton_iter6(src, dst);
			dst.copyTo(src);

			d += _skeleton_iter7(src, dst);
			dst.copyTo(src);

			d += _skeleton_iter8(src, dst);

			//printf("Iteration: %3d, pixel changed: %5d\r", niters, d);

			if(d == 0)
				break;

			dst.copyTo(src);
		}
		//printf("\n");

		return niters;
	}

	int hitmissSkeletonZhangSuen_pass(cv::Mat& dst, int pass)
	{
		const int bgColor = 0;

		cv::Mat tmp = dst.clone();
		uchar* pixels2 = tmp.ptr<uchar>();
		uchar* pixels = dst.ptr<uchar>();
		int pixelsRemoved = 0;
		int xMin = 1, yMin = 1, rowOffset = dst.cols;

		cvu::parallel_for(cv::Range(yMin, dst.rows - 1), [&](const cv::Range& range)
		{
			for(int y = range.start; y < range.end; ++y)
			{
				int offset = xMin + y * rowOffset;
				for(int x = xMin; x < dst.cols - 1; ++x)
				{
					uchar p5 = pixels2[offset];
					uchar v = p5;
					if(v != bgColor)
					{
						uchar p1 = pixels2[offset-rowOffset-1];
						uchar p2 = pixels2[offset-rowOffset];
						uchar p3 = pixels2[offset-rowOffset+1];
						uchar p4 = pixels2[offset-1];
						uchar p6 = pixels2[offset+1];
						uchar p7 = pixels2[offset+rowOffset-1];
						uchar p8 = pixels2[offset+rowOffset];
						uchar p9 = pixels2[offset+rowOffset+1];

						// lut index
						int index = 
							((p4&0x01) << 7) |
							((p7&0x01) << 6) |
							((p8&0x01) << 5) |
							((p9&0x01) << 4) |
							((p6&0x01) << 3) |
							((p3&0x01) << 2) |
							((p2&0x01) << 1) |
							(p1&0x01);
						int code = skeletonZHLutTable[index];

						//odd pass
						if((pass & 1) == 1)
						{ 
							if(code == 2 || code == 3)
							{
								v = bgColor;
								pixelsRemoved++;
							}
						} 
						//even pass
						else
						{
							if (code == 1 || code == 3)
							{
								v = bgColor;
								pixelsRemoved++;
							}
						}
					}
					pixels[offset++] = v;
				}
			}
		});

		return pixelsRemoved;
	}

	int hitmissSkeletonZhangSuen(const cv::Mat& src, cv::Mat& dst)
	{
		// Based on ImageJ implementation of skeletonization which is
		// based on an a thinning algorithm by by Zhang and Suen (CACM, March 1984, 236-239)
		int pass = 0;
		int pixelsRemoved = 0;
		dst = src.clone();
		int niters = 0;

		do 
		{
			niters++;
			pixelsRemoved  = hitmissSkeletonZhangSuen_pass(dst, pass++);
			pixelsRemoved += hitmissSkeletonZhangSuen_pass(dst, pass++);
			//printf("Iteration: %3d, pixel changed: %5d\r", niters, pixelsRemoved);
		} while (pixelsRemoved > 0);
		//printf("\n");

		return niters;
	}

private:
	enum class EHitMissOperation
	{
		Outline,
		Skeleton,
		SkeletonZhang
	};

	enum EPropertyID
	{
		ID_Operation
	};

	EHitMissOperation _op;
};

REGISTER_NODE("Morphology/Hit-miss", HitMissOperatorNodeType)
REGISTER_NODE("Morphology/Operator", MorphologyOperatorNodeType)
REGISTER_NODE("Morphology/Structuring element", StructuringElementNodeType)
