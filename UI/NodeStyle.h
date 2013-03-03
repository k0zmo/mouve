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
	static QBrush NodeBrush;
	static QPen NodeBorderPen;
	static QPen NodeBorderSelectedPen;
	static QPen NodeBorderPreviewSelectedPen;
	static int NodeRoundArc;
	static float NodeTitleSize;
	static float NodeTitleHorizontalMargin;
	static float NodeSocketHorizontalMargin;
	static float NodeSocketVerticalMargin;
	static float NodeSocketsMargin;

	// Socket NodeStyle
	static QBrush SocketTitleBrush;
	static QBrush SocketTitleInactiveBrush;
	static QFont SocketFont;
	
	// Connector NodeStyle
	static QColor SocketGradientStart;
	static QColor SocketGradientStop;
	static QPen SocketPen;
	static QRect SocketSize;
	static float NodeSocketPenWidth;
	static float NodeSocketPenWidthHovered;

	static int ZValueNode;
	static int ZValueNodeHovered;
	static int ZValueLink;
	static int ZValueTemporaryLink;
};
