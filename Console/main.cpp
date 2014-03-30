#include "Logic/NodeSystem.h"
#include "Logic/NodeTree.h"
#include "Logic/NodeType.h"
#include "Logic/NodeLink.h"
#include "Logic/NodeException.h"
#include "Logic/NodeTreeSerializer.h"

#include "Logic/OpenCL/IGpuNodeModule.h"

#include <opencv2/highgui/highgui.hpp>
#include <stdexcept>
#include <iostream>

int main()
{
	try
	{
		// Create and register GPU Module
		std::shared_ptr<IGpuNodeModule> gpuModule = createGpuModule();
		gpuModule->setInteractiveInit(false);

		NodeSystem nodeSystem;
		nodeSystem.registerNodeModule(gpuModule);

		// Register Kuwahara plugin
		nodeSystem.loadPlugin("plugins/Plugin.Kuwahara.dll");

		// Load node tree from a file
		std::string filePath = "D:/Programowanie/Projects/mouve-assets/Kuwahara.tree";
		NodeTreeSerializer treeSerializer;        
		std::shared_ptr<NodeTree> nodeTree = nodeSystem.createNodeTree();
		if(!treeSerializer.deserializeJson(nodeTree, filePath))
			throw std::runtime_error("Cannot open file " + filePath);

		// Get IDs of input and output nodes
		const NodeID inputNodeID = nodeTree->resolveNode("Image from file");
		const NodeID outputNodeID = nodeTree->resolveNode("Download image");

		if(inputNodeID == InvalidNodeID)
			throw std::logic_error("Couldn't find node named 'Image from file', terminating");
		if(outputNodeID == InvalidNodeID)
			throw std::logic_error("Couldn't find node named 'Download image', terminating");

		// Set some properties, e.g input filename
		/*
		NodeConfig nodeConfig;
		nodeTree->nodeConfiguration(inputNodeID, nodeConfig);
		if(nodeConfig.pProperties != nullptr)
		{
			if(nodeConfig.pProperties[0].type == EPropertyType::Filepath)
			{
				nodeTree->nodeSetProperty(inputNodeID, 0, 
					Filepath("D:/Programowanie/Projects/mouve-assets/lion.png"));
			}
		}
		*/

		// Execute a node
		nodeTree->execute(true);

		// Get output data
		const NodeFlowData& outData = nodeTree->outputSocket(outputNodeID, 0);

		// Show it if possible
		cv::namedWindow("output from node tree");
		cv::imshow("output from node tree", outData.getImage());
		cv::waitKey(-1);
	}
	catch(ExecutionError& ex)
	{
		std::cerr << "Execution error in:\nNode: " << ex.nodeName << "\n"
			"Node typename: " << ex.nodeTypeName << "\n\nError message:\n" << ex.errorMessage;;
		std::cin.get();
	}
	catch(std::exception& ex)
	{
		std::cerr << ex.what() << std::endl;
		std::cin.get();
	}
}
