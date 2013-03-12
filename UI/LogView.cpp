#include "LogView.h"

#include <QDateTime>

LogView* LogView::_headView = nullptr;
QIcon LogView::_warningIcon;
QIcon LogView::_infoIcon;
QIcon LogView::_errorIcon;

LogView::LogView(QWidget* parent)
	: QWidget(parent)
	, _widget(new QListWidget(this))
{
	// register message handler - but only once
	if(!_headView)
	{
		qInstallMessageHandler(LogView::messageHandler);

		// open icon
		_warningIcon = QIcon(":/images/exclamation-white.png");
		_infoIcon = QIcon(":/images/question-baloon.png");
		_errorIcon = QIcon(":/images/exclamation-black.png");
	}

	// append at the beginning
	_nextView = _headView;
	_headView = this;

	_layout = new QHBoxLayout(this);
	_layout->addWidget(_widget);
	_layout->setContentsMargins(0, 0, 0, 0);
	_layout->setSpacing(0);
}

LogView::~LogView()
{
	// remove yourself from the list
	auto it = _headView;
	while(it->_nextView && it->_nextView != this)
		it = it->_nextView;

	if(it->_nextView == this)
		it->_nextView = _nextView;
}

void LogView::debug(const char* msg)
{
	qDebug(msg);
}

void LogView::warn(const char* msg)
{
	qWarning(msg);
}

void LogView::critical(const char* msg)
{
	qCritical(msg);
}

void LogView::messageHandler(QtMsgType type, 
							 const QMessageLogContext& context,
							 const QString& msg)
{
	// This should be fixed in 5.0.2
	if(msg == "QWindowsNativeInterface::nativeResourceForWindow: 'handle' "
		"requested for null window or window without handle.")
		return;

	// If this isn't UI thread
	if(QThread::currentThread() != QCoreApplication::instance()->thread())
	{
		return;
	}

	auto it = _headView;
	while(it)
	{
		auto item = new QListWidgetItem(msg);

		item->setToolTip(QDateTime::currentDateTime()
			.toString("[MM.dd.yyyy hh:mm:ss.zzz]"));

		switch(type)
		{
		case QtDebugMsg:
			item->setIcon(_infoIcon);
			break;

		case QtWarningMsg:
			item->setIcon(_warningIcon);
			break;

		case QtCriticalMsg:
			item->setBackgroundColor(Qt::red);
			item->setTextColor(Qt::blue);
			item->setIcon(_errorIcon);
			break;

		case QtFatalMsg:
			item->setBackgroundColor(Qt::red);
			item->setTextColor(Qt::yellow);
			item->setIcon(_errorIcon);
			break;
		}

		it->_widget->addItem(item);
		it->_widget->scrollToBottom();

		// go to next logView
		it = it->_nextView;
	}
}