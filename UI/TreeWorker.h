#pragma once

#include "Prerequisites.h"

#include <QObject>

class TreeWorker : public QObject
{
	Q_OBJECT
public:
	explicit TreeWorker(QObject* parent = nullptr);
	~TreeWorker() override;

	void setNodeTree(const std::shared_ptr<NodeTree>& nodeTree);
	Q_INVOKABLE void process(bool withInit);

signals:
	void completed(bool res);
	void error(const QString& msg);

private:
	std::shared_ptr<NodeTree> _nodeTree;
};
