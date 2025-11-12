/*==============================================================================

  Copyright (c) Kitware, Inc.

  See http://www.slicer.org/copyright/copyright.txt for details.

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

  This file was originally developed by Julien Finet, Kitware, Inc.
  and was partially funded by NIH grant 3P41RR013218-12S1

==============================================================================*/

// Qt includes
#include <QDebug>
#include <QLinearGradient>
#include <QMenuBar>
#include <QPainter>
#include <QPalette>
#include <QPushButton>
#include <QStyleFactory>
#include <QStyleOption>
#include <QToolBar>

// CTK includes
#include <ctkCollapsibleButton.h>

// Radiance includes
#include "qAppStyle.h"

// --------------------------------------------------------------------------
// qAppStyle methods

// --------------------------------------------------------------------------
qAppStyle::qAppStyle()
{
  // Slicer uses a QCleanlooksStyle as base style.
  this->setBaseStyle(new QProxyStyle(QStyleFactory::create("fusion")));
}

// --------------------------------------------------------------------------
qAppStyle::~qAppStyle()
{
}

//------------------------------------------------------------------------------
QPalette qAppStyle::standardPalette()const
{
  QPalette palette = this->Superclass::standardPalette();

  return palette;
}

//------------------------------------------------------------------------------
void qAppStyle::drawComplexControl(ComplexControl control,
                                   const QStyleOptionComplex* option,
                                   QPainter* painter,
                                   const QWidget* widget )const
{
  const_cast<QStyleOptionComplex*>(option)->palette =
    this->tweakWidgetPalette(option->palette, widget);
  this->Superclass::drawComplexControl(control, option, painter, widget);
}

//------------------------------------------------------------------------------
void qAppStyle::drawControl(ControlElement element,
                            const QStyleOption* option,
                            QPainter* painter,
                            const QWidget* widget )const
{
  const_cast<QStyleOption*>(option)->palette =
    this->tweakWidgetPalette(option->palette, widget);

  // For some reason the toolbar paint routine is not respecting the palette.
  // here we make sure the background is correctly drawn.
  if (element == QStyle::CE_ToolBar &&
      qobject_cast<const QToolBar*>(widget))
    {
    painter->fillRect(option->rect, option->palette.brush(QPalette::Window));
    }
  this->Superclass::drawControl(element, option, painter, widget);
}

//------------------------------------------------------------------------------
void qAppStyle::drawPrimitive(PrimitiveElement element,
                              const QStyleOption* option,
                              QPainter* painter,
                              const QWidget* widget )const
{
  const_cast<QStyleOption*>(option)->palette =
    this->tweakWidgetPalette(option->palette, widget);
  this->Superclass::drawPrimitive(element, option, painter, widget);
}

//------------------------------------------------------------------------------
QPalette qAppStyle::tweakWidgetPalette(QPalette widgetPalette,
                                       const QWidget* widget)const
{
  if (!widget)
    {
    return widgetPalette;
    }
  if (qobject_cast<const QMenuBar*>(widget))
    {
    QColor highlightColor = this->standardPalette().color(QPalette::Dark);
    //QBrush highlightBrush = this->standardPalette().brush(QPalette::Dark);
    QColor highlightTextColor =
      this->standardPalette().color(QPalette::Light);
    QBrush highlightTextBrush =
      this->standardPalette().brush(QPalette::Light);
    QColor darkColor = this->standardPalette().color(QPalette::Highlight);
    QColor lightColor =
      this->standardPalette().color(QPalette::HighlightedText);

    QLinearGradient hilightGradient(0., 0., 0., 1.);
    hilightGradient.setCoordinateMode(QGradient::ObjectBoundingMode);
    hilightGradient.setColorAt(0., highlightColor);
    hilightGradient.setColorAt(1., highlightColor.darker(120));
    QBrush highlightBrush(hilightGradient);

    widgetPalette.setColor(QPalette::Highlight, darkColor);
    widgetPalette.setColor(QPalette::HighlightedText, lightColor);

    widgetPalette.setColor(QPalette::Window, highlightColor);
    widgetPalette.setColor(QPalette::WindowText, highlightTextColor);
    widgetPalette.setColor(QPalette::Base, highlightColor);
    widgetPalette.setColor(QPalette::Text, highlightTextColor);
    widgetPalette.setColor(QPalette::Button, highlightColor);
    widgetPalette.setColor(QPalette::ButtonText, highlightTextColor);

    widgetPalette.setBrush(QPalette::Window, highlightBrush);
    widgetPalette.setBrush(QPalette::WindowText, highlightTextBrush);
    widgetPalette.setBrush(QPalette::Base, highlightBrush);
    widgetPalette.setBrush(QPalette::Text, highlightTextBrush);
    widgetPalette.setBrush(QPalette::Button, highlightBrush);
    widgetPalette.setBrush(QPalette::ButtonText, highlightTextBrush);
    }
/*
  QWidget* parentWidget = widget->parentWidget();
  QWidget* grandParentWidget = parentWidget? parentWidget->parentWidget() : 0;
  if (qobject_cast<const QToolBar*>(widget) ||
      qobject_cast<QToolBar*>(parentWidget) ||
      qobject_cast<QToolBar*>(grandParentWidget))
    {
    QColor windowColor = this->standardPalette().color(QPalette::Window);

    //QColor highlightColor = this->standardPalette().color(QPalette::Highlight);
    //QColor highlightTextColor =
    //  this->standardPalette().color(QPalette::HighlightedText);
    //QColor darkColor = this->standardPalette().color(QPalette::Dark);
    //QColor lightColor =
    //  this->standardPalette().color(QPalette::Light);
    QColor highlightColor = this->standardPalette().color(QPalette::Dark);
    //QBrush highlightBrush = this->standardPalette().brush(QPalette::Dark);
    QBrush highlightTextBrush =
      this->standardPalette().brush(QPalette::Light);
    QColor darkColor = this->standardPalette().color(QPalette::Highlight);
    QColor lightColor =
      this->standardPalette().color(QPalette::HighlightedText);

    QLinearGradient hilightGradient(0., 0., 0., 1.);
    hilightGradient.setCoordinateMode(QGradient::ObjectBoundingMode);

    hilightGradient.setColorAt(0., highlightColor);
    hilightGradient.setColorAt(1., highlightColor.darker(140));
    QBrush highlightBrush(hilightGradient);

    widgetPalette.setColor(QPalette::Highlight, darkColor);
    widgetPalette.setColor(QPalette::HighlightedText, lightColor);
    widgetPalette.setBrush(QPalette::Window, highlightBrush);
    widgetPalette.setBrush(QPalette::WindowText, highlightTextBrush);
    widgetPalette.setBrush(QPalette::Base, highlightBrush);
    widgetPalette.setBrush(QPalette::Text, highlightTextBrush);
    widgetPalette.setBrush(QPalette::Button, highlightBrush);
    widgetPalette.setBrush(QPalette::ButtonText, highlightTextBrush);
    }
*/
  return widgetPalette;
}

//------------------------------------------------------------------------------
void qAppStyle::polish(QWidget* widget)
{
  this->Superclass::polish(widget);
  ctkCollapsibleButton* collapsibleButton =
    qobject_cast<ctkCollapsibleButton*>(widget);
  if (collapsibleButton)
    {
    collapsibleButton->setFlat(true);
    collapsibleButton->setContentsFrameShadow(QFrame::Sunken);
    }
}
