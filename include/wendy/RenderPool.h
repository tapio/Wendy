///////////////////////////////////////////////////////////////////////
// Wendy default renderer
// Copyright (c) 2010 Camilla Berglund <elmindreda@elmindreda.org>
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
#ifndef WENDY_RENDERPOOL_H
#define WENDY_RENDERPOOL_H
///////////////////////////////////////////////////////////////////////

#include <wendy/Core.h>
#include <wendy/Signal.h>

#include <wendy/GLTexture.h>
#include <wendy/GLBuffer.h>

///////////////////////////////////////////////////////////////////////

namespace wendy
{
  namespace render
  {

///////////////////////////////////////////////////////////////////////

/*! @brief Vertex pool.
 *  @ingroup renderer
 */
class VertexPool : public Trackable, public RefObject
{
public:
  /*! Allocates a range of temporary vertices of the specified format.
   *  @param[out] range The newly allocated vertex range.
   *  @param[in] count The number of vertices to allocate.
   *  @param[in] format The format of vertices to allocate.
   *  @return @c true if the allocation succeeded, or @c false if an
   *  error occurred.
   *
   *  @remarks The allocated vertex range is only valid until the end of the
   *  current frame.
   */
  bool allocateVertices(GL::VertexRange& range,
                        uint count,
                        const VertexFormat& format);
  /*! @return The OpenGL context used by this pool.
   */
  GL::Context& context() const { return m_context; }
  /*! Creates a vertex pool.
   *  @param[in] context The OpenGL context to be used.
   *  @param[in] granularity The desired allocation granularity.
   *  @return The newly created vertex pool.
   */
  static Ref<VertexPool> create(GL::Context& context, size_t granularity = 1024);
private:
  VertexPool(GL::Context& context);
  bool init(size_t granularity);
  /*! @internal
   */
  struct Slot
  {
    Ref<GL::VertexBuffer> buffer;
    uint available;
  };
  void onFrame();
  GL::Context& m_context;
  size_t m_granularity;
  std::vector<Slot> m_slots;
};

///////////////////////////////////////////////////////////////////////

  } /*namespace render*/
} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
#endif /*WENDY_RENDERPOOL_H*/
///////////////////////////////////////////////////////////////////////
