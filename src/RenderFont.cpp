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

#include <wendy/GLTexture.hpp>
#include <wendy/GLBuffer.hpp>
#include <wendy/GLProgram.hpp>
#include <wendy/GLContext.hpp>

#include <wendy/RenderPool.hpp>
#include <wendy/RenderState.hpp>
#include <wendy/RenderFont.hpp>

#include <pugixml.hpp>

#include <utf8.h>

///////////////////////////////////////////////////////////////////////

namespace wendy
{
  namespace render
  {

///////////////////////////////////////////////////////////////////////

namespace
{

const uint FONT_XML_VERSION = 2;

} /*namespace*/

///////////////////////////////////////////////////////////////////////

void Font::drawText(vec2 pen, vec4 color, const char* text)
{
  uint vertexCount = 0;

  // Realize vertices for glyphs
  {
    const size_t length = std::strlen(text);
    m_vertices.resize(length * 6);

    for (const char* c = text;  *c != '\0'; )
    {
      const uint32 codepoint = utf8::next<const char*>(c, text + length);
      const Glyph* glyph = findGlyph(codepoint);
      if (!glyph)
      {
        glyph = findGlyph(0xfffd);
        if (!glyph)
          continue;
      }

      pen = round(pen);

      if (all(greaterThan(glyph->size, vec2(0.f))))
      {
        const Rect pa(pen + glyph->bearing - vec2(0.5f), glyph->size);
        const Rect ta(glyph->offset + vec2(0.5f), glyph->size);

        m_vertices[vertexCount + 0].texcoord = ta.position;
        m_vertices[vertexCount + 0].position = pa.position;
        m_vertices[vertexCount + 1].texcoord = ta.position + vec2(ta.size.x, 0.f);
        m_vertices[vertexCount + 1].position = pa.position + vec2(pa.size.x, 0.f);
        m_vertices[vertexCount + 2].texcoord = ta.position + ta.size;
        m_vertices[vertexCount + 2].position = pa.position + pa.size;

        m_vertices[vertexCount + 3] = m_vertices[vertexCount + 2];
        m_vertices[vertexCount + 4].texcoord = ta.position + vec2(0.f, ta.size.y);
        m_vertices[vertexCount + 4].position = pa.position + vec2(0.f, pa.size.y);
        m_vertices[vertexCount + 5] = m_vertices[vertexCount + 0];

        vertexCount += 6;
      }

      pen += vec2(glyph->advance, 0.f);
    }
  }

  if (!vertexCount)
    return;

  GL::VertexRange range = m_pool->allocate(vertexCount, Vertex2ft2fv::format);
  if (range.isEmpty())
  {
    logError("Failed to allocate vertices for text drawing");
    return;
  }

  range.copyFrom(&m_vertices[0]);

  m_pass.setUniformState(m_colorIndex, color);
  m_pass.apply();

  m_pool->context().render(GL::PrimitiveRange(GL::TRIANGLE_LIST, range));
}

Rect Font::boundsOf(const char* text)
{
  vec2 pen;
  Rect bounds;
  const size_t length = std::strlen(text);

  for (const char* c = text;  *c != '\0'; )
  {
    const uint32 codepoint = utf8::next<const char*>(c, text + length);
    if (const Glyph* glyph = findGlyph(codepoint))
    {
      bounds.envelop(Rect(glyph->bearing + pen, glyph->size));
      pen = round(pen + vec2(glyph->advance, 0.f));
    }
  }

  bounds.envelop(pen);
  return bounds;
}

std::vector<Rect> Font::layoutOf(const char* text)
{
  vec2 pen;
  const size_t length = std::strlen(text);

  std::vector<Rect> layout;
  layout.reserve(length);

  for (const char* c = text;  *c != '\0'; )
  {
    const uint32 codepoint = utf8::next<const char*>(c, text + length);
    if (const Glyph* glyph = findGlyph(codepoint))
    {
      layout.push_back(Rect(glyph->bearing + pen, glyph->size));
      pen = round(pen + vec2(glyph->advance, 0.f));
    }
  }

  return layout;
}

Ref<Font> Font::create(const ResourceInfo& info,
                       VertexPool& pool,
                       Face& face,
                       uint height)
{
  Ref<Font> font(new Font(info, pool));
  if (!font->init(face, height))
    return nullptr;

  return font;
}

Ref<Font> Font::read(VertexPool& pool, const String& name)
{
  FontReader reader(pool);
  return reader.read(name);
}

Font::Font(const ResourceInfo& info, VertexPool& pool):
  Resource(info),
  m_pool(&pool)
{
}

bool Font::init(Face& face, uint height)
{
  GL::Context& context = m_pool->context();

  m_face = &face;

  m_scale = face.scale(height);

  m_width  = ceil(face.width(m_scale));
  m_height = ceil(face.height(m_scale));

  m_leading = ceil(face.leading(m_scale));

  m_ascender  = ceil(face.ascender(m_scale));
  m_descender = ceil(face.descender(m_scale));

  if (uint(m_width) + 1 > context.limits().maxTextureSize ||
      uint(m_height) + 1 > context.limits().maxTextureSize)
  {
    logError("Font %s is too large for texture size limits", name().c_str());
    return false;
  }

  // Create render pass
  {
    Ref<GL::Program> program = GL::Program::read(context,
                                                 "wendy/RenderFont.vs",
                                                 "wendy/RenderFont.fs");
    if (!program)
    {
      logError("Failed to read program for font %s", name().c_str());
      return false;
    }

    GL::ProgramInterface interface;
    interface.addSampler("glyphs", GL::SAMPLER_RECT);
    interface.addUniform("color", GL::UNIFORM_VEC4);
    interface.addAttributes(Vertex2ft2fv::format);

    if (!interface.matches(*program, true))
    {
      logError("Program %s for font %s does not conform to the required interface",
               program->name().c_str(),
               name().c_str());
      return false;
    }

    m_pass.setProgram(program);
    m_pass.setMultisampling(false);
    m_pass.setDepthTesting(false);
    m_pass.setDepthWriting(false);
    m_pass.setBlendFactors(GL::BLEND_SRC_ALPHA, GL::BLEND_ONE_MINUS_SRC_ALPHA);
    m_pass.setUniformState("color", vec4(1.f));

    m_colorIndex = m_pass.uniformStateIndex("color");
  }

  m_position = ivec2(1);

  if (!addGlyphTextureRow())
    return false;

  return true;
}

const Font::Glyph* Font::addGlyph(uint32 codepoint)
{
  const int index = m_face->indexForCodePoint(codepoint);
  if (!index)
    return nullptr;

  m_glyphs.push_back(Glyph());
  Glyph& glyph = m_glyphs.back();

  glyph.codepoint = codepoint;
  glyph.advance = ceil(m_face->advance(index, m_scale));
  glyph.bearing = ceil(m_face->bearing(index, m_scale));

  if (Ref<Image> image = m_face->glyph(index, m_scale))
  {
    if (m_position.x + image->width() + 1 > m_texture->width())
    {
      m_position.x = 1;
      m_position.y += int(m_height) + 1;

      if (m_position.y + image->height() + 1 > m_texture->height())
      {
        if (!addGlyphTextureRow())
          return nullptr;
      }
    }

    if (!m_texture->image().copyFrom(*image, m_position.x, m_position.y))
    {
      logError("Failed to copy glyph image data for font %s",
                name().c_str());
      return nullptr;
    }

    glyph.offset = vec2(m_position);
    glyph.size = vec2(image->width(), image->height());

    m_position.x += image->width() + 1;
  }

  return &glyph;
}

const Font::Glyph* Font::findGlyph(uint32 codepoint)
{
  auto glyph = std::find(m_glyphs.begin(), m_glyphs.end(), codepoint);
  if (glyph == m_glyphs.end())
    return addGlyph(codepoint);

  return &(*glyph);
}

bool Font::addGlyphTextureRow()
{
  GL::Context& context = m_pool->context();

  Ref<Image> glyphs;
  uint textureWidth = context.limits().maxTextureSize;
  uint textureHeight = 0;

  if (m_texture)
  {
    textureHeight = m_texture->height();
    glyphs = m_texture->image().data();
  }

  textureHeight += uint(m_height) + 1;

  if (textureHeight > context.limits().maxTextureSize)
  {
    logError("Glyph texture for font %s is full", name().c_str());
    return false;
  }

  GL::TextureData data(PixelFormat::L8, textureWidth, textureHeight);
  GL::TextureParams params(GL::TEXTURE_RECT, GL::TF_NONE);

  m_texture = GL::Texture::create(cache(), context, params, data);
  if (!m_texture)
  {
    logError("Failed to create glyph texture for font %s", name().c_str());
    return false;
  }

  if (glyphs)
    m_texture->image().copyFrom(*glyphs);

  m_texture->setFilterMode(GL::FILTER_NEAREST);

  m_pass.setSamplerState("glyphs", m_texture);
  return true;
}

///////////////////////////////////////////////////////////////////////

FontReader::FontReader(VertexPool& pool):
  ResourceReader<Font>(pool.context().cache()),
  m_pool(&pool)
{
}

Ref<Font> FontReader::read(const String& name, const Path& path)
{
  std::ifstream stream(path.name().c_str());
  if (stream.fail())
  {
    logError("Failed to open font %s", name.c_str());
    return nullptr;
  }

  pugi::xml_document document;

  const pugi::xml_parse_result result = document.load(stream);
  if (!result)
  {
    logError("Failed to load font %s: %s",
             name.c_str(),
             result.description());
    return nullptr;
  }

  pugi::xml_node root = document.child("font");
  if (!root || root.attribute("version").as_uint() != FONT_XML_VERSION)
  {
    logError("Font file format mismatch in %s", name.c_str());
    return nullptr;
  }

  const String faceName(root.attribute("face").value());
  if (faceName.empty())
  {
    logError("No typeface specified for font %s", faceName.c_str());
    return nullptr;
  }

  const uint height = root.attribute("height").as_uint();

  Ref<Face> face = Face::read(cache, faceName);
  if (!face)
    return nullptr;

  return Font::create(ResourceInfo(cache, name, path), *m_pool, *face, height);
}

///////////////////////////////////////////////////////////////////////

  } /*namespace render*/
} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
