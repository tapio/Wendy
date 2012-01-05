#version 120

attribute vec3 wyPosition;
attribute vec3 wyNormal;
attribute vec2 wyTexCoord;
attribute vec3 wyTangent;
attribute vec3 wyBitangent;

varying vec2 texCoord;
varying vec3 normal;
varying vec3 tangent;
varying vec3 bitangent;

void main()
{
  texCoord = wyTexCoord;
  normal = (wyMV * vec4(wyNormal, 0.0)).xyz;
  tangent = (wyMV * vec4(wyTangent, 0.0)).xyz;
  bitangent = (wyMV * vec4(wyBitangent, 0.0)).xyz;

  gl_Position = wyMVP * vec4(wyPosition, 1.0);
}
