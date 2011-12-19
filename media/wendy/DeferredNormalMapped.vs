
attribute vec3 wyPosition;
attribute vec3 wyNormal;
attribute vec2 wyTexCoord;
attribute vec3 wyTangent;

varying vec2 texCoord;
varying vec3 normal;
varying vec3 tangent;

void main()
{
  texCoord = wyTexCoord;
  normal = (wyMV * vec4(wyNormal, 0.0)).xyz;
  tangent = wyTangent;

  gl_Position = wyMVP * vec4(wyPosition, 1.0);
}
