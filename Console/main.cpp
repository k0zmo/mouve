#include "Logic/NodeSystem.h"
#include "Logic/NodeTree.h"
#include "Logic/NodeType.h"
#include "Logic/NodeLink.h"
#include "Logic/NodeTreeSerializer.h"

//#include "Logic/OpenCL/IGpuNodeModule.h"

#include <opencv2/highgui/highgui.hpp>
#include <stdexcept>
#include <iostream>

int main()
{
	try
	{
		NodeSystem nodeSystem;
		NodeTreeSerializer treeSerializer("D:/Programowanie/Projects/mouve-assets");
		std::string filePath = "D:/Programowanie/Projects/mouve-assets/Histogram.tree";
		std::shared_ptr<NodeTree> nodeTree = nodeSystem.createNodeTree();

		//std::shared_ptr<NodeModule> gpuModule = createGpuModule();
		//nodeSystem.registerNodeModule(gpuModule);

		if(!treeSerializer.deserializeJson(nodeTree, filePath))
			throw std::runtime_error("Cannot open file " + filePath);

		// Get input and output nodes
		NodeID inputNodeID = nodeTree->resolveNode("Image from file");
		if(inputNodeID == InvalidNodeID)
			throw std::logic_error("Couldn't find node named 'Image from file', terminating");
		NodeID outputNodeID = nodeTree->resolveNode("Equalize histogram");
		if(outputNodeID == InvalidNodeID)
			throw std::logic_error("Couldn't find node named 'Equalize histogram', terminating");

		// Set some properties, e.g input filename
		NodeConfig nodeConfig;
		nodeTree->nodeConfiguration(inputNodeID, nodeConfig);
		if(nodeConfig.pProperties != nullptr)
		{
			if(nodeConfig.pProperties[0].type == EPropertyType::Filepath)
			{
				nodeTree->nodeSetProperty(inputNodeID, 0, 
					Filepath("D:/Programowanie/Projects/mouve-assets/lena.jpg"));
			}
		}

		// Execute a node
		nodeTree->execute(true);

		// Get output data
		const NodeFlowData& outData = nodeTree->outputSocket(outputNodeID, 0);

		// Show it if possible
		cv::namedWindow("output from node tree");
		cv::imshow("output from node tree", outData.getImage());
		cv::waitKey(-1);
	}
	catch(std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
		std::cin.get();
	}
}
