
struct Light
{
  float3 direction;
  float3 color;
};

float4 main(uniform samplerRECT colorbuffer,
            uniform samplerRECT normalbuffer,
            /*
            uniform float minZ,
            uniform float maxZ,
            uniform float halfFOV,
            uniform float aspect,
            */
            uniform Light light,
            in float2 mapping) : COLOR
{
  float3 color = texRECT(colorbuffer, mapping).rgb;
  float3 normal = texRECT(normalbuffer, mapping).xyz * 2 - float(0.5);

  return float4(color * light.color * dot(normal, light.direction), 1);
}

