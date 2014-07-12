/*
 * Copyright (c) 2013-2014 Kajetan Swierk <k0zmo@outlook.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

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
        if(!treeSerializer.deserializeJsonFromFile(nodeTree, filePath))
            throw std::runtime_error("Cannot open file " + filePath);

        // Get IDs of input and output nodes
        const NodeID inputNodeID = nodeTree->resolveNode("Image from file");
        const NodeID outputNodeID = nodeTree->resolveNode("Download image");

        if(inputNodeID == InvalidNodeID)
            throw std::logic_error("Couldn't find node named 'Image from file', terminating");
        if(outputNodeID == InvalidNodeID)
            throw std::logic_error("Couldn't find node named 'Download image', terminating");

        // Set some properties, e.g input filename
        nodeTree->nodeSetProperty(inputNodeID, nodeTree->resolveProperty(inputNodeID, "File path"), 
            Filepath{ "D:/Programowanie/Projects/mouve-assets/lion.png" });
        nodeTree->nodeSetProperty(inputNodeID, nodeTree->resolveProperty(inputNodeID, "Force grayscale"),
            false);

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
