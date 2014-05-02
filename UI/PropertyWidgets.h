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

#pragma once

#include "Prerequisites.h"

#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QLineEdit>
#include <QPushButton>
#include <QDialog>

// Required for Matrix3x3
#include "Logic/NodeType.h"

class QToolButton;
class QMouseEvent;
class QFileSystemModel;

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

inline bool PropertyCheckBox::isChecked() const
{ return _checkBox->isChecked(); }

inline void PropertyCheckBox::setChecked(bool c)
{ _checkBox->setChecked(c); }

inline void PropertyCheckBox::blockCheckBoxSignals(bool block)
{ _checkBox->blockSignals(block); }


class FileRequester : public QWidget
{
    Q_OBJECT
public :
    explicit FileRequester(QWidget* parent = nullptr);

    QString text() const;
    QString filter() const;

    void blockLineEditSignals(bool block);

public slots:
    void openFileDialog();
    void setText(const QString& text);
    void setFilter(const QString& filter);
    void setMode(bool save);

private slots:
    void _q_textChangedPrivate(const QString& text);

signals:
    void commitData();

private:
    QLineEdit* _fileNameLineEdit;
    QToolButton* _openButton;
    QString _filter;
    QFileSystemModel* _fsModel;
    bool _save;
};

class PropertyMatrixButton : public QPushButton
{
    Q_OBJECT
public:
    explicit PropertyMatrixButton(QWidget* parent = nullptr);

    void setMatrix(const Matrix3x3& mat);
    Matrix3x3 matrix() const;

signals:
    void commitData();

private:
    Matrix3x3 _matrix;
};

class PropertyMatrixDialog : public QDialog
{
public:
    explicit PropertyMatrixDialog(const Matrix3x3& matrix, QWidget* parent = nullptr);
    Matrix3x3 coefficients() const;

private:
    QCheckBox* _normalizeCheckBox;
    QList<QDoubleSpinBox*> _coeffSpinBox;
};