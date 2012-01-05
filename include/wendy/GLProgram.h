///////////////////////////////////////////////////////////////////////
// Wendy OpenGL library
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
#ifndef WENDY_GLPROGRAM_H
#define WENDY_GLPROGRAM_H
///////////////////////////////////////////////////////////////////////

#include <wendy/Core.h>
#include <wendy/Vertex.h>
#include <wendy/Path.h>
#include <wendy/Resource.h>

///////////////////////////////////////////////////////////////////////

namespace wendy
{
  namespace GL
  {

///////////////////////////////////////////////////////////////////////

class Context;
class Program;

///////////////////////////////////////////////////////////////////////

/*! @brief GLSL shader type enumeration.
 *  @ingroup opengl
 */
enum ShaderType
{
  VERTEX_SHADER,
  FRAGMENT_SHADER,
  GEOMETRY_SHADER,
  TESS_CONTROL_SHADER,
  TESS_EVALUATION_SHADER
};

///////////////////////////////////////////////////////////////////////

/*! @brief GLSL shader.
 *  @ingroup opengl
 */
class Shader : public Resource
{
  friend class Program;
public:
  static const int DefaultVersion = 120;
  ~Shader();
  bool isVertexShader() const;
  bool isFragmentShader() const;
  bool isGeometryShader() const;
  bool isTessControlShader() const;
  bool isTessEvaluationShader() const;
  ShaderType getType() const;
  int getVersion() const;
  Context& getContext() const;
  static Ref<Shader> create(const ResourceInfo& info,
                            Context& context,
                            ShaderType type,
                            const String& text,
                            int version = DefaultVersion);
  static Ref<Shader> read(Context& context,
                          ShaderType type,
                          const String& name,
                          int version = DefaultVersion);
private:
  Shader(const ResourceInfo& info, Context& context, ShaderType type, int version);
  bool init(const String& text);
  Context& context;
  ShaderType type;
  int version;
  unsigned int shaderID;
};

///////////////////////////////////////////////////////////////////////

/*! @brief GLSL vertex attribute type enumeration.
 *  @ingroup opengl
 */
enum AttributeType
{
  ATTRIBUTE_FLOAT,
  ATTRIBUTE_VEC2,
  ATTRIBUTE_VEC3,
  ATTRIBUTE_VEC4
};

///////////////////////////////////////////////////////////////////////

/*! @brief GLSL vertex attribute.
 *  @ingroup opengl
 */
class Attribute
{
  friend class Program;
  friend class Context;
public:
  /*! Binds this attribute to the specified stride and offset of the
   *  current vertex buffer.
   */
  void bind(size_t stride, size_t offset);
  /*! @return @c true if the name of this attribute matches the specified
   *  string, or @c false otherwise.
   */
  bool operator == (const char* string) const;
  /*! @return @c true if the type of this attribute is a single value.
   */
  bool isScalar() const;
  /*! @return @c true if the type of this attribute is a vector.
   */
  bool isVector() const;
  /*! @return The type of this attribute.
   */
  AttributeType getType() const;
  /*! @return The name of this attribute.
   */
  const String& getName() const;
  /*! @return The number of elements in this attribute.
   */
  unsigned int getElementCount() const;
  /*! @return The GLSL name of the specified attribute type.
   */
  static const char* getTypeName(AttributeType type);
private:
  AttributeType type;
  String name;
  int location;
};

///////////////////////////////////////////////////////////////////////

/*! @brief GLSL sampler uniform type enumeration.
 *  @ingroup opengl
 */
enum SamplerType
{
  SAMPLER_1D,
  SAMPLER_2D,
  SAMPLER_3D,
  SAMPLER_RECT,
  SAMPLER_CUBE
};

///////////////////////////////////////////////////////////////////////

/*! @brief GLSL sampler uniform.
 *  @ingroup opengl
 */
class Sampler
{
  friend class Program;
public:
  /*! Binds this sampler to the specified texture unit.
   */
  void bind(unsigned int unit);
  /*! @return @c true if the name of this sampler matches the specified string,
   *  or @c false otherwise.
   */
  bool operator == (const char* string) const;
  /*! @return @c true if this sampler is shared, or @c false otherwise.
   *
   *  @remarks Shared samplers get their values via the currently set shared
   *  program state.
   */
  bool isShared() const;
  /*! @return The type of this sampler.
   */
  SamplerType getType() const;
  /*! @return The name of this sampler.
   */
  const String& getName() const;
  /*! @return The shared ID of this sampler, or INVALID_SHARED_STATE_ID if
   *  it is not shared.
   */
  int getSharedID() const;
  /*! @return The GLSL name of the specified sampler type.
   */
  static const char* getTypeName(SamplerType type);
private:
  String name;
  SamplerType type;
  int location;
  int sharedID;
};

///////////////////////////////////////////////////////////////////////

/*! @brief GLSL non-sampler uniform type enumeration.
 *  @ingroup opengl
 */
enum UniformType
{
  UNIFORM_FLOAT,
  UNIFORM_VEC2,
  UNIFORM_VEC3,
  UNIFORM_VEC4,
  UNIFORM_MAT2,
  UNIFORM_MAT3,
  UNIFORM_MAT4
};

///////////////////////////////////////////////////////////////////////

/*! @brief GLSL program non-sampler uniform.
 *  @ingroup opengl
 */
class Uniform
{
  friend class Program;
public:
  /*! Copies a new value for this uniform from the specified address.
   *  @param[in] data The address of the value to use.
   *
   *  @remarks It is the responsibility of the caller to ensure that the source
   *  data type matches.
   */
  void copyFrom(const void* data);
  /*! @return @c true if the name of this uniform matches the specified string,
   *  or @c false otherwise.
   */
  bool operator == (const char* string) const;
  /*! @return @c true if this uniform is shared, or @c false otherwise.
   *
   *  @remarks Shared uniforms get their values via the currently set shared
   *  program state.
   */
  bool isShared() const;
  /*! @return @c true if the type of this uniform is a single value.
   */
  bool isScalar() const;
  /*! @return @c true if the type of this uniform is a vector.
   */
  bool isVector() const;
  /*! @return @c true if the type of this uniform is a matrix.
   */
  bool isMatrix() const;
  /*! @return The type of this uniform.
   */
  UniformType getType() const;
  /*! @return The name of this uniform.
   */
  const String& getName() const;
  /*! @return The number of elements in this uniform.
   */
  unsigned int getElementCount() const;
  /*! @return The shared ID of this uniform, or INVALID_SHARED_STATE_ID if
   *  it is not shared.
   */
  int getSharedID() const;
  /*! @return The GLSL name of the specified uniform type.
   */
  static const char* getTypeName(UniformType type);
private:
  String name;
  UniformType type;
  int location;
  int sharedID;
};

///////////////////////////////////////////////////////////////////////

/*! @brief GLSL program.
 *  @ingroup opengl
 */
class Program : public Resource
{
  friend class Context;
public:
  ~Program();
  Attribute* findAttribute(const char* name);
  const Attribute* findAttribute(const char* name) const;
  Sampler* findSampler(const char* name);
  const Sampler* findSampler(const char* name) const;
  Uniform* findUniform(const char* name);
  const Uniform* findUniform(const char* name) const;
  bool hasGeometryShader() const;
  bool hasTessellationShaders() const;
  unsigned int getAttributeCount() const;
  Attribute& getAttribute(unsigned int index);
  const Attribute& getAttribute(unsigned int index) const;
  unsigned int getSamplerCount() const;
  Sampler& getSampler(unsigned int index);
  const Sampler& getSampler(unsigned int index) const;
  unsigned int getUniformCount() const;
  Uniform& getUniform(unsigned int index);
  const Uniform& getUniform(unsigned int index) const;
  Context& getContext() const;
  static Ref<Program> create(const ResourceInfo& info,
                             Context& context,
                             Shader& vertexShader,
                             Shader& fragmentShader);
  static Ref<Program> create(const ResourceInfo &info,
                             Context &context,
                             Shader& vertexShader,
                             Shader& fragmentShader,
                             Shader& geometryShader);
  static Ref<Program> create(const ResourceInfo &info,
                             Context &context,
                             Shader& vertexShader,
                             Shader& fragmentShader,
                             Shader& tessCtrlShader,
                             Shader& tessEvalShader);
  static Ref<Program> create(const ResourceInfo &info,
                             Context &context,
                             Shader& vertexShader,
                             Shader& fragmentShader,
                             Shader& geometryShader,
                             Shader& tessCtrlShader,
                             Shader& tessEvalShader);
  static Ref<Program> read(Context& context,
                           const String& vertexShaderName,
                           const String& fragmentShaderName,
                           const String& geometryShaderName = "",
                           const String& tessCtrlShaderName = "",
                           const String& tessEvalShaderName = "",
                           int glslVersion = Shader::DefaultVersion);
private:
  Program(const ResourceInfo& info, Context& context);
  Program(const Program& source);
  bool init(Shader* vertexShader,
            Shader* fragmentShader,
            Shader* geometryShader = NULL,
            Shader* tessCtrlShader = NULL,
            Shader* tessEvalShader = NULL);
  bool retrieveUniforms();
  bool retrieveAttributes();
  void bind();
  void unbind();
  Program& operator = (const Program& source);
  bool isValid() const;
  String getInfoLog() const;
  typedef std::vector<Attribute> AttributeList;
  typedef std::vector<Sampler> SamplerList;
  typedef std::vector<Uniform> UniformList;
  Context& context;
  Ref<Shader> vertexShader;
  Ref<Shader> fragmentShader;
  Ref<Shader> geometryShader;
  Ref<Shader> tessCtrlShader;
  Ref<Shader> tessEvalShader;
  unsigned int programID;
  AttributeList attributes;
  SamplerList samplers;
  UniformList uniforms;
};

///////////////////////////////////////////////////////////////////////

/*! @brief GLSL program interface validator.
 *  @ingroup opengl
 */
class ProgramInterface
{
public:
  /*! Adds a sampler to this interface.
   *  @param[in] name The name of the sampler.
   *  @param[in] type The type of the sampler.
   */
  void addSampler(const char* name, SamplerType type);
  /*! Adds a sampler to this interface.
   *  @param[in] name The name of the sampler.
   *  @param[in] type The type of the sampler.
   */
  void addUniform(const char* name, UniformType type);
  /*! Adds a sampler to this interface.
   *  @param[in] name The name of the sampler.
   *  @param[in] type The type of the sampler.
   */
  void addAttribute(const char* name, AttributeType type);
  /*! Adds attributes for all components of the specified vertex format.
   *  @param[in] format The vertex format to use.
   */
  void addAttributes(const VertexFormat& format);
  /*! Checks whether all samplers, uniforms and attributes of this interface
   *  are exposed by the specified program and are of the correct types.
   *  @param[in] program The program to match this interface against.
   *  @param[in] verbose @c true to %log errors and warnings, or @c false for
   *  silent matching.
   *  @return @c true if this entire interface is exposed by the specified
   *  program, or @c false otherwise.
   */
  bool matches(const Program& program, bool verbose = false) const;
  /*! Checks whether components matching all attributes of this interface are
   *  present in the specified vertex format and are of the correct types.
   *  @param[in] format The vertex format to match this interface against.
   *  @param[in] verbose @c true to %log errors and warnings, or @c false for
   *  silent matching.
   *  @return @c true if all attributes of this interface are exposed by the
   *  specified vertex format, or @c false otherwise.
   */
  bool matches(const VertexFormat& format, bool verbose = false) const;
private:
  typedef std::vector<std::pair<String, SamplerType> > SamplerList;
  typedef std::vector<std::pair<String, UniformType> > UniformList;
  typedef std::vector<std::pair<String, AttributeType> > AttributeList;
  SamplerList samplers;
  UniformList uniforms;
  AttributeList attributes;
};

///////////////////////////////////////////////////////////////////////

  } /*namespace GL*/
} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
#endif /*WENDY_GLPROGRAM_H*/
///////////////////////////////////////////////////////////////////////
