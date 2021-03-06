///////////////////////////////////////////////////////////////////////
// Wendy OpenGL library
// Copyright (c) 2012 Camilla Berglund <elmindreda@elmindreda.org>
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
#include <wendy/Path.hpp>
#include <wendy/Resource.hpp>

#include <internal/GLParser.hpp>

#include <algorithm>
#include <fstream>

#include <cstring>

///////////////////////////////////////////////////////////////////////

namespace wendy
{
  namespace GL
  {

///////////////////////////////////////////////////////////////////////

Preprocessor::Preprocessor(ResourceCache& initCache):
  cache(initCache)
{
}

void Preprocessor::parse(const char* name)
{
  const Path path = cache.findFile(name);
  if (path.isEmpty())
  {
    if (files.empty())
      logError("Failed to find shader %s", name);
    else
    {
      logError("%s:%u: Failed to find shader %s",
               files.back().name,
               files.back().line,
               name);
    }

    throw Exception("Failed to find shader file");
  }

  std::ifstream stream(path.name().c_str());
  if (stream.fail())
  {
    if (files.empty())
      logError("Failed to open shader file %s", path.name().c_str());
    else
    {
      logError("%s:%u: Failed to open shader file %s",
               files.back().name,
               files.back().line,
               path.name().c_str());
    }

    throw Exception("Failed to open shader file");
  }

  paths.push_back(path);

  String text;

  stream.seekg(0, std::ios::end);
  text.resize((size_t) stream.tellg());
  stream.seekg(0, std::ios::beg);
  stream.read(&text[0], text.size());
  stream.close();

  parse(name, text.c_str());
}

void Preprocessor::parse(const char* name, const char* text)
{
  if (std::find(names.begin(), names.end(), name) != names.end())
    return;

  files.push_back(File(name, text));
  names.push_back(name);

  list += format("( file %u: %s )\n", (uint) names.size(), name);

  output.reserve(output.size() + std::strlen(text));
  appendToOutput(format("#line 0 %u /* entering %s */\n",
                        (uint) files.size(),
                        files.back().name).c_str());

  while (hasMore())
  {
    if (isMultiLineComment())
      parseMultiLineComment();
    else if (isSingleLineComment())
      parseSingleLineComment();
    else if (isNewLine())
      parseNewLine();
    else if (isWhitespace())
      parseWhitespace();
    else if (isCommand())
      parseCommand();
    else
    {
      advance(1);
      appendToOutput();
      setFirstOnLine(false);
    }
  }

  files.pop_back();

  if (!files.empty())
  {
    appendToOutput(format("\n#line %u %u /* returning to %s */",
                          files.back().line,
                          (uint) files.size(),
                          files.back().name).c_str());
  }
}

const String& Preprocessor::getOutput() const
{
  return output;
}

bool Preprocessor::hasVersion() const
{
  return !version.empty();
}

const String& Preprocessor::getVersion() const
{
  return version;
}

const String& Preprocessor::getNameList() const
{
  return list;
}

const PathList& Preprocessor::getPaths() const
{
  return paths;
}

void Preprocessor::addLine()
{
  files.back().line++;
}

void Preprocessor::advance(size_t count)
{
  files.back().pos += count;
}

void Preprocessor::discard()
{
  files.back().base = files.back().pos;
}

void Preprocessor::appendToOutput()
{
  File& file = files.back();
  output.append(file.base, file.pos);
  file.base = file.pos;
}

void Preprocessor::appendToOutput(const char* text)
{
  output.append(text);
}

char Preprocessor::c(ptrdiff_t offset) const
{
  return files.back().pos[offset];
}

void Preprocessor::passWhitespace()
{
  while (isWhitespace())
    advance(1);
}

void Preprocessor::parseWhitespace()
{
  passWhitespace();
  appendToOutput();
}

void Preprocessor::parseNewLine()
{
  if (c(0) == '\r' && c(1) == '\n')
    advance(2);
  else
    advance(1);

  addLine();
  setFirstOnLine(true);
  appendToOutput();
}

void Preprocessor::parseSingleLineComment()
{
  advance(2);
  setFirstOnLine(false);

  while (hasMore())
  {
    if (isNewLine())
      break;
    else
      advance(1);
  }

  appendToOutput();
}

void Preprocessor::parseMultiLineComment()
{
  advance(2);
  setFirstOnLine(false);

  for (;;)
  {
    if (!hasMore())
    {
      logError("%s:%u: Unexpected end of file in multi-line comment",
               files.back().name,
               files.back().line);

      throw Exception("Unexpected end of file in multi-line comment");
    }

    if (c(0) == '*' && c(1) == '/')
    {
      advance(2);
      break;
    }
    else if (isNewLine())
      parseNewLine();
    else
      advance(1);
  }

  appendToOutput();
}

String Preprocessor::passNumber()
{
  if (!isNumeric())
  {
    logError("%s:%u: Expected number", files.back().name, files.back().line);

    throw Exception("Expected number");
  }

  String number;

  while (isNumeric())
  {
    number.append(1, c(0));
    advance(1);
  }

  return number;
}

String Preprocessor::passIdentifier()
{
  if (!isAlpha())
  {
    logError("%s:%u: Expected identifier",
             files.back().name,
             files.back().line);

    throw Exception("Expected identifier");
  }

  String identifier;

  while (isAlphaNumeric())
  {
    identifier.append(1, c(0));
    advance(1);
  }

  return identifier;
}

String Preprocessor::passShaderName()
{
  char terminator;
  if (c(0) == '<')
    terminator = '>';
  else if (c(0) == '\"')
    terminator = '\"';
  else
  {
    logError("%s:%u: Expected < or \" after #include",
             files.back().name,
             files.back().line);

    throw Exception("Expected < or \" after #include");
  }

  advance(1);

  String name;

  while (hasMore())
  {
    if (!hasMore() || isNewLine())
    {
      logError("%s:%u: Expected %c after shader name",
               files.back().name,
               files.back().line,
               terminator);

      throw Exception("Expected < or \" after shader name");
    }

    if (c(0) == terminator)
    {
      advance(1);
      break;
    }
    else
    {
      name.append(1, c(0));
      advance(1);
    }
  }

  return name;
}

void Preprocessor::parseCommand()
{
  advance(1);
  setFirstOnLine(false);
  passWhitespace();

  const String command = passIdentifier();
  if (command == "include")
  {
    passWhitespace();
    const String name = passShaderName();
    discard();
    parse(name.c_str());
  }
  else if (command == "version")
  {
    if (!version.empty())
    {
      logError("%s:%u: Duplicate #version directive",
               files.back().name,
               files.back().line);

      throw Exception("Duplicate #version directive");
    }

    passWhitespace();
    version = passNumber();
    discard();
  }

  while (hasMore())
  {
    if (isNewLine() || isSingleLineComment() || isMultiLineComment())
      break;

    advance(1);
  }

  appendToOutput();
}

bool Preprocessor::hasMore() const
{
  return c(0) != '\0';
}

bool Preprocessor::isNewLine() const
{
  return c(0) == '\r' || c(0) == '\n';
}

bool Preprocessor::isMultiLineComment() const
{
  return c(0) == '/' && c(1) == '*';
}

bool Preprocessor::isSingleLineComment() const
{
  return c(0) == '/' && c(1) == '/';
}

bool Preprocessor::isWhitespace() const
{
  return c(0) == ' ' || c(0) == '\t';
}

bool Preprocessor::isCommand() const
{
  return isFirstOnLine() && c(0) == '#';
}

bool Preprocessor::isAlpha() const
{
  return (c(0) >= 'a' && c(0) <= 'z') || (c(0) >= 'A' && c(0) <= 'Z');
}

bool Preprocessor::isNumeric() const
{
  return c(0) >= '0' && c(0) <= '9';
}

bool Preprocessor::isAlphaNumeric() const
{
  return isAlpha() || isNumeric();
}

bool Preprocessor::isFirstOnLine() const
{
  return files.back().first;
}

void Preprocessor::setFirstOnLine(bool newState)
{
  files.back().first = newState;
}

///////////////////////////////////////////////////////////////////////

Preprocessor::File::File(const char* name, const char* text):
  name(name),
  text(text),
  base(text),
  pos(text),
  line(1),
  first(true)
{
}

///////////////////////////////////////////////////////////////////////

  } /*namespace GL*/
} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
