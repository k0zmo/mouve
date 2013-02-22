#pragma once

#include "Prerequisites.h"

#include <QStyledItemDelegate>

class QSignalMapper;

class PropertyDelegate : public QStyledItemDelegate
{
	Q_OBJECT
public:
	explicit PropertyDelegate(QObject* parent = nullptr);
	~PropertyDelegate() override;

	void setImmediateUpdate(bool immediate);

	QWidget* createEditor(QWidget* parent,
		const QStyleOptionViewItem& option,
		const QModelIndex& index) const override;

	void setEditorData(QWidget* editor,
		const QModelIndex& index) const override;

	void setModelData(QWidget* editor, QAbstractItemModel* model,
		const QModelIndex& index) const override;

	void paint(QPainter* painter,
		const QStyleOptionViewItem& option,
		const QModelIndex& index) const override;

	QSize sizeHint(const QStyleOptionViewItem& option,
		const QModelIndex& index) const override;

protected:
	bool eventFilter(QObject* obj, QEvent* event) override;

private:
	QSignalMapper* _mapper;
	bool _immediateUpdate;
};

inline void PropertyDelegate::setImmediateUpdate(bool immediate)
{ _immediateUpdate = immediate; }