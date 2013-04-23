#include "PropertyWidgets.h"

#include <QMouseEvent>
#include <QHBoxLayout>
#include <QCompleter>
#include <QFileSystemModel>
#include <QShortcut>
#include <QToolButton>
#include <QFileDialog>
#include <QDialogButtonBox>

PropertySpinBox::PropertySpinBox(QWidget* parent)
	: QSpinBox(parent)
{
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

	setFocusProxy(_checkBox);
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

FileRequester::FileRequester(QWidget* parent)
	: QWidget(parent)
	, _fileNameLineEdit(new QLineEdit(this))
	, _openButton(new QToolButton(this))
	, _filter()
	, _fsModel(new QFileSystemModel(this))
{
	_openButton->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::MinimumExpanding));;
	_openButton->setText("");
	_openButton->setAutoRaise(true);
	_openButton->setIcon(QIcon::fromTheme("document-open",
		QIcon(":/qt-project.org/styles/commonstyle/"
			  "images/standardbutton-open-16.png")));

	connect(_openButton, &QToolButton::clicked,
		this, &FileRequester::openFileDialog); 
	connect(_fileNameLineEdit, &QLineEdit::textChanged, 
		this, &FileRequester::_q_textChangedPrivate);

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->addWidget(_fileNameLineEdit, 1);
	layout->addWidget(_openButton);
	layout->setSpacing(0);
	layout->setContentsMargins(0, 0, 0, 0);

	setSizePolicy(QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed, QSizePolicy::LineEdit));
	setFocusProxy(_fileNameLineEdit);

	_fsModel->setRootPath("/");

	auto completer = new QCompleter(this);
	completer->setMaxVisibleItems(15);
	completer->setModel(_fsModel);
	_fileNameLineEdit->setCompleter(completer);

	connect(_fsModel, &QFileSystemModel::directoryLoaded,
		[completer](const QString&){
			completer->complete();
	});

	auto* sc = new QShortcut(QKeySequence("Ctrl+Space"), this);
	connect(sc, &QShortcut::activated, [completer](){
		completer->complete();
	});
}

QString FileRequester::text() const
{
	return _fileNameLineEdit->text();
}

QString FileRequester::filter() const
{
	return _filter;
}

void FileRequester::blockLineEditSignals(bool block)
{
	_fileNameLineEdit->blockSignals(block);
}

void FileRequester::openFileDialog()
{
	/// TODO: filemod - existing, existings, read, write, etc...
	QString path = QFileDialog::getOpenFileName(this, "Choose a file",
		QFileInfo(text()).absolutePath(), _filter);

	if(!path.isEmpty())
	{
		_fileNameLineEdit->setText(path);
		_fileNameLineEdit->setFocus();
	}
}

void FileRequester::setText(const QString& text)
{
	_fileNameLineEdit->setText(text);
}

void FileRequester::setFilter(const QString& filter)
{
	_filter = filter;

	QStringList tokens = filter.split(";;");
	// Loosely based on Qt source (qplatformdialoghelper.cpp)
	QRegExp regexp("^(.*)\\(([a-zA-Z0-9_.*? +;#\\-\\[\\]@\\{\\}/!<>\\$%&=^~:\\|]*)\\)$");
	QStringList nameFilters;

	for(const QString& token : tokens)
	{
		QString f = token;
		int i = regexp.indexIn(f);
		if(i >= 0)
			f = regexp.cap(2);
		QStringList fs = f.split(QLatin1Char(' '), QString::SkipEmptyParts);
		
		for(const QString& filter : fs)
		{
			if(filter != "*.*")
				nameFilters.append(filter);
		}
	}
	
	if(nameFilters.empty())
		nameFilters.append("*.*");
	_fsModel->setNameFilters(nameFilters);
}

void FileRequester::_q_textChangedPrivate(const QString& text)
{
	QFileInfo fi(text);
	if(fi.exists() && fi.isFile())
		emit commitData();
}

PropertyMatrixButton::PropertyMatrixButton(QWidget* parent)
	: QPushButton(parent)
{
	setText(tr("Choose"));
	connect(this, SIGNAL(clicked()), this, SLOT(showDialog()));
}

void PropertyMatrixButton::showDialog()
{
	int ok;
	Matrix3x3 tmpMatrix = PropertyMatrixDialog::getCoefficients(this, _matrix, &ok);

	if(ok)
		setMatrix(tmpMatrix);
}

void PropertyMatrixButton::setMatrix(const Matrix3x3& mat)
{
	_matrix = mat;

	emit commitData();
}

Matrix3x3 PropertyMatrixButton::matrix() const
{
	return _matrix;
}

PropertyMatrixDialog::PropertyMatrixDialog(const Matrix3x3& matrix, QWidget* parent)
	: QDialog(parent)
	, _normalizeCheckBox(new QCheckBox(this))
{
	resize(215, 150);
	setMinimumSize(215, 150);
	setMaximumSize(215, 150);
	setWindowTitle(tr("Matrix coefficients"));
	
	auto gridLayout = new QGridLayout();
	
	for(int y = 0; y < 3; ++y)
	{
		for(int x = 0; x < 3; ++x)
		{
			auto sb = new QDoubleSpinBox();
			sb->setMinimum(-std::numeric_limits<double>::max());
			sb->setMaximum(+std::numeric_limits<double>::max());
			sb->setDecimals(2);
			sb->setValue(matrix.v[y*3+x]);

			gridLayout->addWidget(sb, y, x, 1, 1);

			_coeffSpinBox.append(sb);
		}
	}

	_normalizeCheckBox->setText(tr("Normalize coefficients"));
	_normalizeCheckBox->setChecked(true);

	auto buttonBox = new QDialogButtonBox(this);
	buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
	buttonBox->setCenterButtons(false);
	connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
	connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

	auto verticalLayout = new QVBoxLayout(this);
	verticalLayout->addLayout(gridLayout);
	verticalLayout->addWidget(_normalizeCheckBox);
	verticalLayout->addWidget(buttonBox);

	_coeffSpinBox.first()->setFocus();
}

Matrix3x3 PropertyMatrixDialog::coefficients() const
{
	Matrix3x3 m;
	int i = 0;
	double sum = 0.0;

	if(_normalizeCheckBox->isChecked())
	{
		foreach(const QDoubleSpinBox* sb, _coeffSpinBox)
			sum += sb->value();
	}

	if(fabs(sum) < std::numeric_limits<double>::epsilon())
		// High pass filter
		sum = 1.0;
	else
		sum = 1.0 / sum;

	foreach(const QDoubleSpinBox* sb, _coeffSpinBox)
		m.v[i++] = sb->value() * sum;

	return m;
}

Matrix3x3 PropertyMatrixDialog::getCoefficients(QWidget* parent, const Matrix3x3& init, int* ok)
{
	PropertyMatrixDialog dlg(init, parent);
	const int res = dlg.exec();
	if(ok)
		*ok = res;
		
	return res == QDialog::Accepted ? dlg.coefficients() : init;
}
