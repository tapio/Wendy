//////////////////////////////////////////////////////////////////////
// Wendy user interface library
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

#include <wendy/Core.hpp>
#include <wendy/Bimap.hpp>

#include <wendy/UIDrawer.hpp>

#include <pugixml.hpp>

///////////////////////////////////////////////////////////////////////

namespace wendy
{
  namespace UI
  {

///////////////////////////////////////////////////////////////////////

namespace
{

Bimap<String, WidgetState> widgetStateMap;

class ElementVertex
{
public:
  inline void set(const vec2& newSizeScale, const vec2& newOffsetScale, const vec2& newTexScale)
  {
    sizeScale = newSizeScale;
    offsetScale = newOffsetScale;
    texScale = newTexScale;
  }
  vec2 sizeScale;
  vec2 offsetScale;
  vec2 texScale;
  static VertexFormat format;
};

VertexFormat ElementVertex::format("2f:vSizeScale 2f:vOffsetScale 2f:vTexScale");

const uint THEME_XML_VERSION = 3;

} /*namespace*/

///////////////////////////////////////////////////////////////////////

Alignment::Alignment(HorzAlignment horizontal, VertAlignment vertical):
  horizontal(horizontal),
  vertical(vertical)
{
}

void Alignment::set(HorzAlignment newHorizontal, VertAlignment newVertical)
{
  horizontal = newHorizontal;
  vertical = newVertical;
}

///////////////////////////////////////////////////////////////////////

Theme::Theme(const ResourceInfo& info):
  Resource(info)
{
}

Ref<Theme> Theme::read(render::VertexPool& pool, const String& name)
{
  ThemeReader reader(pool);
  return reader.read(name);
}

///////////////////////////////////////////////////////////////////////

ThemeReader::ThemeReader(render::VertexPool& initPool):
  ResourceReader<Theme>(initPool.context().cache()),
  pool(&initPool)
{
  if (widgetStateMap.isEmpty())
  {
    widgetStateMap["disabled"] = STATE_DISABLED;
    widgetStateMap["normal"] = STATE_NORMAL;
    widgetStateMap["active"] = STATE_ACTIVE;
    widgetStateMap["selected"] = STATE_SELECTED;
  }
}

Ref<Theme> ThemeReader::read(const String& name, const Path& path)
{
  std::ifstream stream(path.name().c_str());
  if (stream.fail())
  {
    logError("Failed to open animation %s", name.c_str());
    return nullptr;
  }

  pugi::xml_document document;

  const pugi::xml_parse_result result = document.load(stream);
  if (!result)
  {
    logError("Failed to load UI theme %s: %s",
             name.c_str(),
             result.description());
    return nullptr;
  }

  pugi::xml_node root = document.child("theme");
  if (!root || root.attribute("version").as_uint() != THEME_XML_VERSION)
  {
    logError("UI theme file format mismatch in %s", name.c_str());
    return nullptr;
  }

  Ref<Theme> theme = new Theme(ResourceInfo(cache, name, path));

  const String imageName(root.attribute("image").value());
  if (imageName.empty())
  {
    logError("No image specified for UI theme %s", name.c_str());
    return nullptr;
  }

  const GL::TextureParams params(GL::TEXTURE_RECT, GL::TF_NONE);

  theme->texture = GL::Texture::read(pool->context(), params, imageName);
  if (!theme->texture)
  {
    logError("Failed to create texture for UI theme %s", name.c_str());
    return nullptr;
  }

  const String fontName(root.attribute("font").value());
  if (fontName.empty())
  {
    logError("Font for UI theme %s is empty", name.c_str());
    return nullptr;
  }

  theme->font = render::Font::read(*pool, fontName);
  if (!theme->font)
  {
    logError("Failed to load font for UI theme %s", name.c_str());
    return nullptr;
  }

  const vec3 scale(1.f / 255.f);

  for (auto sn : root.children())
  {
    if (!widgetStateMap.hasKey(sn.name()))
    {
      logError("Unknown widget state %s in UI theme %s",
               sn.name(),
               name.c_str());
      return nullptr;
    }

    WidgetState state = widgetStateMap[sn.name()];

    if (pugi::xml_node node = sn.child("text"))
      theme->textColors[state] = vec3Cast(node.attribute("color").value()) * scale;

    if (pugi::xml_node node = sn.child("back"))
      theme->backColors[state] = vec3Cast(node.attribute("color").value()) * scale;

    if (pugi::xml_node node = sn.child("caret"))
      theme->caretColors[state] = vec3Cast(node.attribute("color").value()) * scale;

    if (pugi::xml_node node = sn.child("button"))
      theme->buttonElements[state] = rectCast(node.attribute("area").value());

    if (pugi::xml_node node = sn.child("handle"))
      theme->handleElements[state] = rectCast(node.attribute("area").value());

    if (pugi::xml_node node = sn.child("frame"))
      theme->frameElements[state] = rectCast(node.attribute("area").value());

    if (pugi::xml_node node = sn.child("well"))
      theme->wellElements[state] = rectCast(node.attribute("area").value());

    if (pugi::xml_node node = sn.child("tab"))
      theme->tabElements[state] = rectCast(node.attribute("area").value());
  }

  return theme;
}

///////////////////////////////////////////////////////////////////////

void Drawer::begin()
{
  GL::Framebuffer& framebuffer = m_context.currentFramebuffer();
  const uint width = framebuffer.width();
  const uint height = framebuffer.height();

  m_context.setCurrentSharedProgramState(m_state);
  m_context.setViewportArea(Recti(0, 0, width, height));
  m_context.setScissorArea(Recti(0, 0, width, height));

  m_state->setOrthoProjectionMatrix(float(width), float(height));
}

void Drawer::end()
{
  m_context.setCurrentSharedProgramState(nullptr);
}

bool Drawer::pushClipArea(const Rect& area)
{
  if (!m_clipAreaStack.push(area))
    return false;

  const Rect& total = m_clipAreaStack.total();

  m_context.setScissorArea(Recti(ivec2(total.position), ivec2(total.size)));
  return true;
}

void Drawer::popClipArea()
{
  m_clipAreaStack.pop();

  GL::Framebuffer& framebuffer = m_context.currentFramebuffer();

  Recti area;

  if (m_clipAreaStack.isEmpty())
    area.set(0, 0, framebuffer.width(), framebuffer.height());
  else
  {
    const Rect& total = m_clipAreaStack.total();
    area.set(ivec2(total.position), ivec2(total.size));
  }

  m_context.setScissorArea(area);
}

void Drawer::drawPoint(const vec2& point, const vec4& color)
{
  Vertex2fv vertex;
  vertex.position = point;

  GL::VertexRange range = m_pool->allocate(1, Vertex2fv::format);
  if (range.isEmpty())
    return;

  range.copyFrom(&vertex);
  setDrawingState(color, true);
  m_context.render(GL::PrimitiveRange(GL::POINT_LIST, range));
}

void Drawer::drawLine(vec2 start, vec2 end, const vec4& color)
{
  Vertex2fv vertices[2];
  vertices[0].position = start;
  vertices[1].position = end;

  GL::VertexRange range = m_pool->allocate(2, Vertex2fv::format);
  if (range.isEmpty())
    return;

  range.copyFrom(vertices);
  setDrawingState(color, true);
  m_context.render(GL::PrimitiveRange(GL::LINE_LIST, range));
}

void Drawer::drawRectangle(const Rect& rectangle, const vec4& color)
{
  float minX, minY, maxX, maxY;
  rectangle.bounds(minX, minY, maxX, maxY);

  if (maxX - minX < 1.f || maxY - minY < 1.f)
    return;

  Vertex2fv vertices[4];
  vertices[0].position = vec2(minX, minY);
  vertices[1].position = vec2(maxX, minY);
  vertices[2].position = vec2(maxX, maxY);
  vertices[3].position = vec2(minX, maxY);

  GL::VertexRange range = m_pool->allocate(4, Vertex2fv::format);
  if (range.isEmpty())
    return;

  range.copyFrom(vertices);
  setDrawingState(color, true);
  m_context.render(GL::PrimitiveRange(GL::LINE_LOOP, range));
}

void Drawer::fillRectangle(const Rect& rectangle, const vec4& color)
{
  float minX, minY, maxX, maxY;
  rectangle.bounds(minX, minY, maxX, maxY);

  if (maxX - minX < 1.f || maxY - minY < 1.f)
    return;

  Vertex2fv vertices[4];
  vertices[0].position = vec2(minX, minY);
  vertices[1].position = vec2(maxX, minY);
  vertices[2].position = vec2(maxX, maxY);
  vertices[3].position = vec2(minX, maxY);

  GL::VertexRange range = m_pool->allocate(4, Vertex2fv::format);
  if (range.isEmpty())
    return;

  range.copyFrom(vertices);
  setDrawingState(color, false);
  m_context.render(GL::PrimitiveRange(GL::TRIANGLE_FAN, range));
}

void Drawer::blitTexture(const Rect& area, GL::Texture& texture)
{
  float minX, minY, maxX, maxY;
  area.bounds(minX, minY, maxX, maxY);

  if (maxX - minX < 1.f || maxY - minY < 1.f)
    return;

  Vertex2ft2fv vertices[4];
  vertices[0].texcoord = vec2(0.f, 0.f);
  vertices[0].position = vec2(minX, minY);
  vertices[1].texcoord = vec2(1.f, 0.f);
  vertices[1].position = vec2(maxX, minY);
  vertices[2].texcoord = vec2(1.f, 1.f);
  vertices[2].position = vec2(maxX, maxY);
  vertices[3].texcoord = vec2(0.f, 1.f);
  vertices[3].position = vec2(minX, maxY);

  GL::VertexRange range = m_pool->allocate(4, Vertex2ft2fv::format);
  if (range.isEmpty())
    return;

  range.copyFrom(vertices);

  if (texture.format().semantic() == PixelFormat::RGBA)
    m_blitPass.setBlendFactors(GL::BLEND_SRC_ALPHA, GL::BLEND_ONE_MINUS_SRC_ALPHA);
  else
    m_blitPass.setBlendFactors(GL::BLEND_ONE, GL::BLEND_ZERO);

  m_blitPass.setSamplerState("image", &texture);
  m_blitPass.apply();

  m_context.render(GL::PrimitiveRange(GL::TRIANGLE_FAN, range));

  m_blitPass.setSamplerState("image", nullptr);
}

void Drawer::drawText(const Rect& area,
                      const char* text,
                      Alignment alignment,
                      vec3 color)
{
  const Rect bounds = m_font->boundsOf(text);

  vec2 pen;

  switch (alignment.horizontal)
  {
    case LEFT_ALIGNED:
      pen.x = area.position.x - bounds.position.x;
      break;
    case CENTERED_ON_X:
      pen.x = area.center().x - bounds.center().x;
      break;
    case RIGHT_ALIGNED:
      pen.x = (area.position.x + area.size.x) -
              (bounds.position.x + bounds.size.x);
      break;
    default:
      panic("Invalid horizontal alignment");
  }

  switch (alignment.vertical)
  {
    case BOTTOM_ALIGNED:
      pen.y = area.position.y - m_font->descender();
      break;
    case CENTERED_ON_Y:
      pen.y = area.center().y - m_font->descender() - m_font->height() / 2.f;
      break;
    case TOP_ALIGNED:
      pen.y = area.position.y + area.size.y - m_font->ascender();
      break;
    default:
      panic("Invalid vertical alignment");
  }

  m_font->drawText(pen, vec4(color, 1.f), text);
}

void Drawer::drawText(const Rect& area,
                      const char* text,
                      Alignment alignment,
                      WidgetState state)
{
  drawText(area, text, alignment, m_theme->textColors[state]);
}

void Drawer::drawWell(const Rect& area, WidgetState state)
{
  drawElement(area, m_theme->wellElements[state]);
}

void Drawer::drawFrame(const Rect& area, WidgetState state)
{
  drawElement(area, m_theme->frameElements[state]);
}

void Drawer::drawHandle(const Rect& area, WidgetState state)
{
  drawElement(area, m_theme->handleElements[state]);
}

void Drawer::drawButton(const Rect& area, WidgetState state, const char* text)
{
  drawElement(area, m_theme->buttonElements[state]);

  if (state == STATE_SELECTED)
  {
    const Rect textArea(area.position.x + 2.f,
                        area.position.y,
                        area.size.x - 2.f,
                        area.size.y - 2.f);

    drawText(textArea, text, Alignment(), state);
  }
  else
    drawText(area, text, Alignment(), state);
}

void Drawer::drawTab(const Rect& area, WidgetState state, const char* text)
{
  drawElement(area, m_theme->tabElements[state]);
  drawText(area, text, Alignment(), state);
}

const Theme& Drawer::theme() const
{
  return *m_theme;
}

GL::Context& Drawer::context()
{
  return m_context;
}

render::VertexPool& Drawer::vertexPool()
{
  return *m_pool;
}

render::Font& Drawer::currentFont()
{
  return *m_font;
}

void Drawer::setCurrentFont(render::Font* newFont)
{
  if (newFont)
    m_font = newFont;
  else
    m_font = m_theme->font;
}

float Drawer::currentEM() const
{
  return m_font->height();
}

Ref<Drawer> Drawer::create(render::VertexPool& pool)
{
  Ptr<Drawer> drawer(new Drawer(pool));
  if (!drawer->init())
    return nullptr;

  return drawer.detachObject();
}

Drawer::Drawer(render::VertexPool& pool):
  m_context(pool.context()),
  m_pool(&pool)
{
}

bool Drawer::init()
{
  m_state = new render::SharedProgramState();
  if (!m_state->reserveSupported(m_context))
    return false;

  // Set up element geometry
  {
    m_vertexBuffer = GL::VertexBuffer::create(m_context, 16, ElementVertex::format, GL::USAGE_STATIC);
    if (!m_vertexBuffer)
      return false;

    ElementVertex vertices[16];

    // These are scaling factors used when rendering UI widget elements
    //
    // There are three kinds:
    //  * The size scale, which when multiplied by the screen space size
    //    of the element places vertices in the closest corner
    //  * The offset scale, which when multiplied by the texture space size of
    //    the element pulls the vertices defining its inner edges towards the
    //    center of the element
    //  * The texture coordinate scale, which when multiplied by the texture
    //    space size of the element becomes the relative texture coordinate
    //    of that vertex
    //
    // This allows rendering of UI elements by changing only four uniforms: the
    // position and size of the element in screen and texture space.

    vertices[0x0].set(vec2(0.f, 0.f), vec2(  0.f,   0.f), vec2( 0.f,  0.f));
    vertices[0x1].set(vec2(0.f, 0.f), vec2( 0.5f,   0.f), vec2(0.5f,  0.f));
    vertices[0x2].set(vec2(1.f, 0.f), vec2(-0.5f,   0.f), vec2(0.5f,  0.f));
    vertices[0x3].set(vec2(1.f, 0.f), vec2(  0.f,   0.f), vec2( 1.f,  0.f));

    vertices[0x4].set(vec2(0.f, 0.f), vec2(  0.f,  0.5f), vec2( 0.f, 0.5f));
    vertices[0x5].set(vec2(0.f, 0.f), vec2( 0.5f,  0.5f), vec2(0.5f, 0.5f));
    vertices[0x6].set(vec2(1.f, 0.f), vec2(-0.5f,  0.5f), vec2(0.5f, 0.5f));
    vertices[0x7].set(vec2(1.f, 0.f), vec2(  0.f,  0.5f), vec2( 1.f, 0.5f));

    vertices[0x8].set(vec2(0.f, 1.f), vec2(  0.f, -0.5f), vec2( 0.f, 0.5f));
    vertices[0x9].set(vec2(0.f, 1.f), vec2( 0.5f, -0.5f), vec2(0.5f, 0.5f));
    vertices[0xa].set(vec2(1.f, 1.f), vec2(-0.5f, -0.5f), vec2(0.5f, 0.5f));
    vertices[0xb].set(vec2(1.f, 1.f), vec2(  0.f, -0.5f), vec2( 1.f, 0.5f));

    vertices[0xc].set(vec2(0.f, 1.f), vec2(  0.f,   0.f), vec2( 0.f,  1.f));
    vertices[0xd].set(vec2(0.f, 1.f), vec2( 0.5f,   0.f), vec2(0.5f,  1.f));
    vertices[0xe].set(vec2(1.f, 1.f), vec2(-0.5f,   0.f), vec2(0.5f,  1.f));
    vertices[0xf].set(vec2(1.f, 1.f), vec2(  0.f,   0.f), vec2( 1.f,  1.f));

    m_vertexBuffer->copyFrom(vertices, 16);

    m_indexBuffer = GL::IndexBuffer::create(m_context, 54, GL::INDEX_UINT8, GL::USAGE_STATIC);
    if (!m_indexBuffer)
      return false;

    uint8 indices[54];
    size_t index = 0;

    // This is a perfectly normal indexed triangle list using the vertices above

    for (int y = 0;  y < 3;  y++)
    {
      for (int x = 0;  x < 3;  x++)
      {
        indices[index++] = x + y * 4;
        indices[index++] = (x + 1) + (y + 1) * 4;
        indices[index++] = x + (y + 1) * 4;

        indices[index++] = x + y * 4;
        indices[index++] = (x + 1) + y * 4;
        indices[index++] = (x + 1) + (y + 1) * 4;
      }
    }

    m_indexBuffer->copyFrom(indices, 54);

    m_range = GL::PrimitiveRange(GL::TRIANGLE_LIST,
                                 *m_vertexBuffer,
                                 *m_indexBuffer);
  }

  // Load default theme
  {
    const String themeName("wendy/UIDefault.theme");

    m_theme = Theme::read(*m_pool, themeName);
    if (!m_theme)
    {
      logError("Failed to load default UI theme %s", themeName.c_str());
      return false;
    }

    m_font = m_theme->font;
  }

  // Set up solid pass
  {
    Ref<GL::Program> program = GL::Program::read(m_context,
                                                 "wendy/UIElement.vs",
                                                 "wendy/UIElement.fs");
    if (!program)
    {
      logError("Failed to load UI element program");
      return false;
    }

    GL::ProgramInterface interface;
    interface.addUniform("elementPos", GL::UNIFORM_VEC2);
    interface.addUniform("elementSize", GL::UNIFORM_VEC2);
    interface.addUniform("texPos", GL::UNIFORM_VEC2);
    interface.addUniform("texSize", GL::UNIFORM_VEC2);
    interface.addSampler("image", GL::SAMPLER_RECT);
    interface.addAttributes(ElementVertex::format);

    if (!interface.matches(*program, true))
    {
      logError("UI element program %s does not conform to the required interface",
               program->name().c_str());
      return false;
    }

    m_elementPass.setProgram(program);
    m_elementPass.setDepthTesting(false);
    m_elementPass.setDepthWriting(false);
    m_elementPass.setSamplerState("image", m_theme->texture);
    m_elementPass.setBlendFactors(GL::BLEND_SRC_ALPHA, GL::BLEND_ONE_MINUS_SRC_ALPHA);
    m_elementPass.setMultisampling(false);

    m_elementPosIndex = m_elementPass.uniformStateIndex("elementPos");
    m_elementSizeIndex = m_elementPass.uniformStateIndex("elementSize");
    m_texPosIndex = m_elementPass.uniformStateIndex("texPos");
    m_texSizeIndex = m_elementPass.uniformStateIndex("texSize");
  }

  // Set up solid pass
  {
    Ref<GL::Program> program = GL::Program::read(m_context,
                                                 "wendy/UIDrawSolid.vs",
                                                 "wendy/UIDrawSolid.fs");
    if (!program)
    {
      logError("Failed to load UI drawing shader program");
      return false;
    }

    GL::ProgramInterface interface;
    interface.addUniform("color", GL::UNIFORM_VEC4);
    interface.addAttributes(Vertex2fv::format);

    if (!interface.matches(*program, true))
    {
      logError("UI drawing shader program %s does not conform to the required interface",
               program->name().c_str());
      return false;
    }

    m_drawPass.setProgram(program);
    m_drawPass.setCullMode(GL::CULL_NONE);
    m_drawPass.setDepthTesting(false);
    m_drawPass.setDepthWriting(false);
    m_drawPass.setMultisampling(false);
  }

  // Set up blitting pass
  {
    Ref<GL::Program> program = GL::Program::read(m_context,
                                                 "wendy/UIDrawMapped.vs",
                                                 "wendy/UIDrawMapped.fs");
    if (!program)
    {
      logError("Failed to load UI blitting shader program");
      return false;
    }

    GL::ProgramInterface interface;
    interface.addSampler("image", GL::SAMPLER_2D);
    interface.addAttributes(Vertex2ft2fv::format);

    if (!interface.matches(*program, true))
    {
      logError("UI blitting shader program %s does not conform to the required interface",
               program->name().c_str());
      return false;
    }

    m_blitPass.setProgram(program);
    m_blitPass.setCullMode(GL::CULL_NONE);
    m_blitPass.setDepthTesting(false);
    m_blitPass.setDepthWriting(false);
    m_blitPass.setMultisampling(false);
  }

  return true;
}

void Drawer::drawElement(const Rect& area, const Rect& mapping)
{
  m_elementPass.setUniformState(m_elementPosIndex, area.position);
  m_elementPass.setUniformState(m_elementSizeIndex, area.size);
  m_elementPass.setUniformState(m_texPosIndex, mapping.position);
  m_elementPass.setUniformState(m_texSizeIndex, mapping.size);
  m_elementPass.apply();

  m_context.render(m_range);
}

void Drawer::setDrawingState(const vec4& color, bool wireframe)
{
  m_drawPass.setUniformState("color", color);

  if (color.a == 1.f)
    m_drawPass.setBlendFactors(GL::BLEND_ONE, GL::BLEND_ZERO);
  else
    m_drawPass.setBlendFactors(GL::BLEND_SRC_ALPHA, GL::BLEND_ONE_MINUS_SRC_ALPHA);

  m_drawPass.setWireframe(wireframe);
  m_drawPass.apply();
}

///////////////////////////////////////////////////////////////////////

  } /*namespace UI*/
} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
