///////////////////////////////////////////////////////////////////////
// Wendy default renderer
// Copyright (c) 2006 Camilla Berglund <elmindreda@elmindreda.org>
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any
// damages arising from the use of this software.
//
// Permission is granted to anyone to use this software for any
// purpose, including commercial applications, and to alter it and
// redistribute it freely, subject to the following restrictions:
//
//  1. The origin of this software must not be misrepresented; you
//     must not claim that you wrote the original software. If you use
//     this software in a product, an acknowledgment in the product
//     documentation would be appreciated but is not required.
//
//  2. Altered source versions must be plainly marked as such, and
//     must not be misrepresented as being the original software.
//
//  3. This notice may not be removed or altered from any source
//     distribution.
//
///////////////////////////////////////////////////////////////////////

#include <wendy/Config.hpp>

#include <wendy/Bimap.hpp>

#include <wendy/GLTexture.hpp>
#include <wendy/GLBuffer.hpp>
#include <wendy/GLProgram.hpp>
#include <wendy/GLContext.hpp>

#include <wendy/RenderPool.hpp>
#include <wendy/RenderState.hpp>
#include <wendy/RenderSystem.hpp>
#include <wendy/RenderMaterial.hpp>

#include <algorithm>

#include <pugixml.hpp>

///////////////////////////////////////////////////////////////////////

namespace wendy
{
  namespace render
  {

///////////////////////////////////////////////////////////////////////

namespace
{

Bimap<String, GL::CullMode> cullModeMap;
Bimap<String, GL::BlendFactor> blendFactorMap;
Bimap<String, GL::Function> functionMap;
Bimap<String, GL::StencilOp> operationMap;
Bimap<String, GL::FilterMode> filterModeMap;
Bimap<String, GL::AddressMode> addressModeMap;
Bimap<String, System::Type> systemTypeMap;
Bimap<String, Phase> phaseMap;

Bimap<GL::SamplerType, GL::TextureType> textureTypeMap;

const uint MATERIAL_XML_VERSION = 9;

void initializeMaps()
{
  if (cullModeMap.isEmpty())
  {
    cullModeMap["none"] = GL::CULL_NONE;
    cullModeMap["front"] = GL::CULL_FRONT;
    cullModeMap["back"] = GL::CULL_BACK;
    cullModeMap["both"] = GL::CULL_BOTH;
  }

  if (blendFactorMap.isEmpty())
  {
    blendFactorMap["zero"] = GL::BLEND_ZERO;
    blendFactorMap["one"] = GL::BLEND_ONE;
    blendFactorMap["src color"] = GL::BLEND_SRC_COLOR;
    blendFactorMap["dst color"] = GL::BLEND_DST_COLOR;
    blendFactorMap["src alpha"] = GL::BLEND_SRC_ALPHA;
    blendFactorMap["dst alpha"] = GL::BLEND_DST_ALPHA;
    blendFactorMap["one minus src color"] = GL::BLEND_ONE_MINUS_SRC_COLOR;
    blendFactorMap["one minus dst color"] = GL::BLEND_ONE_MINUS_DST_COLOR;
    blendFactorMap["one minus src alpha"] = GL::BLEND_ONE_MINUS_SRC_ALPHA;
    blendFactorMap["one minus dst alpha"] = GL::BLEND_ONE_MINUS_DST_ALPHA;
  }

  if (functionMap.isEmpty())
  {
    functionMap["never"] = GL::ALLOW_NEVER;
    functionMap["always"] = GL::ALLOW_ALWAYS;
    functionMap["equal"] = GL::ALLOW_EQUAL;
    functionMap["not equal"] = GL::ALLOW_NOT_EQUAL;
    functionMap["lesser"] = GL::ALLOW_LESSER;
    functionMap["lesser or equal"] = GL::ALLOW_LESSER_EQUAL;
    functionMap["greater"] = GL::ALLOW_GREATER;
    functionMap["greater or equal"] = GL::ALLOW_GREATER_EQUAL;
  }

  if (operationMap.isEmpty())
  {
    operationMap["keep"] = GL::OP_KEEP;
    operationMap["zero"] = GL::OP_ZERO;
    operationMap["replace"] = GL::OP_REPLACE;
    operationMap["increase"] = GL::OP_INCREASE;
    operationMap["decrease"] = GL::OP_DECREASE;
    operationMap["invert"] = GL::OP_INVERT;
    operationMap["increase wrap"] = GL::OP_INCREASE_WRAP;
    operationMap["decrease wrap"] = GL::OP_DECREASE_WRAP;
  }

  if (addressModeMap.isEmpty())
  {
    addressModeMap["wrap"] = GL::ADDRESS_WRAP;
    addressModeMap["clamp"] = GL::ADDRESS_CLAMP;
  }

  if (filterModeMap.isEmpty())
  {
    filterModeMap["nearest"] = GL::FILTER_NEAREST;
    filterModeMap["bilinear"] = GL::FILTER_BILINEAR;
    filterModeMap["trilinear"] = GL::FILTER_TRILINEAR;
  }

  if (textureTypeMap.isEmpty())
  {
    textureTypeMap[GL::SAMPLER_1D] = GL::TEXTURE_1D;
    textureTypeMap[GL::SAMPLER_2D] = GL::TEXTURE_2D;
    textureTypeMap[GL::SAMPLER_3D] = GL::TEXTURE_3D;
    textureTypeMap[GL::SAMPLER_RECT] = GL::TEXTURE_RECT;
    textureTypeMap[GL::SAMPLER_CUBE] = GL::TEXTURE_CUBE;
  }

  if (systemTypeMap.isEmpty())
  {
    systemTypeMap["forward"] = System::FORWARD;
  }

  if (phaseMap.isEmpty())
  {
    phaseMap[""] = PHASE_DEFAULT;
    phaseMap["default"] = PHASE_DEFAULT;
    phaseMap["shadowmap"] = PHASE_SHADOWMAP;
  }
}

} /*namespace*/

///////////////////////////////////////////////////////////////////////

bool parsePass(System& system, Pass& pass, pugi::xml_node root)
{
  initializeMaps();

  GL::Context& context = system.context();
  ResourceCache& cache = system.cache();

  if (pugi::xml_node node = root.child("blending"))
  {
    if (pugi::xml_attribute a = node.attribute("src"))
    {
      if (blendFactorMap.hasKey(a.value()))
        pass.setBlendFactors(blendFactorMap[a.value()], pass.dstFactor());
      else
      {
        logError("Invalid source blend factor %s", a.value());
        return false;
      }
    }

    if (pugi::xml_attribute a = node.attribute("dst"))
    {
      if (blendFactorMap.hasKey(a.value()))
        pass.setBlendFactors(pass.srcFactor(), blendFactorMap[a.value()]);
      else
      {
        logError("Invalid destination blend factor %s", a.value());
        return false;
      }
    }
  }

  if (pugi::xml_node node = root.child("color"))
  {
    if (pugi::xml_attribute a = node.attribute("writing"))
      pass.setColorWriting(a.as_bool());

    if (pugi::xml_attribute a = node.attribute("multisampling"))
      pass.setMultisampling(a.as_bool());
  }

  if (pugi::xml_node node = root.child("depth"))
  {
    if (pugi::xml_attribute a = node.attribute("testing"))
      pass.setDepthTesting(a.as_bool());

    if (pugi::xml_attribute a = node.attribute("writing"))
      pass.setDepthWriting(a.as_bool());

    if (pugi::xml_attribute a = node.attribute("function"))
    {
      if (functionMap.hasKey(a.value()))
        pass.setDepthFunction(functionMap[a.value()]);
      else
      {
        logError("Invalid depth function %s", a.value());
        return false;
      }
    }
  }

  if (pugi::xml_node node = root.child("stencil"))
  {
    if (pugi::xml_attribute a = node.attribute("testing"))
      pass.setStencilTesting(a.as_bool());

    if (pugi::xml_attribute a = node.attribute("mask"))
      pass.setStencilWriteMask(a.as_uint());

    if (pugi::xml_attribute a = node.attribute("reference"))
      pass.setStencilReference(a.as_uint());

    if (pugi::xml_attribute a = node.attribute("stencilFail"))
    {
      if (functionMap.hasKey(a.value()))
        pass.setStencilFailOperation(operationMap[a.value()]);
      else
      {
        logError("Invalid stencil fail operation %s", a.value());
        return false;
      }
    }

    if (pugi::xml_attribute a = node.attribute("depthFail"))
    {
      if (functionMap.hasKey(a.value()))
        pass.setDepthFailOperation(operationMap[a.value()]);
      else
      {
        logError("Invalid depth fail operation %s", a.value());
        return false;
      }
    }

    if (pugi::xml_attribute a = node.attribute("depthPass"))
    {
      if (functionMap.hasKey(a.value()))
        pass.setDepthPassOperation(operationMap[a.value()]);
      else
      {
        logError("Invalid depth pass operation %s", a.value());
        return false;
      }
    }

    if (pugi::xml_attribute a = node.attribute("function"))
    {
      if (functionMap.hasKey(a.value()))
        pass.setStencilFunction(functionMap[a.value()]);
      else
      {
        logError("Invalid stencil function %s", a.value());
        return false;
      }
    }
  }

  if (pugi::xml_node node = root.child("polygon"))
  {
    if (pugi::xml_attribute a = node.attribute("wireframe"))
      pass.setWireframe(a.as_bool());

    if (pugi::xml_attribute a = node.attribute("cull"))
    {
      if (cullModeMap.hasKey(a.value()))
        pass.setCullMode(cullModeMap[a.value()]);
      else
      {
        logError("Invalid cull mode %s", a.value());
        return false;
      }
    }
  }

  if (pugi::xml_node node = root.child("line"))
  {
    if (pugi::xml_attribute a = node.attribute("smoothing"))
      pass.setLineSmoothing(a.as_bool());

    if (pugi::xml_attribute a = node.attribute("width"))
      pass.setLineWidth(a.as_float());
  }

  if (pugi::xml_node node = root.child("program"))
  {
    const String vertexShaderName(node.attribute("vs").value());
    if (vertexShaderName.empty())
    {
      logError("No vertex shader specified");
      return false;
    }

    const String fragmentShaderName(node.attribute("fs").value());
    if (fragmentShaderName.empty())
    {
      logError("No fragment shader specified");
      return false;
    }

    Ref<GL::Program> program = GL::Program::read(context,
                                                 vertexShaderName,
                                                 fragmentShaderName);
    if (!program)
    {
      logError("Failed to load program");
      return false;
    }

    pass.setProgram(program);

    for (auto s : node.children("sampler"))
    {
      const String samplerName(s.attribute("name").value());
      if (samplerName.empty())
      {
        logWarning("Program %s lists unnamed sampler uniform",
                   program->name().c_str());

        continue;
      }

      GL::Sampler* sampler = program->findSampler(samplerName.c_str());
      if (!sampler)
      {
        logWarning("Program %s does not have sampler uniform %s",
                   program->name().c_str(),
                   samplerName.c_str());

        continue;
      }

      Ref<GL::Texture> texture;

      if (pugi::xml_attribute a = s.attribute("image"))
      {
        GL::TextureParams params(textureTypeMap[sampler->type()], GL::TF_NONE);

        if (s.attribute("mipmapped").as_bool())
          params.flags |= GL::TF_MIPMAPPED;

        if (s.attribute("sRGB").as_bool())
          params.flags |= GL::TF_SRGB;

        texture = GL::Texture::read(context, params, a.value());
      }
      else if (pugi::xml_attribute a = s.attribute("texture"))
        texture = cache.find<GL::Texture>(a.value());
      else
      {
        logError("No texture specified for sampler %s of program %s",
                  samplerName.c_str(),
                  program->name().c_str());

        return false;
      }

      if (!texture)
      {
        logError("Failed to find texture for sampler %s of program %s",
                  samplerName.c_str(),
                  program->name().c_str());

        return false;
      }

      if (pugi::xml_attribute a = root.attribute("anisotropy"))
        texture->setMaxAnisotropy(a.as_float());

      if (pugi::xml_attribute a = s.attribute("filter"))
      {
        if (filterModeMap.hasKey(a.value()))
          texture->setFilterMode(filterModeMap[a.value()]);
        else
        {
          logError("Invalid filter mode name %s", a.value());
          return false;
        }
      }

      if (pugi::xml_attribute a = s.attribute("address"))
      {
        if (addressModeMap.hasKey(a.value()))
          texture->setAddressMode(addressModeMap[a.value()]);
        else
        {
          logError("Invalid address mode name %s", a.value());
          return false;
        }
      }

      pass.setSamplerState(samplerName.c_str(), texture);
    }

    for (auto u : node.children("uniform"))
    {
      const String uniformName(u.attribute("name").value());
      if (uniformName.empty())
      {
        logWarning("Program %s lists unnamed uniform",
                   program->name().c_str());

        continue;
      }

      const GL::Uniform* uniform = program->findUniform(uniformName.c_str());
      if (!uniform)
      {
        logWarning("Program %s does not have uniform %s",
                   program->name().c_str(),
                   uniformName.c_str());

        continue;
      }

      pugi::xml_attribute attribute = u.attribute("value");
      if (!attribute)
      {
        logError("Missing value for uniform %s of program %s",
                 uniformName.c_str(),
                 program->name().c_str());

        return false;
      }

      switch (uniform->type())
      {
        case GL::UNIFORM_FLOAT:
          pass.setUniformState(uniformName.c_str(), attribute.as_float());
          break;
        case GL::UNIFORM_VEC2:
          pass.setUniformState(uniformName.c_str(), vec2Cast(attribute.value()));
          break;
        case GL::UNIFORM_VEC3:
          pass.setUniformState(uniformName.c_str(), vec3Cast(attribute.value()));
          break;
        case GL::UNIFORM_VEC4:
          pass.setUniformState(uniformName.c_str(), vec4Cast(attribute.value()));
          break;
        case GL::UNIFORM_MAT2:
          pass.setUniformState(uniformName.c_str(), mat2Cast(attribute.value()));
          break;
        case GL::UNIFORM_MAT3:
          pass.setUniformState(uniformName.c_str(), mat3Cast(attribute.value()));
          break;
        case GL::UNIFORM_MAT4:
          pass.setUniformState(uniformName.c_str(), mat4Cast(attribute.value()));
          break;
      }
    }
  }

  return true;
}

///////////////////////////////////////////////////////////////////////

void Material::setSamplerStates(const char* name, GL::Texture* newTexture)
{
  for (uint i = 0;  i < 2;  i++)
  {
    for (auto& p : m_techniques[i].passes)
    {
      if (p.hasSamplerState(name))
        p.setSamplerState(name, newTexture);
    }
  }
}

Ref<Material> Material::create(const ResourceInfo& info, System& system)
{
  return new Material(info);
}

Ref<Material> Material::read(System& system, const String& name)
{
  MaterialReader reader(system);
  return reader.read(name);
}

Material::Material(const ResourceInfo& info):
  Resource(info)
{
}

///////////////////////////////////////////////////////////////////////

MaterialReader::MaterialReader(System& initSystem):
  ResourceReader<Material>(initSystem.cache()),
  system(initSystem)
{
  initializeMaps();
}

Ref<Material> MaterialReader::read(const String& name, const Path& path)
{
  std::ifstream stream(path.name().c_str());
  if (stream.fail())
  {
    logError("Failed to open material %s", name.c_str());
    return nullptr;
  }

  pugi::xml_document document;

  const pugi::xml_parse_result result = document.load(stream);
  if (!result)
  {
    logError("Failed to load material %s: %s",
             name.c_str(),
             result.description());
    return nullptr;
  }

  pugi::xml_node root = document.child("material");
  if (!root || root.attribute("version").as_uint() != MATERIAL_XML_VERSION)
  {
    logError("Material file format mismatch in %s", name.c_str());
    return nullptr;
  }

  std::vector<bool> phases(2, false);

  GL::Context& context = system.context();

  Ref<Material> material = Material::create(ResourceInfo(cache, name, path), system);

  for (auto t : root.children("technique"))
  {
    const String phaseName(t.attribute("phase").value());
    if (!phaseMap.hasKey(phaseName))
    {
      logError("Invalid render phase %s in material %s",
               phaseName.c_str(),
               name.c_str());
      return nullptr;
    }

    const Phase phase = phaseMap[phaseName];
    if (phases[phase])
      continue;

    const String typeName(t.attribute("type").value());
    if (!systemTypeMap.hasKey(typeName))
    {
      logError("Invalid render system type %s in material %s",
               typeName.c_str(),
               name.c_str());
      return nullptr;
    }

    const System::Type type = systemTypeMap[typeName];
    if (system.type() != type)
      continue;

    Technique& technique = material->technique(phase);

    for (auto p : t.children("pass"))
    {
      technique.passes.push_back(Pass());
      Pass& pass = technique.passes.back();

      if (!parsePass(system, pass, p))
      {
        logError("Failed to parse pass for material %s", name.c_str());
        return nullptr;
      }

      phases[phase] = true;
    }
  }

  return material;
}

///////////////////////////////////////////////////////////////////////

  } /*namespace render*/
} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
