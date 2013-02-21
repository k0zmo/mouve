#include "PropertyWidgets.h"

#include <QMouseEvent>
#include <QHBoxLayout>

#include <QDirModel>
#include <QCompleter>
#include <QFileIconProvider>
#include <QStyle>

#include <QPainter>
#include <QStyle>
#include <QStyleOptionFrame>
#include <QShortcut>
#include <QToolButton>
#include <QFileDialog>

PropertySpinBox::PropertySpinBox(QWidget* parent)
	: QSpinBox(parent)
{
	// Why so ugly syntax ?? - cuz of overload
	connect(this, static_cast<void (QSpinBox::*)(int)>
		(&QSpinBox::valueChanged),
		[=](int) {
			emit commitData();
		});
}

PropertyDoubleSpinBox::PropertyDoubleSpinBox(QWidget* parent)
	: QDoubleSpinBox(parent)
{
	connect(this, static_cast<void (QDoubleSpinBox::*)(double)>
		(&QDoubleSpinBox::valueChanged),
		[=](double) {
			emit commitData();
		});
}

PropertyComboBox::PropertyComboBox(QWidget* parent)
	: QComboBox(parent)
{
	connect(this, static_cast<void (QComboBox::*)(int)>
		(&QComboBox::currentIndexChanged),
		[=](int) {
			emit commitData();
		});
}

PropertyCheckBox::PropertyCheckBox(QWidget* parent)
	: QWidget(parent)
	, _checkBox(new QCheckBox(this))
{
	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->setContentsMargins(4, 0, 0, 0);
	layout->addWidget(_checkBox);

	connect(_checkBox, &QCheckBox::toggled, [=](bool){
		emit commitData();
	});

	/// TODO: This causes pretty weird behaviour on Windows 
	//setFocusProxy(_checkBox);
}

void PropertyCheckBox::mousePressEvent(QMouseEvent* event)
{
	if(event->button() == Qt::LeftButton)
	{
		_checkBox->click();
		event->accept();
	}
	else
	{
		QWidget::mousePressEvent(event);
	}
}

ShortenTextLineEdit::ShortenTextLineEdit(QWidget* parent)
	: QLineEdit(parent)
	, _drawShortenMessage(true)
	, _shortenMessage("")
{
	//auto model = new QFileSystemModel(this);
	//model->setRootPath("/");

	auto model = new QDirModel(this);
	auto completer = new QCompleter(this);

	completer->setModel(model);
	setCompleter(completer);

	connect(this, &QLineEdit::textChanged, this,
			&ShortenTextLineEdit::textChangedPrivate);

	auto* sc = new QShortcut(QKeySequence("Ctrl+Space"), this);
	connect(sc, &QShortcut::activated, [=](){
		completer->complete();
	});

	QStyleOption opt;
	opt.initFrom(this);

	_margin = style()->pixelMetric(QStyle::PM_DefaultFrameWidth, &opt, this);
	_size = style()->pixelMetric(QStyle::PM_SmallIconSize, &opt, this);
	setTextMargins(_size + 2, 0, 0, 0);
}

void ShortenTextLineEdit::paintEvent(QPaintEvent* event)
{
	bool drawShorten = _drawShortenMessage
		&& !_shortenMessage.isEmpty()
		&& !hasFocus();

	auto drawIcon = [this](QPainter* p)
	{
		_icon.paint(p, QRect(_margin+1, _margin+1, _size, _size),
			Qt::AlignCenter, QIcon::Normal);
	};
	
	if(drawShorten)
	{
		// Draw widget with icon

		QPainter p(this);
		QStyleOptionFrameV2 panel;
		initStyleOption(&panel);
		style()->drawPrimitive(QStyle::PE_PanelLineEdit, &panel, &p, this);
		drawIcon(&p);

		// Get clip rect for text

		QRect r = style()->subElementRect(QStyle::SE_LineEditContents, &panel, this);

		int left, top, right, bottom;
		getTextMargins(&left, &top, &right, &bottom);
		r.adjust(left, top, -right, -bottom);

		p.setClipRect(r);

		static const int horizontalMargin = 2;

		QFontMetrics fm = fontMetrics();
		QRect lineRect(r.x() + horizontalMargin,
			r.y() + (r.height() - fm.height() + 1) / 2,
			r.width() - 2*horizontalMargin,
			fm.height());

		// Set font properties and draw text

		p.setFont(font());
		QColor color(palette().color(QPalette::Foreground));
		color.setAlphaF(0.45);
		p.setPen(color);

		p.drawText(lineRect, Qt::AlignLeft | Qt::AlignVCenter, _shortenMessage);
	}
	else
	{
		QLineEdit::paintEvent(event);

		QPainter p(this);
		drawIcon(&p);
	}
}

void ShortenTextLineEdit::focusInEvent(QFocusEvent* event)
{
	if(_drawShortenMessage && !_shortenMessage.isEmpty())
	{
		_drawShortenMessage = false;
		update();
	}
	QLineEdit::focusInEvent(event);
}

void ShortenTextLineEdit::focusOutEvent(QFocusEvent* event)
{
	if(!_drawShortenMessage && !_shortenMessage.isEmpty())
	{
		_drawShortenMessage = true;
		update();
	}
	QLineEdit::focusOutEvent(event);
}

void ShortenTextLineEdit::textChangedPrivate(const QString& text)
{
	QFileIconProvider prov;
	_fileInfo = QFileInfo(text);
	if(_fileInfo.exists())
	{
		if(_fileInfo.isRoot())
			_shortenMessage = _fileInfo.absolutePath();
		else
			_shortenMessage = _fileInfo.fileName();
	}
	else
	{
		_shortenMessage = text;
	}
	_icon = prov.icon(_fileInfo);
	update();
}

FileRequester::FileRequester(QWidget* parent)
	: QWidget(parent)
	, _fileNameLineEdit(new ShortenTextLineEdit(this))
	, _openButton(new QToolButton(this))
{
	QHBoxLayout* layout = new QHBoxLayout(this);

	setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed, QSizePolicy::LineEdit));

	_openButton->setIcon(QIcon::fromTheme("document-open",
		QIcon(":/qt-project.org/styles/commonstyle/"
		"images/standardbutton-open-16.png")));

	layout->addWidget(_fileNameLineEdit, 1);
	layout->addWidget(_openButton);
	layout->setSpacing(3);

	connect(_openButton, &QToolButton::clicked,
		this, &FileRequester::openFileDialog);
	_openButton->setAutoRaise(true);
}

void FileRequester::openFileDialog()
{
	//QFileDialog::FileMode fm = QFileDialog::ExistingFile;
	QString path = QFileDialog::getOpenFileName(this, "Choose a file",
		fileInfo().absolutePath(), QString());

	if(!path.isEmpty())
	{
		_fileNameLineEdit->setText(path);
		_fileNameLineEdit->setFocus();
	}
}

void FileRequester::setFilePath(const QString& path)
{
	_fileNameLineEdit->setText(path);
}

LineEditReset::LineEditReset(QWidget* parent)
	: QLineEdit(parent)
	, _toolButton(new QToolButton(this))
{
	_toolButton->setIcon(QIcon::fromTheme(
		"edit-clear", QIcon(":/qt-project.org/styles/commonstyle/"
							"images/standardbutton-close-16.png")));
	_toolButton->setCursor(Qt::ArrowCursor);
	//_toolButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
	//_toolButton->setStyleSheet("QToolButton { padding: 0px; }");
	_toolButton->setAutoRaise(true);
	_toolButton->hide();

	connect(_toolButton, &QToolButton::clicked, this, &QLineEdit::clear);
	connect(this, &QLineEdit::textChanged, this, &LineEditReset::updateCloseButton);

	int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

	setStyleSheet(QString("QLineEdit { padding-right: %1px; }")
		.arg(_toolButton->sizeHint().width() + frameWidth + 1));
	QSize sz = minimumSizeHint();
	setMinimumSize(qMax(sz.width(), _toolButton->sizeHint().width() + frameWidth * 2 + 2),
		qMax(sz.height(), _toolButton->sizeHint().height() + frameWidth * 2 + 2));
}

void LineEditReset::resizeEvent(QResizeEvent* event)
{
	QSize sz = _toolButton->sizeHint();
	int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
	_toolButton->move(rect().right() - frameWidth - sz.width(),
		(rect().bottom() + 1 - sz.height())/2);
}

void LineEditReset::updateCloseButton(const QString& text)
{
	_toolButton->setVisible(!text.isEmpty());
}