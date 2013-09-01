///////////////////////////////////////////////////////////////////////
// Wendy default renderer
// Copyright (c) 2011 Camilla Berglund <elmindreda@elmindreda.org>
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

#include <wendy/Config.h>

#include <wendy/GLTexture.h>
#include <wendy/GLBuffer.h>
#include <wendy/GLProgram.h>
#include <wendy/GLContext.h>

#include <wendy/RenderPool.h>
#include <wendy/RenderSystem.h>

///////////////////////////////////////////////////////////////////////

namespace wendy
{
  namespace render
  {

///////////////////////////////////////////////////////////////////////

ResourceCache& System::cache() const
{
  return m_pool->context().cache();
}

GL::Context& System::context() const
{
  return m_pool->context();
}

VertexPool& System::vertexPool() const
{
  return *m_pool;
}

System::Type System::type() const
{
  return m_type;
}

System::System(VertexPool& pool, Type type):
  m_pool(&pool),
  m_type(type)
{
}

System::System(const System& source)
{
  panic("Render systems may not be copied");
}

System& System::operator = (const System& source)
{
  panic("Render systems may not be assigned");
}

///////////////////////////////////////////////////////////////////////

  } /*namespace render*/
} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
