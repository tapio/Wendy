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
#ifndef WENDY_UISLIDER_H
#define WENDY_UISLIDER_H
///////////////////////////////////////////////////////////////////////

namespace wendy
{
  namespace UI
  {
  
///////////////////////////////////////////////////////////////////////

using namespace moira;

///////////////////////////////////////////////////////////////////////

/*! @ingroup ui
 */
class Slider : public Widget
{
public:
  Slider(Orientation orientation = HORIZONTAL, const String& name = "");
  float getMinValue(void) const;
  float getMaxValue(void) const;
  void setValueRange(float newMinValue, float newMaxValue);
  float getValue(void) const;
  void setValue(float newValue);
  Orientation getOrientation(void) const;
  void setOrientation(Orientation newOrientation);
  SignalProxy2<void, Slider&, float> getChangeValueSignal(void);
protected:
  void render(void) const;
private:
  void onButtonClick(Widget& widget,
                     const Vector2& position,
		     unsigned int button,
		     bool clicked);
  void onKeyPress(Widget& widget, GL::Key key, bool pressed);
  void setValue(float newValue, bool notify);
  Signal2<void, Slider&, float> changeValueSignal;
  float minValue;
  float maxValue;
  float value;
  Orientation orientation;
};

///////////////////////////////////////////////////////////////////////

  } /*namespace UI*/
} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
#endif /*WENDY_UISLIDER_H*/
///////////////////////////////////////////////////////////////////////
