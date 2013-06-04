#if defined(HAVE_OPENCL)

#include "GpuNode.h"
#include "NodeFactory.h"

class GpuNearestNeighborDistanceRatioMatcherNodeType : public GpuNodeType
{
public:
	GpuNearestNeighborDistanceRatioMatcherNodeType()
		: _distanceRatio(0.8)
	{
	}

	bool setProperty(PropertyID propId, const QVariant& newValue) override
	{
		switch(propId)
		{
		case ID_DistanceRatio:
			_distanceRatio = newValue.toDouble();
			return true;
		}

		return false;
	}

	QVariant property(PropertyID propId) const override
	{
		switch(propId)
		{
		case ID_DistanceRatio: return _distanceRatio;
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

		vector<vector<cv::DMatch>> knMatches;

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

		mt.queryPoints.clear();
		mt.trainPoints.clear();

		for(auto&& knMatch : knMatches)
		{
			if(knMatch.size() != 2)
				continue;

			auto&& best = knMatch[0];
			auto&& good = knMatch[1];

			if(best.distance <= _distanceRatio * good.distance)
			{
				mt.queryPoints.emplace_back(queryKp.kpoints[best.queryIdx].pt);
				mt.trainPoints.emplace_back(trainKp.kpoints[best.trainIdx].pt);
			}
		}

		mt.queryImage = queryKp.image;
		mt.trainImage = trainKp.image;

		return ExecutionStatus(EStatus::Ok, 
			formatMessage("Initial matches found: %d\nNNDR Matches found: %d",
			(int) knMatches.size(), (int) mt.queryPoints.size()));
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
			{ EPropertyType::Double, "ID_DistanceRatio", "min:0.0, max:1.0, step:0.1" },
			{ EPropertyType::Unknown, "", "" }
		};

		nodeConfig.description = "";
		nodeConfig.pInputSockets = in_config;
		nodeConfig.pOutputSockets = out_config;
		nodeConfig.pProperties = prop_config;
		nodeConfig.module = "opencl";
	}

private:
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
		ID_DistanceRatio
	};

	double _distanceRatio;

private:
	KernelID _kidBruteForceMatch_2nnMatch_SURF;
	KernelID _kidBruteForceMatch_2nnMatch_SIFT;
	KernelID _kidBruteForceMatch_2nnMatch_FREAK; // also BRISK
	KernelID _kidBruteForceMatch_2nnMatch_ORB;
	KernelID _kidFillBuffer;

	clw::Buffer _trainIdx_cl;
	clw::Buffer _distance_cl;
};

REGISTER_NODE("OpenCL/Features/NNDR Matcher", GpuNearestNeighborDistanceRatioMatcherNodeType)

#endif