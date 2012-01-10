///////////////////////////////////////////////////////////////////////
// Wendy library
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

#include <wendy/Config.h>
#include <wendy/Core.h>
#include <wendy/Transform.h>
#include <wendy/Path.h>
#include <wendy/Resource.h>
#include <wendy/Mesh.h>

#include <wendy/Bullet.h>

#include <btBulletWorldImporter.h>

///////////////////////////////////////////////////////////////////////

namespace wendy
{
  namespace bullet
  {

///////////////////////////////////////////////////////////////////////

Transform3 convert(const btTransform& transform, float scale)
{
  Transform3 result;
  result.position = vec3(transform.getOrigin().x(),
                         transform.getOrigin().y(),
                         transform.getOrigin().z());

  btQuaternion rotation = transform.getRotation();
  result.rotation = quat(rotation.w(), rotation.x(), rotation.y(), rotation.z());

  result.scale = scale;
  return result;
}

btTransform convert(const Transform3& transform)
{
  btTransform result;
  result.setOrigin(btVector3(transform.position.x,
                             transform.position.y,
                             transform.position.z));

  result.setRotation(btQuaternion(transform.rotation.x,
                                  transform.rotation.y,
                                  transform.rotation.z,
                                  transform.rotation.w));

  return result;
}

vec3 convert(const btVector3& vector)
{
  return vec3(vector.x(), vector.y(), vector.z());
}

btVector3 convert(const vec3& vector)
{
  return btVector3(vector.x, vector.y, vector.z);
}

btTriangleMesh* convert(const Mesh& data, bool removeDuplicateVertices)
{
  btTriangleMesh* mesh;

  if (data.vertices.size() > 65536)
    mesh = new btTriangleMesh(true);
  else
    mesh = new btTriangleMesh(false);

  for (MeshSectionList::const_iterator s = data.sections.begin();
       s != data.sections.end();
       s++)
  {
    for (MeshTriangleList::const_iterator t = s->triangles.begin();
         t != s->triangles.end();
         t++)
    {
      mesh->addTriangle(convert(data.vertices[t->indices[0]].position),
                        convert(data.vertices[t->indices[1]].position),
                        convert(data.vertices[t->indices[2]].position),
                        removeDuplicateVertices);
    }
  }

  return mesh;
}

///////////////////////////////////////////////////////////////////////

BvhMeshShape::BvhMeshShape(const ResourceInfo& info):
  Resource(info)
{
}

///////////////////////////////////////////////////////////////////////

BvhMeshShapeReader::BvhMeshShapeReader(ResourceCache& cache):
  ResourceReader<BvhMeshShape>(cache)
{
}

Ref<BvhMeshShape> BvhMeshShapeReader::read(const String& name, const Path& path)
{
  btBulletWorldImporter importer;

  if (!importer.loadFile(path.asString().c_str()))
  {
    logError("Failed to load mesh shape file \'%s\'", name.c_str());
    return NULL;
  }

  if (!importer.getNumCollisionShapes())
  {
    logError("Mesh shape file \'%s\' does not contain any shapes", name.c_str());
    return NULL;
  }

  Ref<BvhMeshShape> result = new BvhMeshShape(ResourceInfo(cache, name, path));

  result->shape = (btBvhTriangleMeshShape*) importer.getCollisionShapeByIndex(0);
  if (!result->shape)
  {
    logError("No mesh shapes in \'%s\'", name.c_str());
    return NULL;
  }

  result->mesh = (btTriangleIndexVertexArray*) result->shape->getMeshInterface();
  result->bvh = result->shape->getOptimizedBvh();
  result->info = result->shape->getTriangleInfoMap();

  importer.detachBvhTriangleMeshShape(result->shape);
  importer.detachMeshInterface(result->mesh);
  importer.detachOptimizedBvh(result->bvh);
  importer.detachTriangleInfoMap(result->info);
  importer.deleteAllData();

  return result;
}

///////////////////////////////////////////////////////////////////////

bool BvhMeshShapeWriter::write(const Path& path, const btBvhTriangleMeshShape& shape)
{
  std::ofstream stream(path.asString().c_str());
  if (!stream.is_open())
  {
    logError("Failed to open mesh shape file \'%s\' for writing", path.asString().c_str());
    return false;
  }

  btDefaultSerializer serializer;

  serializer.startSerialization();
  shape.serializeSingleBvh(&serializer);
  serializer.finishSerialization();

  stream.write(reinterpret_cast<const char*>(serializer.getBufferPointer()),
               serializer.getCurrentBufferSize());

  if (stream.bad())
  {
    logError("Failed to write BVH data to \'%s\'", path.asString().c_str());
    return false;
  }

  stream.close();
  return true;
}

///////////////////////////////////////////////////////////////////////

AvatarSweepCallback::AvatarSweepCallback(const btCollisionObject* initSelf):
  self(initSelf)
{
}

bool AvatarSweepCallback::needsCollision(btBroadphaseProxy* proxy) const
{
  if (!ConvexResultCallback::needsCollision(proxy))
    return false;

  if (proxy->m_clientObject == self)
    return false;

  return true;
}

btScalar AvatarSweepCallback::addSingleResult(btCollisionWorld::LocalConvexResult& result,
                                              bool normalInWorldSpace)
{
  m_hitCollisionObject = result.m_hitCollisionObject;

  if (normalInWorldSpace)
    m_hitNormalWorld = result.m_hitNormalLocal;
  else
  {
    const btTransform& transform = m_hitCollisionObject->getWorldTransform();
    m_hitNormalWorld = transform.getBasis() * result.m_hitNormalLocal;
  }

  return m_closestHitFraction = result.m_hitFraction;
}

///////////////////////////////////////////////////////////////////////

  } /*namespace bullet*/
} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
