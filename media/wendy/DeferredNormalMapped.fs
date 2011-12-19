
uniform sampler2D colormap;
uniform sampler2D normalmap;

varying vec2 texCoord;
varying vec3 normal;
varying vec3 tangent;

void main()
{
  vec4 ns = texture2D(normalmap, texCoord);
  vec3 n = normalize(ns.xyz - 0.5f);

  vec3 N = normalize(normal);
  vec3 T = normalize(tangent);
  vec3 B = normalize(cross(N, T));
  mat3 TBN = mat3(T,B,N);
  vec3 bump = normalize(n * TBN);

  gl_FragData[0] = texture2D(colormap, texCoord);
  gl_FragData[1] = vec4(0.5f * bump + 0.5f, ns.a);
}

