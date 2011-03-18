
uniform sampler2D colormap;
uniform sampler2D normalmap;

in vec3 normal;
in vec2 mapping;

void main()
{
  vec4 color = texture2D(colormap, mapping);

  if (color.a - 0.5 < 0)
    discard;

  gl_FragData[0] = color;
  gl_FragData[1] = vec4(normalize(normal * sign(normal.z)) / 2 + 0.5, 0);
}
