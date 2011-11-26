
uniform sampler2D colormap;
uniform sampler2D normalmap;

in vec3 gPosition;
in vec2 gTexCoord;
in vec3 gNormal;
in vec3 gTangent;
in vec3 gBinormal;

void main()
{
  vec4 ns = texture2D(normalmap, gTexCoord);
  vec3 n = normalize(2.0 * ns.xyz - 1.0);

  vec3 N = normalize(gNormal);
  vec3 T = normalize(gTangent);
  vec3 B = normalize(gBinormal);
  mat3 TBN = transpose(mat3(T,B,N));
  // Ok, matrix multiplication per fragment in a waste,
  // but allows us to leave the light shaders untouched.
  vec3 bump = normalize(n * TBN);

  gl_FragData[0] = texture2D(colormap, gTexCoord);
  gl_FragData[1] = vec4(0.5f * bump + 0.5f, ns.a);
}

