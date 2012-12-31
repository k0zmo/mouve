#include "Controller.h"
#include "NodeScene.h"
#include "NodeView.h"
#include "NodeLinkView.h"

template<> Controller* Singleton<Controller>::_singleton = nullptr;

Controller::Controller(QWidget* parent, Qt::WFlags flags)
    : QMainWindow(parent, flags)
    , mNodeScene(new NodeScene(this))
{
    setupUi(this);

	// Set up a node scene
	/// xXx: Temporary
	mNodeScene->setSceneRect(-200,-200,1000,600);
	// Qt bug concering scene->removeItem ?? Seems to fixed it
	mNodeScene->setItemIndexMethod(QGraphicsScene::NoIndex);
	mGraphicsView->setScene(mNodeScene);

	// Context menu from node graphics view
	connect(mGraphicsView, SIGNAL(contextMenu(QPoint,QPointF)),
		this, SLOT(contextMenu(QPoint,QPointF)));
	// Key handler from node graphics view
	connect(mGraphicsView, SIGNAL(keyPress(QKeyEvent*)),
		this, SLOT(keyPress(QKeyEvent*)));
}

Controller::~Controller()
{
	foreach(NodeLinkView* link, mLinkViews)
		delete link;
	mLinkViews.clear();

	foreach(NodeView* node, mNodeViews.values())
		delete node;
	mNodeViews.clear();
}

