///////////////////////////////////////////////////////////////////////
// Wendy user interface library
// Copyright (c) 2006 Camilla Berglund <elmindreda@elmindreda.org>
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
#ifndef WENDY_UIWIDGET_HPP
#define WENDY_UIWIDGET_HPP
///////////////////////////////////////////////////////////////////////

namespace wendy
{
  namespace UI
  {

///////////////////////////////////////////////////////////////////////

class Layer;

///////////////////////////////////////////////////////////////////////

/*! @brief Base class for widgets.
 *  @ingroup ui
 *
 *  This is the base class for all %UI widgets.
 *
 *  @remarks Yes, it's big.
 */
class Widget : public Trackable
{
  friend class Layer;
public:
  /*! Constructor.
   */
  Widget(Layer& layer);
  /*! Destructor.
   */
  ~Widget();
  /*! Adds the specified widget as a child of this widget.
   *  @param[in] child The widget to add.
   *  @remarks The specified widget must not be a parent or ancestor of this
   *  widget.
   */
  void addChild(Widget& child);
  /*! Destroys all children, recursively, of this widget.
   */
  void destroyChildren();
  /*! Detaches this widget from its parent, if any.
   */
  void removeFromParent();
  /*! Searches for a widget at the specified point.
   *  @param[in] point The point at which to search.
   *  @return The widget at the specified point, or @c nullptr if no matching
   *  widget was found.
   *
   *  @remarks The point is in parent coordinates.  If this is a top-level
   *  widget, it is in global coordinates.
   */
  Widget* findByPoint(vec2 point);
  /*! Transforms the specified point from global into local (client
   *  area) coordinates.
   *  @param[in] globalPoint The global coordinate point to transform.
   *  @return The corresponding local coordinate point.
   */
  vec2 transformToLocal(vec2 globalPoint) const;
  /*! Transforms the specified point from local (client area) into
   *  global coordinates.
   *  @param[in] localPoint The local coordinate point to transform.
   *  @return The corresponding global coordinate point.
   */
  vec2 transformToGlobal(vec2 localPoint) const;
  /*! Makes this widget visible.
   */
  void show();
  /*! Makes this widget non-visible.
   */
  void hide();
  /*! Enables this widget, allowing it to become active and receive input
   *  events.
   */
  void enable();
  /*! Disables this widget, disallowing activation and reception of input
   *  events.
   */
  void disable();
  /*! Flags the layer this widget belongs to as needing to redraw itself.
   *
   *  @remarks This is a helper method for Layer::invalidate.
   */
  void invalidate();
  /*! Makes this the active widget.
   *
   *  @remarks This is ignored if the widget is hidden or disabled.
   */
  void activate();
  /*! Makes this the top-level widget.
   */
  void bringToFront();
  /*! Makes this the bottom-most widget.
   */
  void sendToBack();
  /*! Cancels any current drag operation.
   */
  void cancelDragging();
  /*! @return @c true if this widget is enabled, otherwise @c false.
   *
   *  @remarks This is a helper method for Layer::cancelDragging.
   */
  bool isEnabled() const;
  /*! @return @c true if this widget is visible, otherwise @c false.
   */
  bool isVisible() const;
  /*! @return @c true if this is the active widget, otherwise @c false.
   */
  bool isActive() const;
  /*! @return @c true if this widget is directly under the cursor, otherwise @c
   *  false.
   */
  bool isUnderCursor() const;
  /*! @return @c true if this widget supports dragging, otherwise @c false.
   */
  bool isDraggable() const { return m_draggable; }
  /*! @return @c true if this widget is currently the source of a dragging
   *  operation, otherwise @c false.
   */
  bool isBeingDragged() const;
  /*! @return @c true if the specified widget is a parent or ancestor of this
   *  widget.
   */
  bool isChildOf(const Widget& widget) const;
  /*! @return @c true if this widget has captured the cursor, otherwise @c
   *  false.
   */
  bool hasCapturedCursor() const;
  /*! @return The layer this widget belongs to.
   */
  Layer& layer() const { return m_layer; }
  /*! @return The parent of this widget, or @c nullptr if it has no parent.
   */
  Widget* parent() const { return m_parent; }
  /*! @return The child widgets of this widget.
   */
  const WidgetList& children() const { return m_children; }
  WidgetState state() const;
  /*! @return The width of this widget.
   */
  float width() const { return m_area.size.x; }
  /*! @return The height of this widget.
   */
  float height() const { return m_area.size.y; }
  /*! @return The area of this widget, in parent coordinates.
   */
  Rect area() const { return m_area; }
  /*! @return The area of this widget, in global coordinates.
   */
  Rect globalArea() const;
  /*! @return The position of this widget, in global coordinates.
   */
  vec2 globalPos() const;
  /*! Sets the area of this widget.
   *  @param[in] newArea The desired area, in parent coordinates.
   */
  void setArea(const Rect& newArea);
  /*! @return The size of this widget.
   */
  vec2 size() const { return m_area.size; }
  /*! Sets the size of this widget.
   *  @param[in] newSize The desired size, in parent coordinates.
   *
   *  @remarks This is a helper method for Widget::setArea.
   */
  void setSize(vec2 newSize);
  /*! Sets the position of this widget.
   *  @param[in] newPosition The desired position, in parent coordinates.
   *
   *  @remarks This is a helper method for Widget::setArea.
   */
  void setPosition(vec2 newPosition);
  /*! Sets whether this widget can be the source of drag operations.
   */
  void setDraggable(bool newState);
  SignalProxy1<void, Widget&> destroyedSignal();
  SignalProxy1<void, Widget&> areaChangedSignal();
  SignalProxy2<void, Widget&, bool> focusChangedSignal();
  SignalProxy4<void, Widget&, Key, Action, uint> keyPressedSignal();
  SignalProxy3<void, Widget&, uint32, uint> charInputSignal();
  SignalProxy2<void, Widget&, vec2> cursorMovedSignal();
  SignalProxy5<void, Widget&, vec2, MouseButton, Action, uint> buttonClickedSignal();
  SignalProxy2<void, Widget&, vec2> scrolledSignal();
  SignalProxy1<void, Widget&> cursorEnteredSignal();
  SignalProxy1<void, Widget&> cursorLeftSignal();
  SignalProxy2<void, Widget&, vec2> dragBegunSignal();
  SignalProxy2<void, Widget&, vec2> dragMovedSignal();
  SignalProxy2<void, Widget&, vec2> dragEndedSignal();
protected:
  /*! Calls Widget::draw for all children of this widget.
   */
  virtual void draw() const;
  virtual void addedChild(Widget& child);
  virtual void removedChild(Widget& child);
  virtual void addedToParent(Widget& parent);
  virtual void removedFromParent(Widget& parent);
private:
  Signal1<void, Widget&> m_destroyedSignal;
  Signal1<void, Widget&> m_areaChangedSignal;
  Signal2<void, Widget&, bool> m_focusChangedSignal;
  Signal4<void, Widget&, Key, Action, uint> m_keyPressedSignal;
  Signal3<void, Widget&, uint32, uint> m_charInputSignal;
  Signal2<void, Widget&, vec2> m_cursorMovedSignal;
  Signal5<void, Widget&, vec2, MouseButton, Action, uint> m_buttonClickedSignal;
  Signal2<void, Widget&, vec2> m_scrolledSignal;
  Signal1<void, Widget&> m_cursorEnteredSignal;
  Signal1<void, Widget&> m_cursorLeftSignal;
  Signal2<void, Widget&, vec2> m_dragBegunSignal;
  Signal2<void, Widget&, vec2> m_dragMovedSignal;
  Signal2<void, Widget&, vec2> m_dragEndedSignal;
  Layer& m_layer;
  Widget* m_parent;
  WidgetList m_children;
  bool m_enabled;
  bool m_visible;
  bool m_draggable;
  Rect m_area;
};

///////////////////////////////////////////////////////////////////////

  } /*namespace UI*/
} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
#endif /*WENDY_UIWIDGET_HPP*/
///////////////////////////////////////////////////////////////////////
