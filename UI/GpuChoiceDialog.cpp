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

#include "GpuChoiceDialog.h"
#include "Logic/OpenCL/IGpuNodeModule.h"

#include <QMessageBox>

static const int SPECIFIC_INDEX = 3;

GpuChoiceDialog::GpuChoiceDialog(const std::vector<GpuPlatform>& gpuPlatforms, 
                                 QWidget* parent)
    : QDialog(parent)
{
    setupUi(this);
    deviceTypeComboBox->addItems(QStringList() << "GPU" << 
        "CPU" << "Default" << "Specific");

    QList<QTreeWidgetItem*> treeItems;
    int platformID = 0;
    for(const auto& platform : gpuPlatforms)
    {
        QTreeWidgetItem* itemPlatform = new QTreeWidgetItem((QTreeWidgetItem*)nullptr,
            QStringList(QString::fromStdString(platform.name)));

        int deviceID = 0;
        for(const auto& device : platform.devices)
        {
            QTreeWidgetItem* itemDevice = new QTreeWidgetItem(itemPlatform, 
                QStringList(QString::fromStdString(device)));
            itemDevice->setData(0, Qt::UserRole, platformID);
            itemDevice->setData(0, Qt::UserRole + 1, deviceID);
            ++deviceID;
        }
        treeItems.append(itemPlatform);
        ++platformID;
    }

    devicesTreeWidget->setEnabled(false);
    devicesTreeWidget->setColumnCount(1);
    devicesTreeWidget->insertTopLevelItems(0, treeItems);
    devicesTreeWidget->headerItem()->setText(0, QStringLiteral("Available OpenCL platforms:"));
    devicesTreeWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    devicesTreeWidget->expandAll();

    connect(deviceTypeComboBox, SIGNAL(currentIndexChanged(int)),
        this, SLOT(changeDeviceTypeIndex(int)));

    /// TODO
    rememberCheckBox->setVisible(false);
}

EDeviceType GpuChoiceDialog::deviceType() const
{
    switch(deviceTypeComboBox->currentIndex())
    {
    case 0: return EDeviceType::Gpu;
    case 1: return EDeviceType::Cpu;
    case 2: return EDeviceType::Default;
    case 3: return EDeviceType::Specific;
    default: return  EDeviceType::None;
    }
}

int GpuChoiceDialog::platformID() const
{
    if(deviceTypeComboBox->currentIndex() == SPECIFIC_INDEX)
        return extractUserData(0);
    return 0;
}

int GpuChoiceDialog::deviceID() const
{
    if(deviceTypeComboBox->currentIndex() == SPECIFIC_INDEX)
        return extractUserData(1);
    return 0;
}

void GpuChoiceDialog::accept()
{
    if(deviceTypeComboBox->currentIndex() == SPECIFIC_INDEX)
    {
        // Need a bit more checking
        auto items = devicesTreeWidget->selectedItems();
        if(items.isEmpty())
        {
            QMessageBox::critical(this, "Gpu module", 
                "Please select appropriate device from the list of available OpenCL devices");
            return;
        }

        const auto& item = items[0];
        if(item->parent() == nullptr)
        {
            QMessageBox::critical(this, "Gpu module", 
                "Please select an OpenCL device not a platform");
            return;
        }
    }

    QDialog::accept();
}

void GpuChoiceDialog::changeDeviceTypeIndex(int index)
{
    // "Specific"
    if(index == SPECIFIC_INDEX)
    {
        devicesTreeWidget->setEnabled(true);
    }
    else
    {
        devicesTreeWidget->selectionModel()->clearSelection();
        devicesTreeWidget->setEnabled(false);
    }
}

int GpuChoiceDialog::extractUserData(int index) const
{
    auto items = devicesTreeWidget->selectedItems();
    if(!items.isEmpty())
    {
        const auto& item = items[0];
        if(item->parent())
        {
            return item->data(0, Qt::UserRole + index).toInt();
        }
    }
    return 0;
}
