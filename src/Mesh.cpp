///////////////////////////////////////////////////////////////////////
// Wendy core library
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

#include <wendy/Config.h>

#include <wendy/Core.h>
#include <wendy/AABB.h>
#include <wendy/Sphere.h>
#include <wendy/Path.h>
#include <wendy/Resource.h>
#include <wendy/Mesh.h>

#include <limits>
#include <cstdlib>
#include <fstream>
#include <cctype>

#include <glm/gtx/compatibility.hpp>
#include <glm/gtx/epsilon.hpp>

///////////////////////////////////////////////////////////////////////

namespace wendy
{

///////////////////////////////////////////////////////////////////////

namespace
{

struct Triplet
{
  unsigned int vertex;
  unsigned int normal;
  unsigned int texcoord;
};

struct Face
{
  Triplet p[3];
};

typedef std::vector<Face> FaceList;

struct FaceGroup
{
  FaceList faces;
  String name;
};

typedef std::vector<FaceGroup> FaceGroupList;

} /*namespace*/

///////////////////////////////////////////////////////////////////////

void MeshEdge::setIndices(unsigned int a, unsigned int b)
{
  indices[0] = a;
  indices[1] = b;
}

///////////////////////////////////////////////////////////////////////

void MeshTriangle::setIndices(unsigned int a, unsigned int b, unsigned int c)
{
  indices[0] = a;
  indices[1] = b;
  indices[2] = c;
}

void MeshTriangle::setEdges(unsigned int a, unsigned int b, unsigned int c)
{
  edges[0] = a;
  edges[1] = b;
  edges[2] = c;
}

///////////////////////////////////////////////////////////////////////

Mesh::Mesh(const ResourceInfo& info):
  Resource(info)
{
}

void Mesh::merge(const Mesh& other)
{
  vertices.insert(vertices.end(), other.vertices.begin(), other.vertices.end());

  for (GeometryList::const_iterator i = other.geometries.begin();  i != other.geometries.end();  i++)
  {
    MeshGeometry* geometry = findGeometry(i->shaderName.c_str());
    if (!geometry)
    {
      geometries.push_back(MeshGeometry());
      geometry = &(geometries.back());
      geometry->shaderName = i->shaderName;
    }

    geometry->triangles.insert(geometry->triangles.end(),
                               i->triangles.begin(),
                               i->triangles.end());
  }
}

void Mesh::collapseGeometries(const char* shaderName)
{
  geometries[0].shaderName = shaderName;

  for (size_t i = 1;  i < geometries.size();  i++)
  {
    geometries[0].triangles.insert(geometries[0].triangles.end(),
                                   geometries[i].triangles.begin(),
                                   geometries[i].triangles.end());
  }

  geometries.resize(1);
}

MeshGeometry* Mesh::findGeometry(const char* shaderName)
{
  for (size_t i = 0;  i < geometries.size();  i++)
  {
    if (geometries[i].shaderName == shaderName)
      return &(geometries[i]);
  }

  return NULL;
}

void Mesh::generateNormals(NormalType type)
{
  generateTriangleNormals();

  VertexMerger merger(vertices);

  if (type == SMOOTH_FACES)
    merger.setNormalMode(VertexMerger::MERGE_NORMALS);

  for (size_t i = 0;  i < geometries.size();  i++)
  {
    for (size_t j = 0;  j < geometries[i].triangles.size();  j++)
    {
      MeshTriangle& triangle = geometries[i].triangles[j];

      for (size_t k = 0;  k < 3;  k++)
      {
        triangle.indices[k] = merger.addAttributeLayer(triangle.indices[k],
                                                       triangle.normal,
                                                       vertices[triangle.indices[k]].texcoord);
      }
    }
  }

  merger.realizeVertices(vertices);
}

void Mesh::generateTriangleNormals()
{
  for (size_t i = 0;  i < geometries.size();  i++)
  {
    for (size_t j = 0;  j < geometries[i].triangles.size();  j++)
    {
      MeshTriangle& triangle = geometries[i].triangles[j];

      vec3 one = vertices[triangle.indices[1]].position - vertices[triangle.indices[0]].position;
      vec3 two = vertices[triangle.indices[2]].position - vertices[triangle.indices[0]].position;
      triangle.normal = normalize(cross(one, two));
    }
  }
}

void Mesh::generateTangents()
{
  // Inspired by article:
  // Lengyel, Eric. “Computing Tangent Space Basis Vectors for an Arbitrary Mesh”.
  // Terathon Software 3D Graphics Library, 2001.
  // http://www.terathon.com/code/tangent.html

  // TODO: Clear previous tangents

  std::vector<vec3> tan1(vertices.size(), vec3(0,0,0));
  std::vector<vec3> tan2(vertices.size(), vec3(0,0,0));

  for (size_t i = 0;  i < geometries.size();  i++)
  {
    for (size_t j = 0;  j < geometries[i].triangles.size();  j++)
    {
      // Convenience variables
      MeshTriangle& triangle = geometries[i].triangles[j];
      const MeshVertex& v0 = vertices[triangle.indices[0]];
      const MeshVertex& v1 = vertices[triangle.indices[1]];
      const MeshVertex& v2 = vertices[triangle.indices[2]];

      // Edge vectors
      const vec3 e1 = v1.position - v0.position;
      const vec3 e2 = v2.position - v0.position;

      // Texture coord magic
      const vec2 uv1 = v1.texcoord - v0.texcoord;
      const vec2 uv2 = v2.texcoord - v0.texcoord;
      const float r = 1.0 / (uv1.x * uv2.y - uv2.x * uv1.y);

      // Tangent components
      triangle.tangent.x = (uv2.y * e1.x - uv1.y * e2.x) * r;
      triangle.tangent.y = (uv2.y * e1.y - uv1.y * e2.y) * r;
      triangle.tangent.z = (uv2.y * e1.z - uv1.y * e2.z) * r;
      triangle.bitangent.x = (uv1.x * e2.x - uv2.x * e1.x) * r;
      triangle.bitangent.y = (uv1.x * e2.y - uv2.x * e1.y) * r;
      triangle.bitangent.z = (uv1.x * e2.z - uv2.x * e1.z) * r;

      // Apply results
      tan1[triangle.indices[0]] = triangle.tangent;
      tan1[triangle.indices[1]] = triangle.tangent;
      tan1[triangle.indices[2]] = triangle.tangent;
      tan2[triangle.indices[0]] = triangle.bitangent;
      tan2[triangle.indices[1]] = triangle.bitangent;
      tan2[triangle.indices[2]] = triangle.bitangent;
    }
  }

  for (size_t i = 0;  i < vertices.size();  i++)
  {
    const vec3& n = vertices[i].normal;
    // Gram-Schmidt orthogonalize
    vertices[i].tangent = normalize(tan1[i] - n * dot(n, tan1[i]));
    // Bitangent
    vertices[i].bitangent = cross(n, vertices[i].tangent);
    // Check for mirroring
    if (dot(cross(n, tan1[i]), tan2[i]) >= 0.0f) {
      vertices[i].tangent *= -1.0f;
      vertices[i].bitangent *= -1.0f;
    }
  }
}

void Mesh::generateEdges()
{
  for (size_t i = 0;  i < geometries.size();  i++)
  {
    for (size_t j = 0;  j < geometries[i].triangles.size();  j++)
    {
      for (size_t k = 0;  k < 3;  k++)
      {
        MeshEdge edge;
        edge.setIndices(geometries[i].triangles[j].indices[k],
                        geometries[i].triangles[j].indices[(k + 1) % 3]);

        size_t l;

        for (l = 0;  l < edges.size();  l++)
        {
          if (edges[l].indices[0] == edge.indices[0] &&
              edges[l].indices[1] == edge.indices[1])
            break;
          if (edges[l].indices[0] == edge.indices[1] &&
              edges[l].indices[1] == edge.indices[0])
            break;
        }

        if (l == edges.size())
        {
          geometries[i].triangles[j].edges[k] = edges.size();
          edges.push_back(edge);
        }
        else
          geometries[i].triangles[j].edges[k] = l;
      }
    }
  }
}

AABB Mesh::generateBoundingAABB() const
{
  AABB bounds;

  if (vertices.empty())
    return bounds;

  vec3 minimum(std::numeric_limits<float>::max());
  vec3 maximum(std::numeric_limits<float>::min());

  for (size_t i = 0;  i < vertices.size();  i++)
  {
    const vec3& position = vertices[i].position;

    minimum = min(minimum, position);
    maximum = max(maximum, position);
  }

  bounds.setBounds(minimum.x, minimum.y, minimum.z,
                   maximum.x, maximum.y, maximum.z);

  return bounds;
}

Sphere Mesh::generateBoundingSphere() const
{
  Sphere bounds;

  if (vertices.empty())
    return bounds;

  bounds.center = vertices[0].position;

  for (size_t i = 1;  i < vertices.size();  i++)
    bounds.envelop(vertices[i].position);

  return bounds;
}

bool Mesh::isValid() const
{
  if (vertices.empty())
    return false;

  for (size_t i = 0;  i < vertices.size();  i++)
  {
    const MeshVertex& vertex = vertices[i];

    if (!all(isfinite(vertex.position)) ||
        !all(isfinite(vertex.normal)) ||
        !all(isfinite(vertex.texcoord)))
    {
      return false;
    }
  }

  for (size_t i = 0;  i < geometries.size();  i++)
  {
    if (geometries[i].triangles.empty())
      return false;

    const MeshGeometry::TriangleList& triangles = geometries[i].triangles;

    for (size_t j = 0;  j < triangles.size();  j++)
    {
      const MeshTriangle& triangle = triangles[j];

      if (!all(isfinite(triangle.normal)))
        return false;

      if (triangle.indices[0] >= vertices.size() ||
          triangle.indices[1] >= vertices.size() ||
          triangle.indices[2] >= vertices.size())
      {
        return false;
      }

      if (!edges.empty())
      {
        if (triangle.edges[0] >= edges.size() ||
            triangle.edges[1] >= edges.size() ||
            triangle.edges[2] >= edges.size())
        {
          return false;
        }
      }
    }
  }

  if (!edges.empty())
  {
    for (size_t i = 0;  i < edges.size();  i++)
    {
      const MeshEdge& edge = edges[i];

      if (edge.indices[0] >= vertices.size() ||
          edge.indices[1] >= vertices.size())
      {
        return false;
      }
    }
  }

  return true;
}

unsigned int Mesh::getTriangleCount() const
{
  unsigned int count = 0;

  for (size_t i = 0;  i < geometries.size();  i++)
    count += (unsigned int) geometries[i].triangles.size();

  return count;
}

Ref<Mesh> Mesh::read(ResourceCache& cache, const String& name)
{
  MeshReader reader(cache);
  return reader.read(name);
}

///////////////////////////////////////////////////////////////////////

VertexMerger::VertexMerger():
  targetCount(0),
  mode(PRESERVE_NORMALS)
{
}

VertexMerger::VertexMerger(const Mesh::VertexList& initVertices):
  targetCount(0),
  mode(PRESERVE_NORMALS)
{
  importPositions(initVertices);
}

void VertexMerger::importPositions(const Mesh::VertexList& initVertices)
{
  vertices.resize(initVertices.size());
  for (size_t i = 0;  i < vertices.size();  i++)
    vertices[i].position = initVertices[i].position;
}

unsigned int VertexMerger::addAttributeLayer(unsigned int vertexIndex,
                                             const vec3& normal,
                                             const vec2& texcoord)
{
  Vertex& vertex = vertices[vertexIndex];

  if (mode == PRESERVE_NORMALS)
  {
    for (Vertex::LayerList::iterator i = vertex.layers.begin();  i != vertex.layers.end();  i++)
    {
      if (all(equalEpsilon(i->normal, normal, 0.001f)) &&
          all(equalEpsilon(i->texcoord, texcoord, 0.001f)))
      {
        return i->index;
      }
    }

    vertex.layers.push_back(VertexLayer());
    VertexLayer& layer = vertex.layers.back();

    layer.normal = normal;
    layer.texcoord = texcoord;
    layer.index = targetCount++;

    return layer.index;
  }
  else
  {
    size_t index;
    bool discontinuous = true;

    for (Vertex::LayerList::iterator i = vertex.layers.begin();  i != vertex.layers.end();  i++)
    {
      if (all(equalEpsilon(i->texcoord, texcoord, 0.001f)))
      {
        if (all(equalEpsilon(i->normal, normal, 0.001f)))
          return i->index;

        discontinuous = false;
        index = i->index;
      }
    }

    if (discontinuous)
      index = targetCount++;

    vertex.layers.push_back(VertexLayer());
    VertexLayer& layer = vertex.layers.back();

    layer.normal = normal;
    layer.texcoord = texcoord;
    layer.index = index;

    return index;
  }
}

void VertexMerger::realizeVertices(Mesh::VertexList& result) const
{
  result.resize(targetCount);

  for (size_t i = 0;  i < vertices.size();  i++)
  {
    const Vertex& vertex = vertices[i];

    vec3 normal;

    if (mode == MERGE_NORMALS)
    {
      for (size_t j = 0;  j < vertex.layers.size();  j++)
        normal += vertex.layers[j].normal;

      normal = normalize(normal);
    }

    for (size_t j = 0;  j < vertex.layers.size();  j++)
    {
      const VertexLayer& layer = vertex.layers[j];

      result[layer.index].position = vertex.position;
      result[layer.index].texcoord = layer.texcoord;

      if (mode == MERGE_NORMALS)
        result[layer.index].normal = normal;
      else
        result[layer.index].normal = layer.normal;
    }
  }
}

void VertexMerger::setNormalMode(NormalMode newMode)
{
  mode = newMode;
}

///////////////////////////////////////////////////////////////////////

MeshReader::MeshReader(ResourceCache& index):
  ResourceReader(index)
{
}

Ref<Mesh> MeshReader::read(const String& name, const Path& path)
{
  std::ifstream stream(path.asString().c_str(), std::ios::in | std::ios::binary);
  if (stream.fail())
  {
    logError("Failed to open mesh \'%s\'", name.c_str());
    return NULL;
  }

  String line;
  unsigned int lineNumber = 0;

  std::vector<vec3> positions;
  std::vector<vec3> normals;
  std::vector<vec2> texcoords;
  std::vector<Triplet> triplets;

  FaceGroupList groups;
  FaceGroup* group = NULL;

  while (std::getline(stream, line))
  {
    const char* text = line.c_str();
    ++lineNumber;

    if (!interesting(&text))
      continue;

    try
    {
      String command = parseName(&text);

      if (command == "g")
      {
        // Silently ignore group names
      }
      else if (command == "o")
      {
        // Silently ignore object names
      }
      else if (command == "s")
      {
        // Silently ignore smoothing
      }
      else if (command == "v")
      {
        vec3 vertex;

        vertex.x = parseFloat(&text);
        vertex.y = parseFloat(&text);
        vertex.z = parseFloat(&text);
        positions.push_back(vertex);
      }
      else if (command == "vt")
      {
        vec2 texcoord;

        texcoord.x = parseFloat(&text);
        texcoord.y = parseFloat(&text);
        texcoords.push_back(texcoord);
      }
      else if (command == "vn")
      {
        vec3 normal;

        normal.x = parseFloat(&text);
        normal.y = parseFloat(&text);
        normal.z = parseFloat(&text);
        normals.push_back(normalize(normal));
      }
      else if (command == "usemtl")
      {
        String shaderName = parseName(&text);

        group = NULL;

        for (FaceGroupList::iterator g = groups.begin();  g != groups.end();  g++)
        {
          if (g->name == shaderName)
            group = &(*g);
        }

        if (!group)
        {
          groups.push_back(FaceGroup());
          groups.back().name = shaderName;
          group = &(groups.back());
        }
      }
      else if (command == "mtllib")
      {
        // Silently ignore .mtl material files
      }
      else if (command == "f")
      {
        if (!group)
          throw Exception("Expected \'usemtl\' before \'f\'");

        triplets.clear();

        while (*text != '\0')
        {
          triplets.push_back(Triplet());
          Triplet& triplet = triplets.back();

          triplet.vertex = parseInteger(&text);
          triplet.texcoord = 0;
          triplet.normal = 0;

          if (*text == '/')
          {
            if (std::isdigit(*(++text)))
              triplet.texcoord = parseInteger(&text);

            if (*text == '/')
            {
              if (std::isdigit(*(++text)))
                triplet.normal = parseInteger(&text);
            }
          }

          while (std::isspace(*text))
            text++;
        }

        for (size_t i = 2;  i < triplets.size();  i++)
        {
          group->faces.push_back(Face());
          Face& face = group->faces.back();

          face.p[0] = triplets[0];
          face.p[1] = triplets[i - 1];
          face.p[2] = triplets[i];
        }
      }
      else
      {
        logWarning("Unknown command \'%s\' in mesh \'%s\' line %d",
                   command.c_str(),
                   name.c_str(),
                   lineNumber);
      }
    }
    catch (Exception& e)
    {
      logError("%s in mesh \'%s\' line %d",
               e.what(),
               name.c_str(),
               lineNumber);

      return NULL;
    }
  }

  Ref<Mesh> mesh = new Mesh(ResourceInfo(cache, name, path));

  mesh->vertices.resize(positions.size());

  for (size_t i = 0;  i < positions.size();  i++)
    mesh->vertices[i].position = positions[i];

  VertexMerger merger(mesh->vertices);

  for (FaceGroupList::const_iterator g = groups.begin();  g != groups.end();  g++)
  {
    mesh->geometries.push_back(MeshGeometry());
    MeshGeometry& geometry = mesh->geometries.back();

    const FaceList& faces = g->faces;

    geometry.shaderName = g->name;
    geometry.triangles.resize(faces.size());

    for (size_t i = 0;  i < faces.size();  i++)
    {
      const Face& face = faces[i];
      MeshTriangle& triangle = geometry.triangles[i];

      for (size_t j = 0;  j < 3;  j++)
      {
        const Triplet& point = face.p[j];

        vec3 normal;
        if (point.normal)
          normal = normals[point.normal - 1];

        vec2 texcoord;
        if (point.texcoord)
          texcoord = texcoords[point.texcoord - 1];

        triangle.indices[j] = merger.addAttributeLayer(point.vertex - 1, normal, texcoord);
      }
    }
  }

  merger.realizeVertices(mesh->vertices);

  mesh->generateTangents();

  return mesh;
}

String MeshReader::parseName(const char** text)
{
  while (std::isspace(**text))
    (*text)++;

  String result;

  while (std::isalnum(**text) || **text == '_')
  {
    result.append(1, **text);
    (*text)++;
  }

  if (!result.size())
    throw Exception("Expected but missing name");

  return result;
}

int MeshReader::parseInteger(const char** text)
{
  char* end;

  const int result = std::strtol(*text, &end, 0);
  if (end == *text)
    throw Exception("Expected but missing integer value");

  *text = end;
  return result;
}

float MeshReader::parseFloat(const char** text)
{
  char* end;

  const float result = float(std::strtod(*text, &end));
  if (end == *text)
    throw Exception("Expected but missing float value");

  *text = end;
  return result;
}

bool MeshReader::interesting(const char** text)
{
  if (std::isspace(**text) || **text == '#' || **text == '\0' || **text == '\r')
    return false;

  return true;
}

///////////////////////////////////////////////////////////////////////

bool MeshWriter::write(const Path& path, const Mesh& mesh)
{
  std::ofstream stream(path.asString().c_str());
  if (!stream.is_open())
  {
    logError("Failed to open \'%s\' for writing",
             path.asString().c_str());
    return false;
  }

  for (Mesh::VertexList::const_iterator v = mesh.vertices.begin();
       v != mesh.vertices.end();
       v++)
  {
    stream << "v " << v->position.x << ' ' << v->position.y << ' ' << v->position.z << '\n';
  }

  for (Mesh::VertexList::const_iterator v = mesh.vertices.begin();
       v != mesh.vertices.end();
       v++)
  {
    stream << "vn " << v->normal.x << ' ' << v->normal.y << ' ' << v->normal.z << '\n';
  }

  for (Mesh::VertexList::const_iterator v = mesh.vertices.begin();
       v != mesh.vertices.end();
       v++)
  {
    stream << "vt " << v->texcoord.x << ' ' << v->texcoord.y << '\n';
  }

  for (Mesh::GeometryList::const_iterator g = mesh.geometries.begin();
       g != mesh.geometries.end();
       g++)
  {
    stream << "usemtl " << g->shaderName << '\n';

    for (MeshGeometry::TriangleList::const_iterator t = g->triangles.begin();
         t != g->triangles.end();
         t++)
    {
      stream << "f";

      for (size_t i = 0;  i < 3;  i++)
      {
        unsigned int index = t->indices[i] + 1;
        stream << ' ' << index << '/' << index << '/' << index;
      }

      stream << '\n';
    }
  }

  stream.close();
  return true;
}

///////////////////////////////////////////////////////////////////////

} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
