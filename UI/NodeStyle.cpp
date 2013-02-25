#include "NodeStyle.h"

#if defined(Q_OS_WIN)
static QString FontName = "Segoe UI";
#else
static QString FontName = "DejaVu Sans";
#endif

// Scene NodeStyle
QColor NodeStyle::SceneBackground = QColor(64, 64, 64);

// Link NodeStyle
QPen NodeStyle::LinkPen = QPen(QColor(222, 222, 222), 2.0, Qt::SolidLine);

// TemporaryLink NodeStyle
QPen NodeStyle::TemporaryLinkPen = QPen(QColor(244, 98, 0), 2.0, Qt::SolidLine);

// Node NodeStyle
QFont NodeStyle::NodeTitleFont = QFont(FontName, 11, QFont::Bold);
QBrush NodeStyle::NodeTitleFontBrush = QBrush(QColor(180, 180, 180));
QBrush NodeStyle::NodeTitleBrush = QBrush(QColor(62, 62, 62));
QBrush NodeStyle::NodeBrush = QBrush(QColor(37, 37, 37));
QPen NodeStyle::NodeBorderPen = QPen(QColor(88, 88, 88), 1.0, Qt::SolidLine);
QPen NodeStyle::NodeBorderSelectedPen = QPen(QColor(128, 128, 128), 2.5, Qt::SolidLine);
QPen NodeStyle::NodeBorderPreviewSelectedPen = QPen(QColor(119, 189, 255), 2.5, Qt::SolidLine);
int NodeStyle::NodeRoundArc = 15;
float NodeStyle::NodeTitleSize = 5.0;
float NodeStyle::NodeTitleHorizontalMargin = 20.0;
float NodeStyle::NodeSocketHorizontalMargin = -7.5;
float NodeStyle::NodeSocketVerticalMargin = 8.0;
float NodeStyle::NodeSocketsMargin = 20.0;

// Socket NodeStyle
QColor NodeStyle::SocketGradientStart = QColor(93, 93, 93);
QColor NodeStyle::SocketGradientStop = QColor(57, 57, 57);
QPen NodeStyle::SocketPen = QPen(QColor(180, 180, 180), 1.0, Qt::SolidLine);
QRect NodeStyle::SocketSize = QRect(0, 0, 15, 15);

QBrush NodeStyle::SocketTitleBrush = QBrush(QColor(180, 180, 180));
QBrush NodeStyle::SocketTitleInactiveBrush = QBrush(QColor(119, 119, 119));

QFont NodeStyle::SocketFont = QFont(FontName, 9);

int NodeStyle::ZValueNode = 100;
int NodeStyle::ZValueNodeHovered = 200;
int NodeStyle::ZValueLink = 0;
int NodeStyle::ZValueTemporaryLink = 400;
