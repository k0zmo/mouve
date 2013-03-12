#include "PropertyDelegate.h"
#include "Property.h"

#include <QApplication>
#include <QSignalMapper>

PropertyDelegate::PropertyDelegate(QObject* parent)
	: QStyledItemDelegate(parent)
	, _mapper(new QSignalMapper(this))
	, _immediateUpdate(true)
{
	// not needed as base class already call it and we just override its eventFilter function
	//installEventFilter(this);

	auto signal = static_cast<void (QSignalMapper::*)(QWidget*)>(&QSignalMapper::mapped);
	connect(_mapper, signal, this, &QStyledItemDelegate::commitData); 
}

PropertyDelegate::~PropertyDelegate()
{
}

QWidget* PropertyDelegate::createEditor(QWidget* parent,
										const QStyleOptionViewItem& option,
										const QModelIndex& index) const
{
	if(index.isValid())
	{
		Property* item = static_cast<Property*>(index.internalPointer());

		if(item)
		{
			QWidget* editor = item->createEditor(parent);

			if(editor)
			{
				if(_immediateUpdate && 
					editor->metaObject()->indexOfSignal("commitData()") >= 0)
				{
					connect(editor, SIGNAL(commitData()),
						_mapper, SLOT(map()));
					_mapper->setMapping(editor, editor);
				}

				return editor;
			}
		}
	}

	return QStyledItemDelegate::createEditor(parent, option, index);
}

void PropertyDelegate::setEditorData(QWidget* editor,
									 const QModelIndex& index) const
{
	if(index.isValid())
	{
		Property* item = static_cast<Property*>(index.internalPointer());

		if(item)
		{
			const QVariant& data = index.model()->data(index, Qt::EditRole);
			if(item->setEditorData(editor, data))
				return;
		}
	}

	QStyledItemDelegate::setEditorData(editor, index);
}

void PropertyDelegate::setModelData(QWidget* editor,
									QAbstractItemModel* model,
									const QModelIndex& index) const
{
	if(index.isValid())
	{
		Property* item = static_cast<Property*>(index.internalPointer());

		if(item)
		{
			QVariant data = item->editorData(editor);
			if(data.isValid())
			{
				model->setData(index, data, Qt::EditRole);
				return;
			}
		}
	}

	QStyledItemDelegate::setModelData(editor, model, index);
}

void PropertyDelegate::paint(QPainter* painter,
							 const QStyleOptionViewItem& option,
							 const QModelIndex& index) const
{
	Q_ASSERT(index.isValid());

	Property* item = static_cast<Property*>(index.internalPointer());

	if(item)
	{
		if(item->paint(painter, option, index.data(Qt::DisplayRole)))
			return;
	}
	QStyledItemDelegate::paint(painter, option, index);
}

QSize PropertyDelegate::sizeHint(const QStyleOptionViewItem& option,
								 const QModelIndex& index) const
{
	return QStyledItemDelegate::sizeHint(option, index) + QSize(3, 4);
}

bool PropertyDelegate::eventFilter(QObject* obj, QEvent* event)
{
	if(_immediateUpdate && event->type() == QEvent::FocusOut)
	{
		if(obj->metaObject()->indexOfSignal("commitData()") >= 0)
		{
			QWidget *editor = qobject_cast<QWidget*>(obj);
			if(editor && (!editor->isActiveWindow() || QApplication::focusWidget() != editor))
			{
				// Did the FocusOut event came internally from the editor
				QWidget* w = QApplication::focusWidget();
				while (w)
				{
					if(w == editor)
					return false;
					w = w->parentWidget();
				}

				emit closeEditor(editor, NoHint);

				return true;
			}
		}
	}
	return QStyledItemDelegate::eventFilter(obj, event);
}
