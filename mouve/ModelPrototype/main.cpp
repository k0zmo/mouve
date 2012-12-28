#include "Prerequisites.h"

#include <opencv2/core/core.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>

#include "Node.h"
#include "NodeType.h"
#include "NodeTree.h"
#include "NodeLink.h"

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
	NodeTree tree;

	NodeID fs = tree.createNode(1, "ImageFromDisk");
	NodeID gb = tree.createNode(2, "GaussianBlur");
	NodeID ca = tree.createNode(3, "CannyEdgeDetector");

	tree.linkNodes(SocketAddress(fs, 0, true), SocketAddress(gb, 0, false));
	tree.linkNodes(SocketAddress(gb, 0, true), SocketAddress(ca, 0, false));

	tree.tagNode(fs);

	tree.step();

	const cv::Mat& output = tree.outputSocket(ca, 0);
	cv::imshow("", output);
	cv::waitKey(0);
}
