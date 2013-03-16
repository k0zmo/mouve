#include "TreeWorker.h"

#include "Logic/NodeTree.h"
#include "Logic/NodeFlowData.h"

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
	try
	{
		if(_nodeTree)
			_nodeTree->execute(withInit);
	}
	catch(boost::bad_get& ex)
	{
		emit error(QString("Bad socket connection, %1").arg(ex.what()));
	}
	catch(...)
	{
		emit error(QStringLiteral("Unknown exception was thrown"));
	}

	emit completed();
}

