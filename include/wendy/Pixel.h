///////////////////////////////////////////////////////////////////////
// Wendy core library
// Copyright (c) 2009 Camilla Berglund <elmindreda@elmindreda.org>
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
#ifndef WENDY_PIXEL_H
#define WENDY_PIXEL_H
///////////////////////////////////////////////////////////////////////

namespace wendy
{

///////////////////////////////////////////////////////////////////////

/*! @brief %Pixel format descriptor.
 *
 *  All formats are at least byte aligned, although their channels may not be.
 */
class PixelFormat
{
public:
  /*! Pixel format semantic enumeration.
   */
  enum Semantic
  {
    NONE,
    R,
    RG,
    RGB,
    RGBA,
    DEPTH,
  };
  /*! Pixel format component type enumeration.
   */
  enum Type
  {
    DUMMY,
    UINT8,
    UINT16,
    UINT24,
    UINT32,
    FLOAT32,
  };
  /*! Default constructor.
   *  @param[in] semantic The desired semantic of this pixel format.
   *  @param[in] type The desired type of this pixel format.
   */
  PixelFormat(Semantic semantic = NONE, Type type = DUMMY);
  /*! Constructor. Creates components according to the specified specification.
   *  @param specification The specification of the desired format.
   *  @remarks This will throw if the specification is syntactically malformed.
   */
  explicit PixelFormat(const String& specification);
  /*! @return @c true if the pixel formats are equal, @c false otherwise.
   */
  bool operator == (const PixelFormat& other) const;
  /*! @return @c true if the pixel formats are not equal, @c false otherwise.
   */
  bool operator != (const PixelFormat& other) const;
  /*! @return The size, in bytes, of a pixel in this pixel format.
   */
  size_t getSize(void) const;
  /*! @return The channel data type of this pixel format.
   */
  Type getType(void) const;
  /*! @return The channel semantic of this pixel format.
   */
  Semantic getSemantic(void) const;
  /*! @return The number of components in this pixel format.
   */
  unsigned int getChannelCount(void) const;
  /*! @return A string representation of this pixel format.
   */
  String asString(void) const;
  static const PixelFormat R8;
  static const PixelFormat R16;
  static const PixelFormat R32F;
  static const PixelFormat RG8;
  static const PixelFormat RG16;
  static const PixelFormat RG32F;
  static const PixelFormat RGB8;
  static const PixelFormat RGB16;
  static const PixelFormat RGB32F;
  static const PixelFormat RGBA8;
  static const PixelFormat RGBA16;
  static const PixelFormat RGBA32F;
  static const PixelFormat DEPTH16;
  static const PixelFormat DEPTH24;
  static const PixelFormat DEPTH32;
  static const PixelFormat DEPTH32F;
private:
  Semantic semantic;
  Type type;
};

///////////////////////////////////////////////////////////////////////

/*! @brief Pixel transform interface.
 */
class PixelTransform
{
public:
  virtual bool supports(const PixelFormat& targetFormat,
                        const PixelFormat& sourceFormat) = 0;
  virtual void convert(void* target,
                       const PixelFormat& targetFormat,
                       const void* source,
                       const PixelFormat& sourceFormat,
                       size_t count) = 0;
};

///////////////////////////////////////////////////////////////////////

} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
#endif /*WENDY_PIXEL_H*/
///////////////////////////////////////////////////////////////////////