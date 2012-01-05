
#include <wendy/Wendy.h>

#include <cstdlib>
#include <sstream>

#include <glm/gtx/quaternion.hpp>
#include <glm/gtx/constants.hpp>

using namespace wendy;

namespace
{

class Test : public Trackable
{
public:
  ~Test();
  bool init();
  void run();
private:
  void onContextResized(unsigned int width, unsigned int height);
  ResourceCache cache;
  GL::Stats stats;
  input::MayaCamera controller;
  Ref<render::GeometryPool> pool;
  Ref<render::Camera> camera;
  Ref<forward::Renderer> renderer;
  Ref<UI::Drawer> drawer;
  Ref<debug::Interface> interface;
  scene::Graph graph;
  scene::CameraNode* cameraNode;
};

Test::~Test()
{
  graph.destroyRootNodes();

  interface = NULL;
  drawer = NULL;
  renderer = NULL;
  pool = NULL;

  input::Context::destroySingleton();
  GL::Context::destroySingleton();
}

bool Test::init()
{
  const char* mediaPath = std::getenv("WENDY_MEDIA_DIR");
  if (!mediaPath)
    mediaPath = WENDY_MEDIA_DIR;

  if (!cache.addSearchPath(Path(mediaPath)))
    return false;

  GL::ContextConfig cc;
  cc.version = GL::Version(4,1);

  if (!GL::Context::createSingleton(cache, GL::WindowConfig("OpenGL 4 Hardware Tessellation"), cc))
    return false;

  GL::Context* context = GL::Context::getSingleton();
  context->getResizedSignal().connect(*this, &Test::onContextResized);
  context->setStats(&stats);

  if (!input::Context::createSingleton(*context))
    return false;

  input::Context::getSingleton()->setTarget(&controller);

  pool = render::GeometryPool::create(*context);

  renderer = forward::Renderer::create(forward::Config(*pool));
  if (!renderer)
  {
    logError("Failed to create forward renderer");
    return false;
  }

  const String modelName("cube_tessellation.model");

  Ref<render::Model> model = render::Model::read(*renderer, modelName);
  if (!model)
  {
    logError("Failed to load model \'%s\'", modelName.c_str());
    return false;
  }

  RandomRange angle(0.f, pi<float>() * 2.f);
  RandomVolume axis(vec3(-1.f), vec3(1.f));
  RandomVolume position(vec3(-2.f), vec3(2.f));

  for (size_t i = 0;  i < 20;  i++)
  {
    scene::ModelNode* modelNode = new scene::ModelNode();
    modelNode->setModel(model);
    modelNode->setLocalPosition(position());
    modelNode->setLocalRotation(angleAxis(degrees(angle()), normalize(axis())));
    graph.addRootNode(*modelNode);
  }

  GL::Framebuffer& framebuffer = context->getCurrentFramebuffer();

  camera = new render::Camera();
  camera->setFOV(60.f);
  camera->setAspectRatio((float) framebuffer.getWidth() / framebuffer.getHeight());

  cameraNode = new scene::CameraNode();
  cameraNode->setCamera(camera);
  cameraNode->setLocalPosition(vec3(0.f, 0.f, model->getBoundingSphere().radius * 3.f));
  graph.addRootNode(*cameraNode);

  drawer = UI::Drawer::create(*pool);
  if (!drawer)
    return false;
  interface = new debug::Interface(*input::Context::getSingleton(), *drawer);

  return true;
}

void Test::run()
{
  render::Scene scene(*pool);
  GL::Context& context = pool->getContext();

  do
  {
    cameraNode->setLocalTransform(controller.getTransform());
    graph.update();

    context.clearBuffers();

    graph.enqueue(scene, *camera);
    renderer->render(scene, *camera);

    scene.removeOperations();
    scene.detachLights();

    interface->update();
    interface->draw();
  }
  while (context.update());
}

void Test::onContextResized(unsigned int width, unsigned int height)
{
  GL::Context* context = GL::Context::getSingleton();
  context->setViewportArea(Recti(0, 0, width, height));

  camera->setAspectRatio(float(width) / float(height));
}

} /*namespace*/

int main()
{
  Ptr<Test> test(new Test());
  if (!test->init())
  {
    logError("Failed to initialize test");
    std::exit(EXIT_FAILURE);
  }

  test->run();
  test = NULL;

  std::exit(EXIT_SUCCESS);
}

