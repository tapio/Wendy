///////////////////////////////////////////////////////////////////////
// Wendy user interface library
// Copyright (c) 2009 Camilla Berglund <elmindreda@elmindreda.org>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any
// damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any
// purpose, including commercial applications, and to alter it and
// redistribute it freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you
//     must not claim that you wrote the original software. If you use
//     this software in a product, an acknowledgment in the product
//     documentation would be appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and
//     must not be misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source
//     distribution.
//
///////////////////////////////////////////////////////////////////////

#include <moira/Moira.h>

#include <wendy/Config.h>

#include <wendy/GLContext.h>
#include <wendy/GLTexture.h>
#include <wendy/GLVertex.h>
#include <wendy/GLBuffer.h>
#include <wendy/GLProgram.h>
#include <wendy/GLRender.h>
#include <wendy/GLState.h>

#include <wendy/RenderFont.h>

#include <wendy/Input.h>

#include <wendy/UIRender.h>
#include <wendy/UIDesktop.h>
#include <wendy/UIWidget.h>

///////////////////////////////////////////////////////////////////////

namespace wendy
{
  namespace UI
  {

///////////////////////////////////////////////////////////////////////

using namespace moira;

///////////////////////////////////////////////////////////////////////

Desktop::Desktop(input::Context& initContext, UI::Renderer& initRenderer):
  context(initContext),
  renderer(initRenderer),
  dragging(false),
  activeWidget(NULL),
  draggedWidget(NULL),
  hoveredWidget(NULL)
{
}

Desktop::~Desktop(void)
{
  destroyRootWidgets();

  if (context.getFocus() == this)
    context.setFocus(NULL);
}

void Desktop::drawRootWidgets(void)
{
  GL::Renderer::get()->setProjectionMatrix2D((float) context.getWidth(),
                                             (float) context.getHeight());

  for (WidgetList::iterator i = roots.begin();  i != roots.end();  i++)
  {
    if ((*i)->isVisible())
      (*i)->draw();
  }
}

void Desktop::destroyRootWidgets(void)
{
  while (!roots.empty())
    delete roots.back();
}

Widget* Desktop::findWidgetByPoint(const Vec2& point)
{
  for (WidgetList::reverse_iterator r = roots.rbegin();  r != roots.rend();  r++)
  {
    if ((*r)->isVisible())
    {
      Widget* widget = (*r)->findByPoint(point);
      if (widget)
	return widget;
    }
  }

  return NULL;
}

void Desktop::cancelDragging(void)
{
  if (dragging && draggedWidget)
  {
    Vec2i cursorPosition = context.getCursorPosition();
    cursorPosition.y = context.getHeight() - cursorPosition.y;

    const Vec2 scaledPosition(cursorPosition.x, cursorPosition.y);

    draggedWidget->dragEndedSignal.emit(*draggedWidget, scaledPosition);

    draggedWidget = NULL;
    dragging = false;
  }
}

Renderer& Desktop::getRenderer(void) const
{
  return renderer;
}

const WidgetList& Desktop::getRootWidgets(void) const
{
  return roots;
}

Widget* Desktop::getActiveWidget(void)
{
  return activeWidget;
}

Widget* Desktop::getDraggedWidget(void)
{
  return draggedWidget;
}

Widget* Desktop::getHoveredWidget(void)
{
  return hoveredWidget;
}

void Desktop::setActiveWidget(Widget* widget)
{
  if (activeWidget == widget)
    return;

  if (widget)
  {
    if (&(widget->desktop) != this)
      throw Exception("Cannot activate widget from other desktop");

    if (!widget->isVisible() || !widget->isEnabled())
      return;
  }

  if (activeWidget)
    activeWidget->focusChangedSignal.emit(*activeWidget, false);

  activeWidget = widget;

  if (activeWidget)
    activeWidget->focusChangedSignal.emit(*activeWidget, true);
}

void Desktop::updateHoveredWidget(void)
{
  Vec2i cursorPosition = context.getCursorPosition();
  cursorPosition.y = context.getHeight() - cursorPosition.y;

  const Vec2 scaledPosition(cursorPosition.x, cursorPosition.y);

  Widget* newWidget = findWidgetByPoint(scaledPosition);

  if (hoveredWidget == newWidget)
    return;

  // TODO: Notify parents up to common ancestor.

  if (hoveredWidget)
    hoveredWidget->cursorLeftSignal.emit(*hoveredWidget);

  hoveredWidget = newWidget;

  if (hoveredWidget)
    hoveredWidget->cursorEnteredSignal.emit(*hoveredWidget);
}

void Desktop::removedWidget(Widget& widget)
{
  if (activeWidget)
  {
    if (activeWidget == &widget || activeWidget->isChildOf(widget))
      setActiveWidget(widget.parent);
  }

  if (hoveredWidget)
  {
    if (hoveredWidget == &widget || hoveredWidget->isChildOf(widget))
      updateHoveredWidget();
  }

  if (dragging)
  {
    if (draggedWidget)
    {
      if (draggedWidget == &widget || draggedWidget->isChildOf(widget))
	cancelDragging();
    }
  }
}

void Desktop::onKeyPressed(input::Key key, bool pressed)
{
  if (activeWidget)
    activeWidget->keyPressedSignal.emit(*activeWidget, key, pressed);
}

void Desktop::onCharInput(wchar_t character)
{
  if (activeWidget)
    activeWidget->charInputSignal.emit(*activeWidget, character);
}

void Desktop::onCursorMoved(const Vec2i& position)
{
  updateHoveredWidget();

  Vec2i cursorPosition = context.getCursorPosition();
  cursorPosition.y = context.getHeight() - cursorPosition.y;

  const Vec2 scaledPosition(cursorPosition.x, cursorPosition.y);

  if (hoveredWidget)
    hoveredWidget->cursorMovedSignal.emit(*hoveredWidget, scaledPosition);

  if (draggedWidget)
  {
    if (dragging)
      draggedWidget->dragMovedSignal.emit(*draggedWidget, scaledPosition);
    else
    {
      // TODO: Add insensitivity radius.

      dragging = true;
      draggedWidget->dragBegunSignal.emit(*draggedWidget, scaledPosition);
    }
  }
}

void Desktop::onButtonClicked(input::Button button, bool clicked)
{
  Vec2i cursorPosition = context.getCursorPosition();
  cursorPosition.y = context.getHeight() - cursorPosition.y;

  const Vec2 scaledPosition(cursorPosition.x, cursorPosition.y);

  if (clicked)
  {
    Widget* clickedWidget = NULL;

    for (WidgetList::reverse_iterator r = roots.rbegin();  r != roots.rend();  r++)
    {
      if ((*r)->isVisible())
      {
	clickedWidget = (*r)->findByPoint(scaledPosition);
	if (clickedWidget)
	  break;
      }
    }

    while (clickedWidget && !clickedWidget->isEnabled())
      clickedWidget = clickedWidget->getParent();

    if (clickedWidget)
    {
      clickedWidget->activate();
      clickedWidget->buttonClickedSignal.emit(*clickedWidget,
                                              scaledPosition,
					      button,
					      clicked);

      // TODO: Allow dragging with any button.

      if (button == input::Button::LEFT && clickedWidget->isDraggable())
	draggedWidget = clickedWidget;
    }
  }
  else
  {
    if (draggedWidget)
    {
      if (dragging)
      {
	draggedWidget->dragEndedSignal.emit(*draggedWidget, scaledPosition);
	dragging = false;
      }

      draggedWidget = NULL;
    }

    if (activeWidget && activeWidget->getGlobalArea().contains(scaledPosition))
    {
      activeWidget->buttonClickedSignal.emit(*activeWidget,
					     scaledPosition,
					     button,
					     clicked);
    }
  }
}

void Desktop::onWheelTurned(int offset)
{
  if (hoveredWidget)
    hoveredWidget->wheelTurnedSignal.emit(*hoveredWidget, offset);
}

///////////////////////////////////////////////////////////////////////

  } /*namespace UI*/
} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
