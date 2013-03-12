#pragma once

#include <QListWidget>
#include <QHBoxLayout>
#include <QIcon>
#include <qlogging.h>

class LogView : public QWidget
{
	Q_OBJECT
public:
	explicit LogView(QWidget* parent = nullptr);
	~LogView() override;

public slots:
	void critical(const char* msg);

private:
	static void messageHandler(QtMsgType type, 
		const QMessageLogContext& context, const QString& msg);

private:
	QHBoxLayout* _layout;
	QListWidget* _widget;

	// Intrusive list of all created log views (in distant future: all log channels)
	LogView* _nextView;
	static LogView* _headView;
	static QIcon _warningIcon;
	static QIcon _infoIcon;
	static QIcon _errorIcon;
};

