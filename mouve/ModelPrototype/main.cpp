#include "Prerequisites.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "Node.h"
#include "NodeType.h"
#include "NodeTree.h"
#include "NodeLink.h"
#include "NodeSystem.h"

#include "BuiltinNodeTypes.h"

#if 0

void dependencyCheck()
{
	NodeTree tree;

	NodeID n0 = tree.createNode(2, "n0");
	NodeID n1 = tree.createNode(3, "n1");
	NodeID n2 = tree.createNode(5, "n2");
	NodeID n3 = tree.createNode(6, "n3");
	NodeID n4 = tree.createNode(4, "n4");
	NodeID n5 = tree.createNode(1, "n5");
	NodeID n6 = tree.createNode(6, "n6");
	NodeID n7 = tree.createNode(3, "n7");

	tree.linkNodes(SocketAddress(n0, 0, true), SocketAddress(n1, 0, false));
	tree.linkNodes(SocketAddress(n1, 0, true), SocketAddress(n6, 0, false));
	tree.linkNodes(SocketAddress(n0, 0, true), SocketAddress(n2, 0, false));
	tree.linkNodes(SocketAddress(n2, 0, true), SocketAddress(n7, 0, false));
	tree.linkNodes(SocketAddress(n7, 0, true), SocketAddress(n6, 1, false));
	tree.linkNodes(SocketAddress(n0, 1, true), SocketAddress(n3, 0, false));
	tree.linkNodes(SocketAddress(n5, 0, true), SocketAddress(n4, 0, false));
	tree.linkNodes(SocketAddress(n4, 0, true), SocketAddress(n3, 1, false));
	tree.linkNodes(SocketAddress(n4, 1, true), SocketAddress(n2, 1, false));

	tree.tagNode(n5);
	tree.tagNode(n0);

	tree.step();
}

#endif

int main()
{
	NodeSystem sys;

	std::unique_ptr<NodeFactory> factoryPtr0 = std::unique_ptr<NodeFactory>(new DefaultNodeFactory<ImageFromFileNodeType>());
	NodeTypeID ImageFromDiskTypeID = sys.registerNodeType("ImageFromDisk", std::move(factoryPtr0));

	std::unique_ptr<NodeFactory> factoryPtr1 = std::unique_ptr<NodeFactory>(new DefaultNodeFactory<GaussianBlurNodeType>());
	NodeTypeID GaussianBlurTypeID = sys.registerNodeType("GaussianBlur", std::move(factoryPtr1));

	std::unique_ptr<NodeFactory> factoryPtr2 = std::unique_ptr<NodeFactory>(new DefaultNodeFactory<CannyEdgeDetector>());
	NodeTypeID CannyEdgeDetectorTypeID = sys.registerNodeType("CannyEdgeDetector", std::move(factoryPtr2));

	auto nti = sys.createNodeTypeIterator();
	NodeTypeIterator::NodeTypeInfo info;
	while(nti->next(info))
	{
		std::cout << info.typeID << " = " << info.typeName << "\n";
	}

	auto tree = sys.createNodeTree();

	NodeID fs = tree->createNode(ImageFromDiskTypeID, "Image from disk");
	NodeID gb = tree->createNode(GaussianBlurTypeID, "Gaussian blur");
	NodeID ca = tree->createNode(CannyEdgeDetectorTypeID, "Canny edge detector");

	tree->linkNodes(SocketAddress(fs, 0, true), SocketAddress(gb, 0, false));
	tree->linkNodes(SocketAddress(gb, 0, true), SocketAddress(ca, 0, false));

	// Lista wezlow
	NodeID nodeID;
	const Node* node;
	auto ni = tree->createNodeIterator();
	while((node = ni->next(nodeID)) != nullptr)
	{
		std::cout << node->nodeName() << 
			" (NodeID: " << nodeID << 
			", NodeTypeID: " <<	node->nodeTypeID() << 
			") [Inputs:" << int(node->numInputSockets()) << 
			" , Outputs:" << int(node->numOutputSockets()) <<
			"]\n";
	}

	// Lista polaczen
	NodeLink nodeLink;
	auto nli = tree->createNodeLinkIterator();
	while(nli->next(nodeLink))
	{
		std::cout << int(nodeLink.fromNode) << "." << int(nodeLink.fromSocket) << " -> " << 
			int(nodeLink.toNode) << "." << int(nodeLink.toSocket) << "\n";
	}

	tree->tagNode(fs);
	tree->step();

	cv::imshow("", tree->outputSocket(ca, 0));
	cv::waitKey(0);
}
