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
#ifndef WENDY_FONTIO_H
#define WENDY_FONTIO_H
///////////////////////////////////////////////////////////////////////

#include <list>

///////////////////////////////////////////////////////////////////////

namespace wendy
{

///////////////////////////////////////////////////////////////////////

typedef ResourceCodec<Font> FontCodec;

///////////////////////////////////////////////////////////////////////

class FontCodecXML : public FontCodec, public XML::Codec
{
public:
  FontCodecXML(void);
  Font* read(const Path& path, const String& name = "");
  Font* read(Stream& stream, const String& name = "");
  bool write(const Path& path, const Font& font);
  bool write(Stream& stream, const Font& font);
private:
  bool onBeginElement(const String& name);
  bool onEndElement(const String& name);
  Ptr<Font> font;
  String fontName;
};

///////////////////////////////////////////////////////////////////////

} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
#endif /*WENDY_FONTIO_H*/
///////////////////////////////////////////////////////////////////////