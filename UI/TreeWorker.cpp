#include "TreeWorker.h"

#include "Logic/NodeTree.h"

TreeWorker::TreeWorker(QObject* parent)
	: QObject(parent)
{
}

TreeWorker::~TreeWorker()
{
}

void TreeWorker::setNodeTree(const std::shared_ptr<NodeTree>& nodeTree)
{
	// Could use mutex here but we try really hard not to call it when process() is working
	_nodeTree = nodeTree;
}

void TreeWorker::process(bool withInit)
{
	if(_nodeTree)
		_nodeTree->execute(withInit);

	emit completed();
}

