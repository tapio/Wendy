
uniform sampler2D colormap;
uniform sampler2D normalmap;

varying vec3 normal;
varying vec2 texCoord;

#ifdef SPECULAR_MASK
  #define MASK texture2D(normalmap, texCoord).a
#else
  #define MASK 0.0
#endif

void main()
{
  vec4 color = texture2D(colormap, texCoord);

#ifdef MASKED
  if (color.a - 0.5 < 0.0)
    discard;
#endif

  gl_FragData[0] = color;
  gl_FragData[1] = vec4(normalize(normal) / 2.0 + 0.5, MASK);
}

