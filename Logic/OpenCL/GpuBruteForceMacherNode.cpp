#if defined(HAVE_OPENCL)

#include "GpuNode.h"
#include "NodeFactory.h"

struct DMatch
{
	int queryIdx;
	int trainIdx;
	float dist;
};

static vector<DMatch> symmetryTest(const vector<DMatch>& matches1to2,
								   const vector<DMatch>& matches2to1)
{
	vector<DMatch> bothMatches;

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

/// TOOD: symmetry test on GPU (killer of performance)
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
			_kidBruteForceMatch_nndrMatch_SURF = _gpuComputeModule->registerKernel("bruteForceMatch_nndrMatch_SURF", "bfmatcher.cl", opts);
			_kidBruteForceMatch_nndrMatch_SIFT = _gpuComputeModule->registerKernel("bruteForceMatch_nndrMatch_SIFT", "bfmatcher.cl", opts);
			_kidBruteForceMatch_nndrMatch_FREAK = _gpuComputeModule->registerKernel("bruteForceMatch_nndrMatch_FREAK", "bfmatcher.cl", opts);
			_kidBruteForceMatch_nndrMatch_ORB = _gpuComputeModule->registerKernel("bruteForceMatch_nndrMatch_ORB", "bfmatcher.cl", opts);
		}
		else
		{
			opts = "-DBLOCK_SIZE=16 -DDESC_LEN=64 -DKERNEL_NAME=bruteForceMatch_nndrMatch_SURF -DQUERY_TYPE=float -DDIST_TYPE=float -DDIST_FUNCTION=l2DistIter -DDIST_FINISH=l2DistFinish";
			_kidBruteForceMatch_nndrMatch_SURF = _gpuComputeModule->registerKernel("bruteForceMatch_nndrMatch_SURF", "bfmatcher_macros.cl", opts);

			opts = "-DBLOCK_SIZE=16 -DDESC_LEN=128 -DKERNEL_NAME=bruteForceMatch_nndrMatch_SIFT -DQUERY_TYPE=float -DDIST_TYPE=float -DDIST_FUNCTION=l2DistIter -DDIST_FINISH=l2DistFinish";
			_kidBruteForceMatch_nndrMatch_SIFT = _gpuComputeModule->registerKernel("bruteForceMatch_nndrMatch_SIFT", "bfmatcher_macros.cl", opts);

			opts = "-DBLOCK_SIZE=16 -DDESC_LEN=64 -DKERNEL_NAME=bruteForceMatch_nndrMatch_FREAK -DQUERY_TYPE=uchar -DDIST_TYPE=int -DDIST_FUNCTION=hammingDistIter -DDIST_FINISH=hammingDistFinish";
			_kidBruteForceMatch_nndrMatch_FREAK = _gpuComputeModule->registerKernel("bruteForceMatch_nndrMatch_FREAK", "bfmatcher_macros.cl", opts);

			opts = "-DBLOCK_SIZE=16 -DDESC_LEN=32 -DKERNEL_NAME=bruteForceMatch_nndrMatch_ORB -DQUERY_TYPE=uchar -DDIST_TYPE=int -DDIST_FUNCTION=hammingDistIter -DDIST_FINISH=hammingDistFinish";
			_kidBruteForceMatch_nndrMatch_ORB = _gpuComputeModule->registerKernel("bruteForceMatch_nndrMatch_ORB", "bfmatcher_macros.cl", opts);
		}

		return _kidBruteForceMatch_nndrMatch_SURF != InvalidKernelID
			&& _kidBruteForceMatch_nndrMatch_SIFT != InvalidKernelID
			&& _kidBruteForceMatch_nndrMatch_FREAK != InvalidKernelID
			&& _kidBruteForceMatch_nndrMatch_ORB != InvalidKernelID;
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

		vector<DMatch> matches;

		if(_symmetryTest)
		{
			vector<DMatch> matches1to2, matches2to1;

			auto status = nndrMatch_caller(query_dev, train_dev, matches1to2);
			if(status.status != EStatus::Ok)
				return status;

			status = nndrMatch_caller(train_dev, query_dev, matches2to1);
			if(status.status != EStatus::Ok)
				return status;

			matches = symmetryTest(matches1to2, matches2to1);
		}
		else
		{
			auto status = nndrMatch_caller(query_dev, train_dev, matches);
			if(status.status != EStatus::Ok)
				return status;
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
			formatMessage("Matches found: %d", (int) mt.queryPoints.size()));
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
	ExecutionStatus nndrMatch_caller(const DeviceArray& query_dev,
		const DeviceArray& train_dev,
		vector<DMatch>& matches)
	{
		if(query_dev.dataType() == EDataType::Float)
		{
			if(query_dev.width() == 64)
				matches = nndrMatch<16, 64>(query_dev, train_dev, _kidBruteForceMatch_nndrMatch_SURF);
			else if(query_dev.width() == 128)
				matches = nndrMatch<16, 128>(query_dev, train_dev, _kidBruteForceMatch_nndrMatch_SIFT);
			else 
				return ExecutionStatus(EStatus::Error, 
				"Unsupported descriptor data size "
				"(must be 64 or 128 elements width");
		}
		else if(query_dev.dataType() == EDataType::Uchar)
		{
			if(query_dev.width() == 32)
				matches = nndrMatch<16, 32>(query_dev, train_dev, _kidBruteForceMatch_nndrMatch_ORB);
			else if(query_dev.width() == 64)
				matches = nndrMatch<16, 64>(query_dev, train_dev, _kidBruteForceMatch_nndrMatch_FREAK);
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
	vector<DMatch> nndrMatch(const DeviceArray& query_dev,
		const DeviceArray& train_dev,
		KernelID kidBruteForceMatch)
	{
		// Ensure internal buffers are enough
		if(_matches_cl.isNull() || _matches_cl.size() < sizeof(DMatch) * query_dev.height())
		{
			_matches_cl = _gpuComputeModule->context().createBuffer(
				clw::Access_ReadWrite, clw::Location_Device, 
				sizeof(DMatch) * query_dev.height());
		}

		if(_matchesCount_cl.isNull())
		{
			_matchesCount_cl = _gpuComputeModule->context().createBuffer(
				clw::Access_ReadWrite, clw::Location_AllocHostMemory, sizeof(int));
		}

		int matchesCount = 0;
		_gpuComputeModule->queue().writeBuffer(_matchesCount_cl, &matchesCount);

		const size_t smemSize = (BLOCK_SIZE*std::max(DESCRIPTOR_LEN,BLOCK_SIZE) + BLOCK_SIZE*BLOCK_SIZE) * sizeof(int);
		clw::Kernel kernelNndrMatch = _gpuComputeModule->acquireKernel(kidBruteForceMatch);
		kernelNndrMatch.setLocalWorkSize(BLOCK_SIZE, BLOCK_SIZE);
		kernelNndrMatch.setRoundedGlobalWorkSize(query_dev.height(), BLOCK_SIZE);
		kernelNndrMatch.setArg(0, query_dev.buffer());
		kernelNndrMatch.setArg(1, train_dev.buffer());
		kernelNndrMatch.setArg(2, _matches_cl);
		kernelNndrMatch.setArg(3, _matchesCount_cl);
		kernelNndrMatch.setArg(4, clw::LocalMemorySize(smemSize));
		kernelNndrMatch.setArg(5, query_dev.height());
		kernelNndrMatch.setArg(6, train_dev.height());
		kernelNndrMatch.setArg(7, (float) _distanceRatio);
		_gpuComputeModule->queue().asyncRunKernel(kernelNndrMatch);

		// Read matches count
		_gpuComputeModule->queue().readBuffer(_matchesCount_cl, &matchesCount);

		if(matchesCount > 0)
		{
			// and allocate required size
			vector<DMatch> matches(matchesCount);

			// Read matches
			_gpuComputeModule->queue().readBuffer(_matches_cl, matches.data(), 
				0, matchesCount * sizeof(DMatch));

			return matches;
		}
		else
		{
			return vector<DMatch>();
		}
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
	KernelID _kidBruteForceMatch_nndrMatch_SURF;
	KernelID _kidBruteForceMatch_nndrMatch_SIFT;
	KernelID _kidBruteForceMatch_nndrMatch_FREAK; // also BRISK
	KernelID _kidBruteForceMatch_nndrMatch_ORB;
	KernelID _kidFillBuffer;

	clw::Buffer _matches_cl;
	clw::Buffer _matchesCount_cl;
};

REGISTER_NODE("OpenCL/Features/BForce Matcher", GpuBruteForceMatcherNodeType)

#endif
