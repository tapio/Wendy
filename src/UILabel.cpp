//////////////////////////////////////////////////////////////////////
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

#include <wendy/Config.hpp>

#include <wendy/UIDrawer.hpp>
#include <wendy/UILayer.hpp>
#include <wendy/UIWidget.hpp>
#include <wendy/UILabel.hpp>

#include <cstdlib>

///////////////////////////////////////////////////////////////////////

namespace wendy
{
  namespace UI
  {

///////////////////////////////////////////////////////////////////////

Label::Label(Layer& layer, const char* text):
  Widget(layer),
  m_text(text),
  m_textAlignment(LEFT_ALIGNED)
{
  Drawer& drawer = layer.drawer();

  const float em = drawer.currentEM();

  float textWidth;

  if (m_text.empty())
    textWidth = em * 3.f;
  else
    textWidth = drawer.currentFont().boundsOf(m_text.c_str()).size.x;

  setSize(vec2(em * 2.f + textWidth, em * 2.f));
}

const String& Label::text() const
{
  return m_text;
}

void Label::setText(const char* newText)
{
  m_text = newText;
  invalidate();
}

const Alignment& Label::textAlignment() const
{
  return m_textAlignment;
}

void Label::setTextAlignment(const Alignment& newAlignment)
{
  m_textAlignment = newAlignment;
  invalidate();
}

void Label::draw() const
{
  Drawer& drawer = layer().drawer();

  const Rect area = globalArea();
  if (drawer.pushClipArea(area))
  {
    drawer.drawText(area, m_text.c_str(), m_textAlignment, state());
    Widget::draw();
    drawer.popClipArea();
  }
}

///////////////////////////////////////////////////////////////////////

  } /*namespace UI*/
} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
