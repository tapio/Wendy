
#extension GL_EXT_geometry_shader4 : require

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;

uniform sampler2D colormap;
uniform sampler2D normalmap;

in vec3 vPosition[];
in vec2 vTexCoord[];
in vec3 vNormal[];

out vec3 gPosition;
out vec2 gTexCoord;
out vec3 gNormal;
out vec3 gTangent;

// This geometry shader calculates tangents for
// normal/parallax mapping on the fly.

void emit(const in int id)
{
  gPosition = vPosition[id];
  gTexCoord = vTexCoord[id];
  gNormal = vNormal[id];
  gl_Position = wyP * vec4(vPosition[id], 1.0);
  EmitVertex();
}

void main()
{
  // FIXME: Implement
  gTangent = vec3(0.0, 0.0, 0.0);

  emit(0);
  emit(1);
  emit(2);

  EndPrimitive();
}

