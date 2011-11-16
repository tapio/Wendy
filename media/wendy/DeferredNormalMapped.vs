
in vec3 wyPosition;
in vec2 wyTexCoord;
in vec3 wyNormal;

out vec3 vPosition;
out vec2 vTexCoord; 
out vec3 vNormal; 
 
void main()
{
  vPosition = (wyMV * vec4(wyPosition, 1.0)).xyz;
  vTexCoord = wyTexCoord;
  vNormal = normalize((wyMV * vec4(wyNormal, 0.0)).xyz);
}
