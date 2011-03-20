///////////////////////////////////////////////////////////////////////
// Wendy forward renderer
// Copyright (c) 2011 Camilla Berglund <elmindreda@elmindreda.org>
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
#ifndef WENDY_FORWARD_H
#define WENDY_FORWARD_H
///////////////////////////////////////////////////////////////////////

#include <wendy/RenderScene.h>
#include <wendy/RenderPool.h>

///////////////////////////////////////////////////////////////////////

namespace wendy
{
  namespace forward
  {

///////////////////////////////////////////////////////////////////////

class Config
{
};

///////////////////////////////////////////////////////////////////////

class Renderer
{
public:
  void render(const render::Scene& scene, const render::Camera& camera);
  static Renderer* create(render::GeometryPool& pool, const Config& config);
private:
  Renderer(render::GeometryPool& pool);
  bool init(const Config& config);
  void renderOperations(const render::Queue& queue);
  render::GeometryPool& pool;
  Config config;
  Ref<GL::SharedProgramState> state;
};

///////////////////////////////////////////////////////////////////////

  } /*namespace forward*/
} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
#endif /*WENDY_FORWARD_H*/
///////////////////////////////////////////////////////////////////////
