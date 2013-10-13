///////////////////////////////////////////////////////////////////////
// Wendy scene graph
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

#include <wendy/Config.hpp>

#include <wendy/Core.hpp>
#include <wendy/Timer.hpp>
#include <wendy/Profile.hpp>
#include <wendy/Transform.hpp>
#include <wendy/AABB.hpp>
#include <wendy/Plane.hpp>
#include <wendy/Frustum.hpp>
#include <wendy/Camera.hpp>

#include <wendy/GLTexture.hpp>
#include <wendy/GLBuffer.hpp>
#include <wendy/GLProgram.hpp>
#include <wendy/GLContext.hpp>

#include <wendy/RenderPool.hpp>
#include <wendy/RenderState.hpp>
#include <wendy/RenderMaterial.hpp>
#include <wendy/RenderScene.hpp>
#include <wendy/RenderModel.hpp>

#include <wendy/SceneGraph.hpp>

#include <algorithm>

///////////////////////////////////////////////////////////////////////

namespace wendy
{
  namespace scene
  {

///////////////////////////////////////////////////////////////////////

Node::Node(bool initNeedsUpdate):
  m_needsUpdate(initNeedsUpdate),
  m_parent(nullptr),
  m_graph(nullptr),
  m_dirtyWorld(false),
  m_dirtyBounds(false)
{
}

Node::~Node()
{
  destroyChildren();
  removeFromParent();
}

bool Node::addChild(Node& child)
{
  if (isChildOf(child))
    return false;

  child.removeFromParent();

  m_children.push_back(&child);
  child.m_parent = this;
  child.setGraph(m_graph);
  child.invalidateWorldTransform();

  invalidateBounds();
  return true;
}

void Node::removeFromParent()
{
  if (m_parent || m_graph)
  {
    if (m_parent)
    {
      List& siblings = m_parent->m_children;
      siblings.erase(std::find(siblings.begin(), siblings.end(), this));

      m_parent->invalidateBounds();
      m_parent = nullptr;

      invalidateWorldTransform();
    }
    else
    {
      List& roots = m_graph->m_roots;
      roots.erase(std::find(roots.begin(), roots.end(), this));
    }

    setGraph(nullptr);
  }
}

void Node::destroyChildren()
{
  while (!m_children.empty())
    delete m_children.back();
}

bool Node::isChildOf(const Node& node) const
{
  if (m_parent)
  {
    if (m_parent == &node)
      return true;

    return m_parent->isChildOf(node);
  }

  return false;
}

void Node::setLocalTransform(const Transform3& newTransform)
{
  m_local = newTransform;

  if (m_parent)
    m_parent->invalidateBounds();

  invalidateWorldTransform();
}

void Node::setLocalPosition(const vec3& newPosition)
{
  m_local.position = newPosition;

  if (m_parent)
    m_parent->invalidateBounds();

  invalidateWorldTransform();
}

void Node::setLocalRotation(const quat& newRotation)
{
  m_local.rotation = newRotation;

  if (m_parent)
    m_parent->invalidateBounds();

  invalidateWorldTransform();
}

void Node::setLocalScale(float newScale)
{
  m_local.scale = newScale;

  if (m_parent)
    m_parent->invalidateBounds();

  invalidateWorldTransform();
}

const Transform3& Node::worldTransform() const
{
  if (m_dirtyWorld)
  {
    if (m_parent)
      m_world = m_parent->worldTransform() * m_local;
    else
      m_world = m_local;

    m_dirtyWorld = false;
  }

  return m_world;
}

const Sphere& Node::localBounds() const
{
  return m_localBounds;
}

void Node::setLocalBounds(const Sphere& newBounds)
{
  m_localBounds = newBounds;
  invalidateBounds();
}

const Sphere& Node::totalBounds() const
{
  if (m_dirtyBounds)
  {
    m_totalBounds = m_localBounds;

    for (auto c : m_children)
    {
      Sphere childBounds = c->totalBounds();
      childBounds.transformBy(c->localTransform());
      m_totalBounds.envelop(childBounds);
    }

    m_dirtyBounds = false;
  }

  return m_totalBounds;
}

void Node::update()
{
  for (auto c : m_children)
    c->update();
}

void Node::enqueue(render::Scene& scene, const Camera& camera) const
{
  const Frustum& frustum = camera.frustum();

  for (auto c : m_children)
  {
    Sphere worldBounds = c->totalBounds();
    worldBounds.transformBy(c->worldTransform());

    if (frustum.intersects(worldBounds))
      c->enqueue(scene, camera);
  }
}

void Node::invalidateBounds()
{
  for (Node* node = this;  node;  node = node->parent())
    node->m_dirtyBounds = true;
}

void Node::invalidateWorldTransform()
{
  m_dirtyWorld = true;

  for (auto c : m_children)
    c->invalidateWorldTransform();
}

void Node::setGraph(Graph* newGraph)
{
  if (m_graph && m_needsUpdate)
  {
    List& updated = m_graph->m_updated;
    updated.erase(std::find(updated.begin(), updated.end(), this));
  }

  m_graph = newGraph;

  if (m_graph && m_needsUpdate)
  {
    List& updated = m_graph->m_updated;
    updated.push_back(this);
  }

  for (auto c : m_children)
    c->setGraph(m_graph);
}

///////////////////////////////////////////////////////////////////////

Graph::~Graph()
{
  destroyRootNodes();
}

void Graph::update()
{
  for (auto n : m_updated)
    n->update();
}

void Graph::enqueue(render::Scene& scene, const Camera& camera) const
{
  ProfileNodeCall call("scene::Graph::enqueue");

  const Frustum& frustum = camera.frustum();

  for (auto r : m_roots)
  {
    Sphere worldBounds = r->totalBounds();
    worldBounds.transformBy(r->worldTransform());

    if (frustum.intersects(worldBounds))
      r->enqueue(scene, camera);
  }
}

void Graph::query(const Sphere& sphere, Node::List& nodes) const
{
  for (auto r : m_roots)
  {
    Sphere worldBounds = r->totalBounds();
    worldBounds.transformBy(r->worldTransform());

    if (sphere.intersects(worldBounds))
      nodes.push_back(r);
  }
}

void Graph::query(const Frustum& frustum, Node::List& nodes) const
{
  for (auto r : m_roots)
  {
    Sphere worldBounds = r->totalBounds();
    worldBounds.transformBy(r->worldTransform());

    if (frustum.intersects(worldBounds))
      nodes.push_back(r);
  }
}

void Graph::addRootNode(Node& node)
{
  node.removeFromParent();
  m_roots.push_back(&node);
  node.setGraph(this);
}

void Graph::destroyRootNodes()
{
  while (!m_roots.empty())
    delete m_roots.back();
}

///////////////////////////////////////////////////////////////////////

LightNode::LightNode():
  Node(true)
{
}

render::Light* LightNode::light() const
{
  return m_light;
}

void LightNode::setLight(render::Light* newLight)
{
  m_light = newLight;

  if (m_light)
    setLocalBounds(Sphere(vec3(0.f), m_light->radius()));
  else
    setLocalBounds(Sphere());
}

void LightNode::enqueue(render::Scene& scene, const Camera& camera) const
{
  Node::enqueue(scene, camera);

  if (m_light)
    m_light->enqueue(scene, camera, worldTransform());
}

///////////////////////////////////////////////////////////////////////

ModelNode::ModelNode():
  m_shadowCaster(false)
{
}

bool ModelNode::isShadowCaster() const
{
  return m_shadowCaster;
}

void ModelNode::setCastsShadows(bool enabled)
{
  m_shadowCaster = enabled;
}

render::Model* ModelNode::model() const
{
  return m_model;
}

void ModelNode::setModel(render::Model* newModel)
{
  m_model = newModel;

  if (m_model)
    setLocalBounds(m_model->boundingSphere());
  else
    setLocalBounds(Sphere());
}

void ModelNode::enqueue(render::Scene& scene, const Camera& camera) const
{
  Node::enqueue(scene, camera);

  if (m_model)
  {
    if (scene.phase() == render::PHASE_SHADOWMAP && !m_shadowCaster)
      return;

    m_model->enqueue(scene, camera, worldTransform());
  }
}

///////////////////////////////////////////////////////////////////////

CameraNode::CameraNode():
  Node(true)
{
}

Camera* CameraNode::camera() const
{
  return m_camera;
}

void CameraNode::setCamera(Camera* newCamera)
{
  m_camera = newCamera;
}

void CameraNode::update()
{
  Node::update();

  if (!m_camera)
    return;

  m_camera->setTransform(worldTransform());
}

///////////////////////////////////////////////////////////////////////

  } /*namespace scene*/
} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
