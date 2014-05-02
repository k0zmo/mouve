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

#include "../Prerequisites.h"

#include <QColor>
#include <QPen>
#include <QBrush>
#include <QFont>

struct NodeStyle
{
    // Scene NodeStyle
    static QColor SceneBackground;
    static QColor SceneBlockedBackground;

    // Link NodeStyle
    static QPen LinkPen;

    // TemporaryLink NodeStyle
    static QPen TemporaryLinkPen;

    // Node NodeStyle
    static QFont NodeTitleFont;
    static QColor NodeTitleFontColor;
    static QBrush NodeTitleBrush;
    static QFont NodeTypeNameFont;
    static QColor NodeTypeNameFontColor;
    static QBrush NodeBrush;
    static QPen NodeBorderPen;
    static QPen NodeBorderSelectedPen;
    static QPen NodeBorderPreviewSelectedPen;
    static QPen NodeBorderDisabledPen;
    static int NodeRoundArc;
    static float NodeTitleSize;
    static float NodeTitleHorizontalMargin;
    static float NodeTypeNameHorizontalMargin;
    static float NodeSocketHorizontalMargin;
    static float NodeSocketVerticalMargin;
    static float NodeSocketsMargin;

    static QFont NodeTimeInfoFont;
    static QBrush NodeTimeInfoFontBrush;

    // Socket NodeStyle
    static QBrush SocketTitleBrush;
    static QBrush SocketTitleInactiveBrush;
    static QFont SocketFont;
    
    // Connector NodeStyle
    static QColor SocketGradientStart1;
    static QColor SocketGradientStop1;
    static QColor SocketGradientStart2;
    static QColor SocketGradientStop2;
    static QColor SocketGradientStart3;
    static QColor SocketGradientStop3;
    static QColor SocketGradientStart4;
    static QColor SocketGradientStop4;
    static QColor SocketGradientStart5;
    static QColor SocketGradientStop5;
    static QColor SocketGradientStart6;
    static QColor SocketGradientStop6;

    static QFont SocketAnnotationFont;
    static QBrush SocketAnnotationBrush;

    static QPen SocketPen;
    static QRect SocketSize;
    static float NodeSocketPenWidth;
    static float NodeSocketPenWidthHovered;

    static int ZValueNode;
    static int ZValueNodeHovered;
    static int ZValueLink;
    static int ZValueTemporaryLink;
};
