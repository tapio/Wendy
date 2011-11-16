
uniform sampler2D colormap;
uniform sampler2D normalmap;

in vec3 gPosition;
in vec2 gTexCoord;
in vec3 gNormal;
in vec3 gTangent;

void main()
{
  vec4 NS = texture2D(normalmap, gTexCoord);
  vec3 N = NS.xyz;

  gl_FragData[0] = texture2D(colormap, gTexCoord);
  gl_FragData[1] = vec4(normalize(gNormal) / 2.0 + 0.5, NS.a);
}

