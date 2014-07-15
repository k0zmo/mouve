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
#include "Logic/NodeResolver.h"

#include "Logic/OpenCL/IGpuNodeModule.h"

#include <opencv2/highgui/highgui.hpp>
#include <stdexcept>
#include <iostream>

using namespace std;

void nodeReflection(NodeTree& nodeTree, NodeID nodeID)
{
    NodeConfig nodeConfig;
    nodeTree.nodeConfiguration(nodeID, nodeConfig);

    cout << endl << "Reflection of " << nodeTree.nodeTypeName(nodeID) << endl;
    const InputSocketConfig* iscIter = begin_config<InputSocketConfig>(nodeConfig);
    while (!end_config(iscIter))
    {
        cout << "<+> input socket: " << iscIter->name << "(" << to_string(iscIter->dataType) << ")" << endl;
        ++iscIter;
    }

    const OutputSocketConfig* oscIter = begin_config<OutputSocketConfig>(nodeConfig);
    while (!end_config(oscIter))
    {
        cout << "<+> output socket: " << oscIter->name << "(" << to_string(oscIter->dataType) << ")" << endl;
        ++oscIter;
    }

    const PropertyConfig* pcIter = begin_config<PropertyConfig>(nodeConfig);
    while (!end_config(pcIter))
    {
        cout << "<+> property: " << pcIter->name << "(" << to_string(pcIter->type) << ")" << endl;
        ++pcIter;
    }
}

int main()
{
    try
    {
        NodeSystem nodeSystem;

        // Create and register GPU Module
        shared_ptr<IGpuNodeModule> gpuModule = createGpuModule();
        gpuModule->setInteractiveInit(false); // Just use default device
        nodeSystem.registerNodeModule(gpuModule);

        // Register Kuwahara plugin
#if K_SYSTEM == K_SYSTEM_WINDOWS
        nodeSystem.loadPlugin("plugins/Plugin.Kuwahara.dll");
#else
        nodeSystem.loadPlugin("plugins/libPlugin.Kuwahara.so");
#endif

        // Print out all registered node types
        cout << "Available node types: \n";
        unique_ptr<NodeTypeIterator> typeIter = nodeSystem.createNodeTypeIterator();
        NodeTypeIterator::NodeTypeInfo nodeTypeInfo;
        while (typeIter->next(nodeTypeInfo))
            cout << nodeTypeInfo.typeID << ": " << nodeTypeInfo.typeName << endl;

        {
            shared_ptr<NodeTree> nodeTree = nodeSystem.createNodeTree();
        
            NodeID inputImageID = nodeTree->createNode("Sources/Image from file", "Input image");
            NodeID uploadID = nodeTree->createNode("OpenCL/Upload image", "Upload");
            NodeID akfID = nodeTree->createNode("OpenCL/Filters/Anisotropic Kuwahara filter", "AKF");
            NodeID downloadID = nodeTree->createNode("OpenCL/Download image", "Download");

            nodeReflection(*nodeTree, inputImageID);
            nodeReflection(*nodeTree, uploadID);
            nodeReflection(*nodeTree, akfID);
            nodeReflection(*nodeTree, downloadID);

            NodeResolver resolver(nodeTree);

            nodeTree->linkNodes(resolver.resolveSocketAddress("o://Input image/output"),
                                resolver.resolveSocketAddress("i://Upload/input"));
            nodeTree->linkNodes(resolver.resolveSocketAddress("o://Upload/output"),
                                resolver.resolveSocketAddress("i://AKF/source"));
            nodeTree->linkNodes(resolver.resolveSocketAddress("o://AKF/output"),
                                resolver.resolveSocketAddress("i://Download/input"));

            // Set some properties, e.g input filename
            nodeTree->nodeSetProperty(inputImageID, 
                resolver.resolveProperty(inputImageID, "File path"), 
                Filepath{ "rgb.png" });
            nodeTree->nodeSetProperty(inputImageID, 
                resolver.resolveProperty(inputImageID, "Force grayscale"),
                false);
            nodeTree->nodeSetProperty(akfID,
                resolver.resolveProperty(akfID, "Radius"),
                8);

            // Execute a node
            nodeTree->execute(true);

            NodeTreeSerializer nodeTreeSerializer;
            nodeTreeSerializer.serializeJsonToFile(nodeTree, "example.tree");

            // Get output data
            const NodeFlowData& outData = nodeTree->outputSocket(downloadID, 
                resolver.resolveOutputSocket(downloadID, "output"));

            // Show it if possible
            cv::namedWindow("Output from node tree");
            cv::imshow("Output from node tree", outData.getImage());
            cv::waitKey(-1);
        }

        {
            // Load a tree from a file
            shared_ptr<NodeTree> nodeTree = nodeSystem.createNodeTree();
            NodeTreeSerializer nodeTreeSerializer;
            if(!nodeTreeSerializer.deserializeJsonFromFile(nodeTree, "example.tree"))
                throw runtime_error("Cannot open file example.tree");

            NodeResolver resolver(nodeTree);

            // Change some property to see a small difference
            NodeID inputImageID = resolver.resolveNode("Input image");
            nodeTree->nodeSetProperty(inputImageID, 
                resolver.resolveProperty(inputImageID, "Force grayscale"),
                true);

            nodeTree->execute(true);

            // Get output data
            NodeID downloadID = resolver.resolveNode("Download");
            const NodeFlowData& outData = nodeTree->outputSocket(downloadID, 
                resolver.resolveOutputSocket(downloadID, "output"));

            // Show it if possible
            cv::namedWindow("Output from node tree");
            cv::imshow("Output from node tree", outData.getImage());
            cv::waitKey(-1);
        }
    }
    catch(ExecutionError& ex)
    {
        cerr << "Execution error in:\nNode: " << ex.nodeName << "\n"
            "Node typename: " << ex.nodeTypeName << "\n\nError message:\n" << ex.errorMessage;;
        cin.get();
    }
    catch(exception& ex)
    {
        cerr << ex.what() << endl;
        cin.get();
    }
}
