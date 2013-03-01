#include "BuiltinNodeTypes.h"

REGISTER_NODE("Mixture of Gaussians", MixtureOfGaussianNodeType)
REGISTER_NODE("Canny edge detector", CannyEdgeDetectorNodeType)

REGISTER_NODE("Convolution", CustomConvolutionNodeType)
REGISTER_NODE("Predefined convolution", PredefinedConvolutionNodeType)
REGISTER_NODE("Morphology op.", MorphologyNodeType)
REGISTER_NODE("Sobel filter", SobelFilterNodeType)
REGISTER_NODE("Gaussian blur", GaussianBlurNodeType)
REGISTER_NODE("Binarization", BinarizationNodeType)

REGISTER_NODE("Negate", NegateNodeType)
REGISTER_NODE("Absolute diff.", AbsoluteDifferenceNodeType)
REGISTER_NODE("Subtract", SubtractNodeType)
REGISTER_NODE("Absolute", AbsoluteNodeType)
REGISTER_NODE("Add", AddNodeType)

REGISTER_NODE("Structuring element", StructuringElementNodeType)
REGISTER_NODE("Video from file", VideoFromFileNodeType)
REGISTER_NODE("Image from file", ImageFromFileNodeType)
