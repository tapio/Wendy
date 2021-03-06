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

#include <wendy/Config.hpp>

#include <wendy/Core.hpp>
#include <wendy/Timer.hpp>
#include <wendy/Profile.hpp>

#include <wendy/UIDrawer.hpp>
#include <wendy/UILayer.hpp>
#include <wendy/UIWidget.hpp>

#include <algorithm>

///////////////////////////////////////////////////////////////////////

namespace wendy
{
  namespace UI
  {

///////////////////////////////////////////////////////////////////////

Layer::Layer(Window& window, UI::Drawer& drawer):
  m_window(window),
  m_drawer(drawer),
  m_dragging(false),
  m_activeWidget(nullptr),
  m_draggedWidget(nullptr),
  m_hoveredWidget(nullptr),
  m_captureWidget(nullptr),
  m_stack(nullptr)
{
  assert(&m_window);
  assert(&m_drawer);
}

Layer::~Layer()
{
  destroyRootWidgets();
}

void Layer::update()
{
}

void Layer::draw()
{
  ProfileNodeCall call("UI::Layer::draw");

  m_drawer.begin();

  for (auto r : m_roots)
  {
    if (r->isVisible())
      r->draw();
  }

  m_drawer.end();
}

void Layer::addRootWidget(Widget& root)
{
  assert(&(root.m_layer) == this);

  root.removeFromParent();
  m_roots.push_back(&root);
}

void Layer::destroyRootWidgets()
{
  while (!m_roots.empty())
    delete m_roots.back();
}

Widget* Layer::findWidgetByPoint(vec2 point)
{
  for (auto r : m_roots)
  {
    if (r->isVisible())
    {
      if (Widget* widget = r->findByPoint(point))
        return widget;
    }
  }

  return nullptr;
}

void Layer::captureCursor()
{
  if (!m_activeWidget)
    return;

  releaseCursor();
  cancelDragging();

  m_captureWidget = m_activeWidget;
  m_hoveredWidget = m_activeWidget;
  m_window.captureCursor();
}

void Layer::releaseCursor()
{
  if (m_captureWidget)
  {
    m_captureWidget = nullptr;
    m_window.releaseCursor();
    updateHoveredWidget();
  }
}

void Layer::cancelDragging()
{
  if (m_dragging && m_draggedWidget)
  {
    vec2 cursorPosition = vec2(m_window.cursorPosition());
    cursorPosition.y = m_window.height() - cursorPosition.y;

    m_draggedWidget->m_dragEndedSignal(*m_draggedWidget, cursorPosition);

    m_draggedWidget = nullptr;
    m_dragging = false;
  }
}

void Layer::invalidate()
{
  m_window.invalidate();
}

bool Layer::isOpaque() const
{
  return true;
}

bool Layer::hasCapturedCursor() const
{
  return m_captureWidget != nullptr;
}

void Layer::setActiveWidget(Widget* widget)
{
  if (m_activeWidget == widget)
    return;

  if (widget)
  {
    assert(&(widget->m_layer) == this);

    if (!widget->isVisible() || !widget->isEnabled())
      return;
  }

  if (m_captureWidget)
    releaseCursor();

  if (m_activeWidget)
    m_activeWidget->m_focusChangedSignal(*m_activeWidget, false);

  m_activeWidget = widget;

  if (m_activeWidget)
    m_activeWidget->m_focusChangedSignal(*m_activeWidget, true);

  invalidate();
}

SignalProxy1<void, Layer&> Layer::sizeChangedSignal()
{
  return m_sizeChangedSignal;
}

void Layer::updateHoveredWidget()
{
  if (m_captureWidget)
    return;

  vec2 cursorPosition = vec2(m_window.cursorPosition());
  cursorPosition.y = m_window.height() - cursorPosition.y;

  Widget* newWidget = findWidgetByPoint(cursorPosition);

  if (m_hoveredWidget == newWidget)
    return;

  Widget* ancestor = m_hoveredWidget;

  while (ancestor)
  {
    // Find the common ancestor (or NULL) and notify each non-common ancestor

    if (newWidget == ancestor)
      break;

    if (newWidget && newWidget->isChildOf(*ancestor))
      break;

    ancestor->m_cursorLeftSignal(*ancestor);
    ancestor = ancestor->parent();
  }

  m_hoveredWidget = newWidget;

  while (newWidget)
  {
    // Notify each widget up to but not including the common ancestor

    if (newWidget == ancestor)
      break;

    newWidget->m_cursorEnteredSignal(*newWidget);
    newWidget = newWidget->parent();
  }
}

void Layer::removedWidget(Widget& widget)
{
  if (m_activeWidget)
  {
    if (m_activeWidget == &widget || m_activeWidget->isChildOf(widget))
      setActiveWidget(widget.parent());
  }

  if (m_hoveredWidget)
  {
    if (m_hoveredWidget == &widget || m_hoveredWidget->isChildOf(widget))
      updateHoveredWidget();
  }

  if (m_captureWidget)
  {
    if (m_captureWidget == &widget || m_captureWidget->isChildOf(widget))
      releaseCursor();
  }

  if (m_dragging)
  {
    if (m_draggedWidget)
    {
      if (m_draggedWidget == &widget || m_draggedWidget->isChildOf(widget))
        cancelDragging();
    }
  }
}

void Layer::onWindowSize(uint width, uint height)
{
  m_sizeChangedSignal(*this);
}

void Layer::onKey(Key key, Action action, uint mods)
{
  if (m_activeWidget)
    m_activeWidget->m_keyPressedSignal(*m_activeWidget, key, action, mods);
}

void Layer::onCharacter(uint32 codepoint, uint mods)
{
  if (m_activeWidget)
    m_activeWidget->m_charInputSignal(*m_activeWidget, codepoint, mods);
}

void Layer::onCursorPos(vec2 position)
{
  updateHoveredWidget();

  position.y = m_window.height() - position.y;

  if (m_hoveredWidget)
    m_hoveredWidget->m_cursorMovedSignal(*m_hoveredWidget, position);

  if (m_draggedWidget)
  {
    if (m_dragging)
      m_draggedWidget->m_dragMovedSignal(*m_draggedWidget, position);
    else
    {
      // TODO: Add insensitivity radius.

      m_dragging = true;
      m_draggedWidget->m_dragBegunSignal(*m_draggedWidget, position);
    }
  }
}

void Layer::onMouseButton(MouseButton button, Action action, uint mods)
{
  vec2 cursorPosition = vec2(m_window.cursorPosition());
  cursorPosition.y = m_window.height() - cursorPosition.y;

  if (action == PRESSED)
  {
    Widget* clickedWidget = nullptr;

    if (m_captureWidget)
      clickedWidget = m_captureWidget;
    else
    {
      for (auto r : m_roots)
      {
        if (r->isVisible())
        {
          clickedWidget = r->findByPoint(cursorPosition);
          if (clickedWidget)
            break;
        }
      }
    }

    while (clickedWidget && !clickedWidget->isEnabled())
      clickedWidget = clickedWidget->parent();

    if (clickedWidget)
    {
      clickedWidget->activate();
      clickedWidget->m_buttonClickedSignal(*clickedWidget,
                                           cursorPosition,
                                           button,
                                           action,
                                           mods);

      if (!m_captureWidget && clickedWidget->isDraggable())
        m_draggedWidget = clickedWidget;
    }
  }
  else if (action == RELEASED)
  {
    if (m_draggedWidget)
    {
      if (m_dragging)
      {
        m_draggedWidget->m_dragEndedSignal(*m_draggedWidget, cursorPosition);
        m_dragging = false;
      }

      m_draggedWidget = nullptr;
    }

    if (m_activeWidget)
    {
      if (m_captureWidget || m_activeWidget->globalArea().contains(cursorPosition))
      {
        m_activeWidget->m_buttonClickedSignal(*m_activeWidget,
                                              cursorPosition,
                                              button,
                                              action,
                                              mods);
      }
    }
  }
}

void Layer::onScroll(vec2 offset)
{
  if (m_hoveredWidget)
    m_hoveredWidget->m_scrolledSignal(*m_hoveredWidget, offset);
}

void Layer::onFocus(bool activated)
{
  if (!activated)
  {
    cancelDragging();
    releaseCursor();
  }
}

///////////////////////////////////////////////////////////////////////

LayerStack::LayerStack(Window& window):
  m_window(window)
{
}

void LayerStack::update() const
{
  for (auto l : m_layers)
    l->update();
}

void LayerStack::draw() const
{
  size_t count = 0;

  while (count < m_layers.size())
  {
    count++;

    if (m_layers[m_layers.size() - count]->isOpaque())
      break;
  }

  while (count)
  {
    m_layers[m_layers.size() - count]->draw();
    count--;
  }
}

void LayerStack::push(Layer& layer)
{
  assert(layer.m_stack == nullptr);
  assert(&layer.m_window == &m_window);

  m_layers.push_back(&layer);
  layer.m_stack = this;
  m_window.setTarget(&layer);
}

void LayerStack::pop()
{
  if (!m_layers.empty())
  {
    m_window.setTarget(nullptr);
    m_layers.back()->m_stack = nullptr;
    m_layers.pop_back();
  }

  if (!m_layers.empty())
    m_window.setTarget(m_layers.back());
}

void LayerStack::empty()
{
  while (!m_layers.empty())
    pop();
}

bool LayerStack::isEmpty() const
{
  return m_layers.empty();
}

Layer* LayerStack::top() const
{
  if (m_layers.empty())
    return nullptr;

  return m_layers.back();
}

///////////////////////////////////////////////////////////////////////

  } /*namespace UI*/
} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
