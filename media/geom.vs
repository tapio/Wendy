#version 330

in vec3 wyPosition;
in vec3 wyNormal;

out vec3 vPosition;
out vec3 vNormal;

void main()
{
  vNormal = (wyMV * vec4(wyPosition, 1)).xyz;
  gl_Position = wyMVP * vec4(wyPosition, 1);
  vPosition = gl_Position.xyz;
}

