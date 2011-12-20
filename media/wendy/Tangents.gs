
#extension GL_EXT_geometry_shader4 : require

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

uniform sampler2D colormap;
uniform sampler2D normalmap;

in vec3 vPosition[];
in vec2 vTexCoord[];
in vec3 vNormal[];

out vec2 gTexCoord;
out vec3 gNormal;
out vec3 gTangent;
out vec3 gBitangent;

// This geometry shader calculates tangents for
// normal/parallax mapping on the fly.

// Gram-Schmidt
vec3 orthogonalize(const in vec3 t, const in vec3 n)
{
  return (t - dot(n, t) * n);
}

// Set the attributes and emit the vertex
void emit(const in int id, const in vec3 t1, const in vec3 t2)
{
  gTexCoord = vTexCoord[id];
  gNormal = vNormal[id];
  gTangent = orthogonalize(t1, vNormal[id]);
  gBitangent = cross(gNormal, gTangent);
  bool mirrored = (dot(cross(vNormal[id], t1), t2) >= 0.0f);
  if (mirrored) {
    gTangent *= -1.0f;
    gBitangent *= -1.0f;
  }
  gl_Position = wyP * vec4(vPosition[id], 1.0f);
  EmitVertex();
}

void main()
{
  // Two edge vectors
  vec3 e1 = vPosition[1] - vPosition[0];
  vec3 e2 = vPosition[2] - vPosition[0];

  // Tex coord gradient vectors
  vec2 t1 = vTexCoord[1] - vTexCoord[0];
  vec2 t2 = vTexCoord[2] - vTexCoord[0];

  float f = 1.0f / (t1.x * t2.y - t2.x * t1.y);

  // Magic
  vec3 tan1 = vec3((t2.y * e1.x - t1.y * e2.x) * f,
                   (t2.y * e1.y - t1.y * e2.y) * f,
                   (t2.y * e1.z - t1.y * e2.z) * f);
  vec3 tan2 = vec3((t1.x * e2.x - t2.x * e1.x) * f,
                   (t1.x * e2.y - t2.x * e1.y) * f,
                   (t1.x * e2.z - t2.x * e1.z) * f);

  emit(0, tan1, tan2);
  emit(1, tan1, tan2);
  emit(2, tan1, tan2);

  EndPrimitive();
}

