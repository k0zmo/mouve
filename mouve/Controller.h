#pragma once

#include "Prerequisites.h"
#include "Singleton.h"

// UI
#include "ui_MainWindow.h"

//static bool DEBUG_LINKS = false;

class Controller
        : public QMainWindow
        , public Singleton<Controller>
        , private Ui::Controller
{
    Q_OBJECT    
public:
    explicit Controller(QWidget* parent = nullptr, Qt::WFlags flags = 0);
    virtual ~Controller();

private:
    QList<NodeLinkView*> mLinkViews;
    QHash<NodeID, NodeView*> mNodeViews;

    NodeScene* mNodeScene;
};

#define gC Controller::instancePtr()
