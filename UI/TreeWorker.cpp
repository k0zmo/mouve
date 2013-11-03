#include "TreeWorker.h"

#include "Logic/NodeTree.h"
#include "Logic/NodeFlowData.h"
#include "Logic/NodeException.h"

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
	bool res = false;
	try
	{
		if(_nodeTree)
			_nodeTree->execute(withInit);
		res = true;
	}
	catch(ExecutionError& ex)
	{
		emit error(QString("Execution error in:\nNode: %1\n"
			"Node typename: %2\n\nError message:\n%3")
				.arg(ex.nodeName.c_str())
				.arg(ex.nodeTypeName.c_str())
				.arg(ex.errorMessage.c_str()));
	}
	catch(...)
	{
		emit error(QStringLiteral("Internal error - Unknown exception was caught"));
	}

	emit completed(res);
}

