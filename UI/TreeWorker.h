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

signals:
	void completed();
	void error(char const* msg);

public slots:
	void process(bool withInit);

private:
	std::shared_ptr<NodeTree> _nodeTree;
};

