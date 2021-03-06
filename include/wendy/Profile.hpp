///////////////////////////////////////////////////////////////////////
// Wendy core library
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
#ifndef WENDY_PROFILE_HPP
#define WENDY_PROFILE_HPP
///////////////////////////////////////////////////////////////////////

namespace wendy
{

///////////////////////////////////////////////////////////////////////

class Profile;

///////////////////////////////////////////////////////////////////////

class ProfileNode
{
  friend class Profile;
public:
  typedef std::vector<ProfileNode> List;
  bool operator == (const char* string) const;
  Time duration() const { return m_duration; }
  uint callCount() const { return m_calls; }
  const String& name() const { return m_name; }
  const List& children() const { return m_children; }
private:
  explicit ProfileNode(const char* name);
  ProfileNode* findChild(const char* name);
  String m_name;
  Time m_duration;
  List m_children;
  uint m_calls;
};

///////////////////////////////////////////////////////////////////////

class Profile
{
public:
  void beginFrame();
  void endFrame();
  void beginNode(const char* name);
  void endNode();
  const ProfileNode& rootNode() const { return m_root; }
  static Profile* currentNode() { return m_current; }
private:
  Profile(const Profile&) = delete;
  void beginNode(ProfileNode& node);
  Profile& operator = (const Profile&) = delete;
  static void resetNode(ProfileNode& node);
  typedef std::vector<ProfileNode*> Stack;
  ProfileNode m_root;
  Stack m_stack;
  Timer m_timer;
  static Profile* m_current;
};

///////////////////////////////////////////////////////////////////////

class ProfileNodeCall
{
public:
  ProfileNodeCall(const char* name):
    m_profile(Profile::currentNode())
  {
    if (m_profile)
      m_profile->beginNode(name);
  }
  ~ProfileNodeCall()
  {
    if (m_profile)
      m_profile->endNode();
  }
private:
  Profile* m_profile;
};

///////////////////////////////////////////////////////////////////////

} /*namespace wendy*/

///////////////////////////////////////////////////////////////////////
#endif /*WENDY_PROFILE_HPP*/
///////////////////////////////////////////////////////////////////////
