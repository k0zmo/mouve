
#pragma once

#include "Prerequisites.h"

#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QIcon>
#include <QFileInfo>

class QToolButton;
class QMouseEvent;

class PropertySpinBox : public QSpinBox
{
	Q_OBJECT
public:
	explicit PropertySpinBox(QWidget* parent = nullptr);

signals:
	void commitData();
};

class PropertyDoubleSpinBox : public QDoubleSpinBox
{
	Q_OBJECT
public:
	explicit PropertyDoubleSpinBox(QWidget* parent = nullptr);

signals:
	void commitData();
};

class PropertyComboBox : public QComboBox
{
	Q_OBJECT
public :
	explicit PropertyComboBox(QWidget* parent = nullptr);

signals:
	void commitData();
};

class PropertyCheckBox : public QWidget
{
	Q_OBJECT
public:
	explicit PropertyCheckBox(QWidget* parent = nullptr);

	bool isChecked() const;
	void blockCheckBoxSignals(bool block);

public slots:
	void setChecked(bool c);

signals:
	void commitData();

protected:
	void mousePressEvent(QMouseEvent* event) override;

private:
	QCheckBox* _checkBox;
};
/*
class ShortenTextLineEdit : public QLineEdit
{
	Q_OBJECT
public:
	explicit ShortenTextLineEdit(QWidget* parent = nullptr);

	QFileInfo fileInfo() const;

protected:
	void paintEvent(QPaintEvent* event) override;
	void focusInEvent(QFocusEvent* event) override;
	void focusOutEvent(QFocusEvent* event) override;

private:
	bool _drawShortenMessage;
	QString _shortenMessage;
	QIcon _icon;
	QFileInfo _fileInfo;
	int _size;
	int _margin;

private slots:
	void textChangedPrivate(const QString& text);
};

class FileRequester : public QWidget
{
	Q_OBJECT
public:
	explicit FileRequester(QWidget* parent = nullptr);

	QFileInfo fileInfo() const;
	QString filePath() const;

	/// TODO:
	/// filter
	/// filemod
	
public slots:
	void openFileDialog();
	void setFilePath(const QString& path);

private:
	ShortenTextLineEdit* _fileNameLineEdit;
	QToolButton* _openButton;
};
*/

class FileRequester : public QWidget
{
	Q_OBJECT
public :
	explicit FileRequester(QWidget* parent = nullptr);

	QString text() const;

	void blockLineEditSignals(bool block);
	
	/// TODO:
	/// filter
	/// filemod

public slots:
	void openFileDialog();
	void setText(const QString& text);

private:
	QLineEdit* _fileNameLineEdit;
	QToolButton* _openButton;
};


/// TODO: dodac widgeta, ktory opakowuje innego i dostawia z prawej strony tool buttona do resetowania
/*
class LineEditReset : public QLineEdit
{
	Q_OBJECT
public:
	explicit LineEditReset(QWidget* parent = nullptr);

protected: 
	void resizeEvent(QResizeEvent* event) override;

private slots:
	void updateCloseButton(const QString& name);

private: 
	QToolButton* _toolButton;
};
*/

inline bool PropertyCheckBox::isChecked() const
{ return _checkBox->isChecked(); }

inline void PropertyCheckBox::setChecked(bool c)
{ _checkBox->setChecked(c); }

inline void PropertyCheckBox::blockCheckBoxSignals(bool block)
{ _checkBox->blockSignals(block); }

/*
inline QFileInfo ShortenTextLineEdit::fileInfo() const
{ return _fileInfo; }

inline QFileInfo FileRequester::fileInfo() const
{ return _fileNameLineEdit->fileInfo(); }

inline QString FileRequester::filePath() const
{ return fileInfo().filePath(); }
*/