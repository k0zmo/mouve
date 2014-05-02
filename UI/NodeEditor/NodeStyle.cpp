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

#include "NodeStyle.h"

#if defined(Q_OS_WIN)
static QString FontName = "Segoe UI";
#else
static QString FontName = "DejaVu Sans";
#endif

// Scene NodeStyle
QColor NodeStyle::SceneBackground = QColor(104, 104, 104);
QColor NodeStyle::SceneBlockedBackground = QColor(34, 34, 34);

// Link NodeStyle
QPen NodeStyle::LinkPen = QPen(QColor(222, 222, 222), 2.0, Qt::SolidLine);

// TemporaryLink NodeStyle
QPen NodeStyle::TemporaryLinkPen = QPen(QColor(244, 98, 0), 2.0, Qt::SolidLine);

// Node NodeStyle
QFont NodeStyle::NodeTitleFont = QFont(FontName, 11, QFont::Bold);
QColor NodeStyle::NodeTitleFontColor = QColor(180, 180, 180);
QBrush NodeStyle::NodeTitleBrush = QBrush(QColor(62, 62, 62));
QFont NodeStyle::NodeTypeNameFont = QFont(FontName, 8, QFont::Normal, true);
QColor NodeStyle::NodeTypeNameFontColor = QColor(180, 180, 180);
QBrush NodeStyle::NodeBrush = QBrush(QColor(37, 37, 37));
QPen NodeStyle::NodeBorderPen = QPen(QColor(88, 88, 88), 1.0, Qt::SolidLine);
QPen NodeStyle::NodeBorderSelectedPen = QPen(QColor(128, 128, 128), 2.5, Qt::SolidLine);
QPen NodeStyle::NodeBorderPreviewSelectedPen = QPen(QColor(119, 189, 255), 2.5, Qt::SolidLine);
QPen NodeStyle::NodeBorderDisabledPen = QPen(QColor(136, 11, 17), 2.5, Qt::SolidLine);
int NodeStyle::NodeRoundArc = 15;
float NodeStyle::NodeTitleSize = 5.0;
float NodeStyle::NodeTitleHorizontalMargin = 20.0;
float NodeStyle::NodeTypeNameHorizontalMargin = 8.0;
float NodeStyle::NodeSocketHorizontalMargin = -7.5;
float NodeStyle::NodeSocketVerticalMargin = 8.0;
float NodeStyle::NodeSocketsMargin = 20.0;

QFont NodeStyle::NodeTimeInfoFont = QFont("Arial", 10);
QBrush NodeStyle::NodeTimeInfoFontBrush = QBrush(QColor(204, 204, 204));

// Socket NodeStyle
QBrush NodeStyle::SocketTitleBrush = QBrush(QColor(180, 180, 180));
QBrush NodeStyle::SocketTitleInactiveBrush = QBrush(QColor(119, 119, 119));
QFont NodeStyle::SocketFont = QFont(FontName, 9);

// Connector NodeStyle
QColor NodeStyle::SocketGradientStart1 = QColor(193, 171, 60);
QColor NodeStyle::SocketGradientStop1 = QColor(99, 88, 34);
QColor NodeStyle::SocketGradientStart2 = QColor(141, 139, 169);
QColor NodeStyle::SocketGradientStop2 = QColor(61, 61, 108);
QColor NodeStyle::SocketGradientStart3 = QColor(93, 93, 93);
QColor NodeStyle::SocketGradientStop3 = QColor(57, 57, 57);
QColor NodeStyle::SocketGradientStart4 = QColor(82, 165, 98);
QColor NodeStyle::SocketGradientStop4 = QColor(40, 81, 47);
QColor NodeStyle::SocketGradientStart5 = QColor(142, 84, 123);
QColor NodeStyle::SocketGradientStop5 = QColor(84, 50, 73);
QColor NodeStyle::SocketGradientStart6 = QColor(19, 126, 130);
QColor NodeStyle::SocketGradientStop6 = QColor(8, 54, 56);

QFont NodeStyle::SocketAnnotationFont = QFont("Segoe UI", 8, QFont::Light);
QBrush NodeStyle::SocketAnnotationBrush = QBrush(QColor(244, 244, 224));

QPen NodeStyle::SocketPen = QPen(QColor(180, 180, 180), 1.0, Qt::SolidLine);
QRect NodeStyle::SocketSize = QRect(0, 0, 15, 15);
float NodeStyle::NodeSocketPenWidth = 1.0f;
float NodeStyle::NodeSocketPenWidthHovered = 2.0f;


int NodeStyle::ZValueNode = 100;
int NodeStyle::ZValueNodeHovered = 200;
int NodeStyle::ZValueLink = 0;
int NodeStyle::ZValueTemporaryLink = 400;
