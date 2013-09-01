///////////////////////////////////////////////////////////////////////
// Wendy OpenAL library
// Copyright (c) 2007 Camilla Berglund <elmindreda@elmindreda.org>
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
#include <wendy/Core.h>

#include <wendy/ALContext.h>

#include <internal/ALHelper.h>

#include <glm/gtc/type_ptr.hpp>

#include <al.h>
#include <alc.h>

///////////////////////////////////////////////////////////////////////

namespace wendy
{
  namespace AL
  {

///////////////////////////////////////////////////////////////////////

Context::~Context()
{
  if (m_handle)
  {
    alcMakeContextCurrent(NULL);
    alcDestroyContext((ALCcontext*) m_handle);
  }

  if (m_device)
    alcCloseDevice((ALCdevice*) m_device);
}

void Context::setListenerPosition(const vec3& newPosition)
{
  if (m_listenerPosition != newPosition)
  {
    m_listenerPosition = newPosition;
    alListenerfv(AL_POSITION, value_ptr(m_listenerPosition));

#if WENDY_DEBUG
    checkAL("Failed to set listener position");
#endif
  }
}

void Context::setListenerVelocity(const vec3& newVelocity)
{
  if (m_listenerVelocity != newVelocity)
  {
    m_listenerVelocity = newVelocity;
    alListenerfv(AL_VELOCITY, value_ptr(m_listenerVelocity));

#if WENDY_DEBUG
    checkAL("Failed to set listener velocity");
#endif
  }
}

void Context::setListenerRotation(const quat& newRotation)
{
  if (m_listenerRotation != newRotation)
  {
    m_listenerRotation = newRotation;

    const vec3 at = newRotation * vec3(0.f, 0.f, -1.f);
    const vec3 up = newRotation * vec3(0.f, 1.f, 0.f);

    const float orientation[] = { at.x, at.y, at.z, up.x, up.y, up.z };

    alListenerfv(AL_ORIENTATION, orientation);

#if WENDY_DEBUG
    checkAL("Failed to set listener rotation");
#endif
  }
}

void Context::setListenerGain(float newGain)
{
  if (m_listenerGain != newGain)
  {
    m_listenerGain = newGain;
    alListenerfv(AL_GAIN, &m_listenerGain);

#if WENDY_DEBUG
    checkAL("Failed to set listener gain");
#endif
  }
}

bool Context::createSingleton(ResourceCache& cache)
{
  Ptr<Context> context(new Context(cache));
  if (!context->init())
    return false;

  set(context.detachObject());
  return true;
}

Context::Context(ResourceCache& cache):
  m_cache(cache),
  m_device(NULL),
  m_handle(NULL),
  m_listenerGain(1.f)
{
}

Context::Context(const Context& source):
  m_cache(source.m_cache)
{
  panic("OpenAL contexts may not be copied");
}

bool Context::init()
{
  m_device = alcOpenDevice(NULL);
  if (!m_device)
  {
    checkALC("Failed to open OpenAL device");
    return false;
  }

  m_handle = alcCreateContext((ALCdevice*) m_device, NULL);
  if (!m_handle)
  {
    checkALC("Failed to create OpenAL context");
    return false;
  }

  if (!alcMakeContextCurrent((ALCcontext*) m_handle))
  {
    checkALC("Failed to make OpenAL context current");
    return false;
  }

  log("OpenAL context version %s created",
      (const char*) alGetString(AL_VERSION));

  log("OpenAL context renderer is %s by %s",
      (const char*) alGetString(AL_RENDERER),
      (const char*) alGetString(AL_VENDOR));

  log("OpenAL context uses device %s",
      (const char*) alcGetString((ALCdevice*) m_device, ALC_DEVICE_SPECIFIER));

  return true;
}

Context& Context::operator = (const Context& source)
{
  panic("OpenAL contexts may not be assigned");
}

///////////////////////////////////////////////////////////////////////

  } /*namespace AL*/
} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
