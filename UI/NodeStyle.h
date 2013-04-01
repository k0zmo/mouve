#pragma once

#include "Prerequisites.h"

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
	static QBrush NodeTitleFontBrush;
	static QBrush NodeTitleBrush;
	static QFont NodeTypeNameFont;
	static QBrush NodeTypeNameFontBrush;
	static QBrush NodeBrush;
	static QPen NodeBorderPen;
	static QPen NodeBorderSelectedPen;
	static QPen NodeBorderPreviewSelectedPen;
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
