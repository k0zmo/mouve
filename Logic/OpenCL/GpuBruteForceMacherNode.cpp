#if defined(HAVE_OPENCL)

#include "GpuNode.h"
#include "NodeFactory.h"

static vector<cv::DMatch> distanceRatioTest(const vector<vector<cv::DMatch>>& knMatches, 
											float distanceRatioThreshold)
{
	vector<cv::DMatch> positiveMatches;
	positiveMatches.reserve(knMatches.size());

	for(auto&& knMatch : knMatches)
	{
		if(knMatch.size() != 2)
			continue;

		auto&& best = knMatch[0];
		auto&& good = knMatch[1];

		if(best.distance <= distanceRatioThreshold * good.distance)
			positiveMatches.push_back(best);
	}

	return positiveMatches;
}

static vector<cv::DMatch> symmetryTest(const vector<cv::DMatch>& matches1to2,
									   const vector<cv::DMatch>& matches2to1)
{
	vector<cv::DMatch> bothMatches;

	for(auto&& match1to2 : matches1to2)
	{
		for(auto&& match2to1 : matches2to1)
		{
			if(match1to2.queryIdx == match2to1.trainIdx
			&& match2to1.queryIdx == match1to2.trainIdx)
			{
				bothMatches.push_back(match1to2);
				break;
			}
		}
	}

	return bothMatches;
}

class GpuBruteForceMatcherNodeType : public GpuNodeType
{
public:
	GpuBruteForceMatcherNodeType()
		: _distanceRatio(0.8)
		, _symmetryTest(false)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_DistanceRatio:
			_distanceRatio = newValue.toDouble();
			return true;
		case ID_SymmetryTest:
			_symmetryTest = newValue.toBool();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_DistanceRatio: return _distanceRatio;
		case ID_SymmetryTest: return _symmetryTest;
		}

		return QVariant();
	}

	bool postInit() override
	{
		string opts;

		if(_gpuComputeModule->device().platform().vendorEnum() == clw::Vendor_AMD
		&& _gpuComputeModule->device().platform().version() >= clw::Version_1_2)
		{
			opts = "-x clc++ -DCL_LANGUAGE_CPP=1";
			_kidBruteForceMatch_2nnMatch_SURF = _gpuComputeModule->registerKernel("bruteForceMatch_2nnMatch_SURF", "bfmatcher.cl", opts);
			_kidBruteForceMatch_2nnMatch_SIFT = _gpuComputeModule->registerKernel("bruteForceMatch_2nnMatch_SIFT", "bfmatcher.cl", opts);
			_kidBruteForceMatch_2nnMatch_FREAK = _gpuComputeModule->registerKernel("bruteForceMatch_2nnMatch_FREAK", "bfmatcher.cl", opts);
			_kidBruteForceMatch_2nnMatch_ORB = _gpuComputeModule->registerKernel("bruteForceMatch_2nnMatch_ORB", "bfmatcher.cl", opts);
		}
		else
		{
			opts = "-DBLOCK_SIZE=16 -DDESC_LEN=64 -DKERNEL_NAME=bruteForceMatch_2nnMatch_SURF -DQUERY_TYPE=float -DDIST_TYPE=float -DDIST_FUNCTION=l2DistIter";
			_kidBruteForceMatch_2nnMatch_SURF = _gpuComputeModule->registerKernel("bruteForceMatch_2nnMatch_SURF", "bfmatcher_macros.cl", opts);

			opts = "-DBLOCK_SIZE=16 -DDESC_LEN=128 -DKERNEL_NAME=bruteForceMatch_2nnMatch_SIFT -DQUERY_TYPE=float -DDIST_TYPE=float -DDIST_FUNCTION=l2DistIter";
			_kidBruteForceMatch_2nnMatch_SIFT = _gpuComputeModule->registerKernel("bruteForceMatch_2nnMatch_SIFT", "bfmatcher_macros.cl", opts);

			opts = "-DBLOCK_SIZE=16 -DDESC_LEN=64 -DKERNEL_NAME=bruteForceMatch_2nnMatch_FREAK -DQUERY_TYPE=uchar -DDIST_TYPE=int -DDIST_FUNCTION=hammingDistIter";
			_kidBruteForceMatch_2nnMatch_FREAK = _gpuComputeModule->registerKernel("bruteForceMatch_2nnMatch_FREAK", "bfmatcher_macros.cl", opts);

			opts = "-DBLOCK_SIZE=16 -DDESC_LEN=32 -DKERNEL_NAME=bruteForceMatch_2nnMatch_ORB -DQUERY_TYPE=uchar -DDIST_TYPE=int -DDIST_FUNCTION=hammingDistIter";
			_kidBruteForceMatch_2nnMatch_ORB = _gpuComputeModule->registerKernel("bruteForceMatch_2nnMatch_ORB", "bfmatcher_macros.cl", opts);
		}

		return _kidBruteForceMatch_2nnMatch_SURF != InvalidKernelID
			&& _kidBruteForceMatch_2nnMatch_SIFT != InvalidKernelID
			&& _kidBruteForceMatch_2nnMatch_FREAK != InvalidKernelID
			&& _kidBruteForceMatch_2nnMatch_ORB != InvalidKernelID;
	}

	ExecutionStatus execute(NodeSocketReader& reader, NodeSocketWriter& writer) override
	{
		// inputs
		const KeyPoints& queryKp = reader.readSocket(0).getKeypoints();
		const DeviceArray& query_dev = reader.readSocket(1).getDeviceArray();
		const KeyPoints& trainKp = reader.readSocket(2).getKeypoints();
		const DeviceArray& train_dev = reader.readSocket(3).getDeviceArray();
		// outputs
		Matches& mt = writer.acquireSocket(0).getMatches();

		if(query_dev.isNull() || query_dev.size() == 0
		|| train_dev.isNull() || train_dev.size() == 0)
			return ExecutionStatus(EStatus::Ok);

		if(query_dev.width() != train_dev.width()
		|| query_dev.dataType() != train_dev.dataType())
			return ExecutionStatus(EStatus::Error, "Query and train descriptors are different types");

		vector<cv::DMatch> matches;
		size_t initialMatches = 0;

		if(_symmetryTest)
		{
			vector<vector<cv::DMatch>> knMatches1to2, knMatches2to1;

			auto status = knnMatch_caller(query_dev, train_dev, knMatches1to2);
			if(status.status != EStatus::Ok)
				return status;

			status = knnMatch_caller(train_dev, query_dev, knMatches2to1);
			if(status.status != EStatus::Ok)
				return status;

			initialMatches = knMatches1to2.size();
			auto& matches1to2 = distanceRatioTest(knMatches1to2, _distanceRatio);
			auto& matches2to1 = distanceRatioTest(knMatches2to1, _distanceRatio);
			matches = symmetryTest(matches1to2, matches2to1);
		}
		else
		{
			vector<vector<cv::DMatch>> knMatches;
			auto status = knnMatch_caller(query_dev, train_dev, knMatches);
			if(status.status != EStatus::Ok)
				return status;
			initialMatches = knMatches.size();

			matches = distanceRatioTest(knMatches, _distanceRatio);
		}

		// Convert to 'Matches' data type
		mt.queryPoints.clear();
		mt.trainPoints.clear();

		for(auto&& match : matches)
		{
			mt.queryPoints.push_back(queryKp.kpoints[match.queryIdx].pt);
			mt.trainPoints.push_back(trainKp.kpoints[match.trainIdx].pt);
		}

		mt.queryImage = queryKp.image;
		mt.trainImage = trainKp.image;

		return ExecutionStatus(EStatus::Ok, 
			formatMessage("Initial matches found: %d\nFinal matches found: %d",
			(int) initialMatches, (int) mt.queryPoints.size()));
	}

	void configuration(NodeConfig& nodeConfig) const override
	{
		static const InputSocketConfig in_config[] = {
			{ ENodeFlowDataType::Keypoints, "keypoints1", "Query keypoints", "" },
			{ ENodeFlowDataType::DeviceArray, "descriptors1", "Query descriptors", "" },
			{ ENodeFlowDataType::Keypoints, "keypoints2", "Train keypoints", "" },
			{ ENodeFlowDataType::DeviceArray, "descriptors2", "Train descriptors", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const OutputSocketConfig out_config[] = {
			{ ENodeFlowDataType::Matches, "matches", "Matches", "" },
			{ ENodeFlowDataType::Invalid, "", "", "" }
		};
		static const PropertyConfig prop_config[] = {
			{ EPropertyType::Double, "Distance ratio", "min:0.0, max:1.0, step:0.1, decimals:2" },
			{ EPropertyType::Boolean, "Symmetry test", "" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
		nodeConfig.module = "opencl";
	}

private:

	ExecutionStatus knnMatch_caller(const DeviceArray& query_dev,
									const DeviceArray& train_dev,
									vector<vector<cv::DMatch>>& knMatches)
	{
		if(query_dev.dataType() == EDataType::Float)
		{
			if(query_dev.width() == 64)
				knMatches = knnMatch<16, 64>(query_dev, train_dev, _kidBruteForceMatch_2nnMatch_SURF);
			else if(query_dev.width() == 128)
				knMatches = knnMatch<16, 128>(query_dev, train_dev, _kidBruteForceMatch_2nnMatch_SIFT);
			else 
				return ExecutionStatus(EStatus::Error, 
				"Unsupported descriptor data size "
				"(must be 64 or 128 elements width");
		}
		else if(query_dev.dataType() == EDataType::Uchar)
		{
			if(query_dev.width() == 32)
				knMatches = knnMatch<16, 32>(query_dev, train_dev, _kidBruteForceMatch_2nnMatch_ORB);
			else if(query_dev.width() == 64)
				knMatches = knnMatch<16, 64>(query_dev, train_dev, _kidBruteForceMatch_2nnMatch_FREAK);
			else 
				return ExecutionStatus(EStatus::Error, 
				"Unsupported descriptor data size "
				"(must be 32 or 64 elements width");
		}
		else
		{
			return ExecutionStatus(EStatus::Error, 
				"Unsupported descriptor data type "
				"(must be float for L2 norm or Uint8 for Hamming norm");
		}

		return ExecutionStatus(EStatus::Ok);
	}

	template<int BLOCK_SIZE, int DESCRIPTOR_LEN>
	vector<vector<cv::DMatch>> knnMatch(const DeviceArray& query_dev,
										const DeviceArray& train_dev,
										KernelID kidBruteForceMatch) 
	{
		// Ensure output buffers are enough
		if(_trainIdx_cl.isNull()
		|| _trainIdx_cl.size() < sizeof(cl_int2) * query_dev.height())
		{
			_trainIdx_cl = _gpuComputeModule->context().createBuffer(
				clw::Access_ReadWrite, clw::Location_Device, 
				sizeof(cl_int2) * query_dev.height());
			_distance_cl = _gpuComputeModule->context().createBuffer(
				clw::Access_ReadWrite, clw::Location_Device, 
				sizeof(cl_float2) * query_dev.height());
		}

		const size_t smemSize = (BLOCK_SIZE*std::max(DESCRIPTOR_LEN,BLOCK_SIZE) + BLOCK_SIZE*BLOCK_SIZE) * sizeof(int);
		clw::Kernel kernel2nnMatch = _gpuComputeModule->acquireKernel(kidBruteForceMatch);
		kernel2nnMatch.setLocalWorkSize(BLOCK_SIZE, BLOCK_SIZE);
		kernel2nnMatch.setRoundedGlobalWorkSize(query_dev.height(), BLOCK_SIZE);
		kernel2nnMatch.setArg(0, query_dev.buffer());
		kernel2nnMatch.setArg(1, train_dev.buffer());
		kernel2nnMatch.setArg(2, _trainIdx_cl);
		kernel2nnMatch.setArg(3, _distance_cl);
		kernel2nnMatch.setArg(4, clw::LocalMemorySize(smemSize));
		kernel2nnMatch.setArg(5, query_dev.height());
		kernel2nnMatch.setArg(6, train_dev.height());
		_gpuComputeModule->queue().asyncRunKernel(kernel2nnMatch);

		cv::Mat trainIdx(1, query_dev.height(), CV_32SC2);
		cv::Mat distance(1, query_dev.height(), CV_32FC2);

		_gpuComputeModule->queue().asyncReadBuffer(_trainIdx_cl, trainIdx.data,
			0, sizeof(cl_int2) * query_dev.height());
		_gpuComputeModule->queue().asyncReadBuffer(_distance_cl, distance.data, 
			0, sizeof(cl_float2) * query_dev.height());
		_gpuComputeModule->queue().finish();

		// TODO: (nearest neighbour) distance ratio test on GPU
		// TOOD: symmetry test on GPU (killer of performance)

		return convertToMatches(trainIdx, distance);
	}

	vector<vector<cv::DMatch>> convertToMatches(const cv::Mat& trainIdx, const cv::Mat& distance)
	{
		int k  = 2;
		int nQuery = trainIdx.cols;
		vector<vector<cv::DMatch>> knMatches;
		knMatches.reserve(nQuery);

		const int *trainIdx_ptr = trainIdx.ptr<int>();
		const float *distance_ptr = distance.ptr<float>();

		for(int queryIdx = 0; queryIdx < nQuery; ++queryIdx)
		{
			knMatches.push_back(vector<cv::DMatch>());
			auto&& curMatches = knMatches.back();
			curMatches.reserve(2);

			for (int i = 0; i < k; ++i, ++trainIdx_ptr, ++distance_ptr)
			{
				if (*trainIdx_ptr != -1)
					curMatches.push_back(cv::DMatch(queryIdx, *trainIdx_ptr, 0, *distance_ptr));
			}
		}

		return knMatches;
	}

private:
	enum EPropertyID
	{
		ID_DistanceRatio,
		ID_SymmetryTest
	};

	double _distanceRatio;
	bool _symmetryTest;

private:
	KernelID _kidBruteForceMatch_2nnMatch_SURF;
	KernelID _kidBruteForceMatch_2nnMatch_SIFT;
	KernelID _kidBruteForceMatch_2nnMatch_FREAK; // also BRISK
	KernelID _kidBruteForceMatch_2nnMatch_ORB;
	KernelID _kidFillBuffer;

	clw::Buffer _trainIdx_cl;
	clw::Buffer _distance_cl;
};

REGISTER_NODE("OpenCL/Features/BForce Matcher", GpuBruteForceMatcherNodeType)

#endif