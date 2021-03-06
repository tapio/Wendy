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

#include <wendy/Config.hpp>

#include <wendy/Core.hpp>
#include <wendy/Timer.hpp>
#include <wendy/Profile.hpp>
#include <wendy/Transform.hpp>
#include <wendy/Primitive.hpp>
#include <wendy/Frustum.hpp>
#include <wendy/Camera.hpp>

#include <wendy/RenderPool.hpp>
#include <wendy/RenderState.hpp>
#include <wendy/RenderMaterial.hpp>
#include <wendy/RenderScene.hpp>
#include <wendy/RenderSprite.hpp>

#include <wendy/Forward.hpp>

///////////////////////////////////////////////////////////////////////

namespace wendy
{
  namespace forward
  {

///////////////////////////////////////////////////////////////////////

Config::Config(render::VertexPool& initPool):
  pool(&initPool)
{
}

///////////////////////////////////////////////////////////////////////

void Renderer::render(const render::Scene& scene, const Camera& camera)
{
  ProfileNodeCall call("forward::Renderer::render");

  context().setCurrentSharedProgramState(m_state);

  const Recti& viewportArea = context().viewportArea();
  m_state->setViewportSize(float(viewportArea.size.x),
                           float(viewportArea.size.y));

  m_state->setProjectionMatrix(camera.projectionMatrix());
  m_state->setViewMatrix(camera.viewTransform());

  if (camera.isPerspective())
  {
    m_state->setCameraProperties(camera.transform().position,
                                 camera.FOV(),
                                 camera.aspectRatio(),
                                 camera.nearZ(),
                                 camera.farZ());
  }

  renderOperations(scene.opaqueQueue());
  renderOperations(scene.blendedQueue());

  context().setCurrentSharedProgramState(nullptr);
}

Ref<Renderer> Renderer::create(const Config& config)
{
  if (!config.pool)
  {
    logError("Cannot create forward renderer without a geometry pool");
    return nullptr;
  }

  Ptr<Renderer> renderer(new Renderer(*config.pool));
  if (!renderer->init(config))
    return nullptr;

  return renderer.detachObject();
}

Renderer::Renderer(render::VertexPool& pool):
  render::System(pool, render::System::FORWARD)
{
}

bool Renderer::init(const Config& config)
{
  if (config.state)
    m_state = config.state;
  else
    m_state = new SharedProgramState();

  m_state->reserveSupported(context());
  return true;
}

void Renderer::renderOperations(const render::Queue& queue)
{
  const render::OperationList& operations = queue.operations();

  for (auto k : queue.keys())
  {
    const render::SortKey key(k);
    const render::Operation& op = operations[key.index];

    m_state->setModelMatrix(op.transform);
    op.state->apply();

    context().render(op.range);
  }
}

///////////////////////////////////////////////////////////////////////

  } /*namespace forward*/
} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
