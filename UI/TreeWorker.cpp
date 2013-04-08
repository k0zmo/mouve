#include "TreeWorker.h"

#include "Logic/NodeTree.h"
#include "Logic/NodeFlowData.h"
#include "Logic/OpenCL/GpuException.h"

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
	catch(boost::bad_get& ex)
	{
		emit error(QString("Bad socket connection, %1").arg(ex.what()));
	}
	catch(GpuBuildException& ex)
	{
		QString logMessage = QString::fromStdString(ex.log);
		logMessage.truncate(1024);
		logMessage.append("\n...");
		
		emit error(
			QString("Building program failed:\n") + 
			logMessage
		);
	}
	catch(GpuNodeException& ex)
	{
		emit error(QString("OpenCL error(%1): %2")
			.arg(ex.error)
			.arg(QString::fromStdString(ex.message))
		);
	}
	catch(cv::Exception& ex)
	{
		emit error(QString("OpenCV exception caught: %1").arg(ex.what()));
	}
	catch(std::exception& ex)
	{
		emit error(QString("Exception was caught: %1").arg(ex.what()));
	}
	catch(...)
	{
		emit error(QStringLiteral("Unknown exception was thrown"));
	}

	emit completed(res);
}

