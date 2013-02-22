#pragma once

#include <QListWidget>
#include <QHBoxLayout>
#include <qlogging.h>

class LogView : public QWidget
{
public:
	explicit LogView(QWidget* parent = nullptr);
	~LogView() override;

private:
	static void messageHandler(QtMsgType type, 
		const QMessageLogContext& context, const QString& msg);

private:
	QHBoxLayout* _layout;
	QListWidget* _widget;

	// Intrusive list of all created log views (in distant future: all log channels)
	LogView* _nextView;
	static LogView* _headView;
};

