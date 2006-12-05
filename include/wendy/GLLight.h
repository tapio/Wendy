///////////////////////////////////////////////////////////////////////
// Wendy OpenGL library
// Copyright (c) 2005 Camilla Berglund <elmindreda@elmindreda.org>
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
#ifndef WENDY_GLLIGHT_H
#define WENDY_GLLIGHT_H
///////////////////////////////////////////////////////////////////////

#include <vector>

///////////////////////////////////////////////////////////////////////

namespace wendy
{
  namespace GL
  {
  
///////////////////////////////////////////////////////////////////////

using namespace moira;

///////////////////////////////////////////////////////////////////////

/*! @brief Camera space light.
 *
 *  This works both with fixed pipeline and GLSL programs.
 */
class Light : public RefObject<Light>, public Managed<Light>
{
public:
  enum Type
  {
    DIRECTIONAL,
    POINT,
    SPOT,
  };
  Light(const String& name = "");
  ~Light(void);
  bool operator < (const Light& other) const;
  bool isBounded(void) const;
  Type getType(void) const;
  void setType(Type type);
  const ColorRGB& getAmbience(void) const;
  void setAmbience(const ColorRGB& newAmbience);
  const ColorRGB& getIntensity(void) const;
  void setIntensity(const ColorRGB& newIntensity);
  const Vector3& getPosition(void) const;
  void setPosition(const Vector3& newPosition);
  const Vector3& getDirection(void) const;
  void setDirection(const Vector3& newDirection);
  float getRadius(void) const;
  void setRadius(float newRadius);
  float getCutoffAngle(void) const;
  void setCutoffAngle(float newAngle);
  const Sphere& getBounds(void) const;
  char getTypeCharacter(void) const;
  void setDefaults(void);
  static void applyFixedState(void);
  static void applyShaderState(void);
  static unsigned int getSlotCount(void);
private:
  Light(const Light& source);
  Light& operator = (const Light& source);
  static void onContextDestroy(void);
  Type type;
  ColorRGB ambience;
  ColorRGB intensity;
  Vector3 position;
  Vector3 direction;
  float radius;
  float cutoff;
  mutable Sphere bounds;
  static unsigned int slotCount;
};

///////////////////////////////////////////////////////////////////////

typedef Ref<Light> LightRef;

///////////////////////////////////////////////////////////////////////

class LightState
{
public:
  LightState(void);
  void apply(void) const;
  void attachLight(Light& light);
  void detachLight(Light& light);
  void detachLights(void);
  unsigned int getLightCount(void) const;
  Light& getLight(unsigned int index) const;
  void getPermutationName(String& name) const;
  const String& getPermutationText(void) const;
  static const LightState& getCurrent(void);
private:
  void generatePermutation(String& text) const;
  static void onContextDestroy(void);
  typedef std::vector<LightRef> List;
  typedef std::map<String, String> PermutationMap;
  List lights;
  static LightState current;
  static PermutationMap permutations;
};

///////////////////////////////////////////////////////////////////////

  } /*namespace GL*/
} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
#endif /*WENDY_GLLIGHT_H*/
///////////////////////////////////////////////////////////////////////
