/*
 * Copyright (c) 2013-2014 Kajetan Swierk <k0zmo@outlook.com>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

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

    connect(_mapper, SIGNAL(mapped(QWidget*)), this, SIGNAL(commitData(QWidget*)));
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
                    connect(editor, SIGNAL(commitData()), _mapper, SLOT(map()));
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
