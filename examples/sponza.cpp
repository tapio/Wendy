
#include <wendy/Wendy.h>

#include <cstdlib>
#include <sstream>

using namespace wendy;

namespace
{

struct Entity: public RefObject {
  Ptr<btRigidBody> body;
  scene::ModelNode* model; // Deleted by scene::Graph
  Entity(): RefObject() {}
  void syncModelFromBody()
  {
    model->setLocalTransform(bullet::convert(body->getCenterOfMassTransform()));
  }
};

class Demo : public Trackable, public input::Target
{
public:
  Demo();
  ~Demo();
  bool init();
  void run();
private:
  void onKeyPressed(input::Key key, bool pressed);
  void onButtonClicked(input::Button button, bool clicked);
  void onCursorMoved(const ivec2& position);
  ResourceCache cache;
  input::SpectatorController controller;
  Ptr<render::GeometryPool> pool;
  Ref<render::Camera> camera;
  Ptr<deferred::Renderer> renderer;
  Ptr<UI::Drawer> drawer;
  Ref<debug::Interface> interface;
  scene::Graph graph;
  scene::CameraNode* cameraNode;
  scene::LightNode* lightNode;
  Timer timer;
  Time currentTime;
  ivec2 lastPosition;
  Ptr<btCollisionShape> sponzaShape;
  Ptr<btCollisionShape> cameraShape;
  Ptr<btCollisionShape> vaseShape;
  std::vector< Ref<Entity> > entities;
  Ptr<btRigidBody> cameraBody;
  Ptr<btBroadphaseInterface> broadphase;
  Ptr<btCollisionDispatcher> dispatcher;
  Ptr<btConstraintSolver> solver;
  Ptr<btDefaultCollisionConfiguration> collisionConfiguration;
  Ptr<btDiscreteDynamicsWorld> dynamicsWorld;
  bool quitting;
  bool drawdebug;
};

Demo::Demo():
  quitting(false),
  drawdebug(true),
  currentTime(0.0),
  cameraNode(NULL)
{
}

Demo::~Demo()
{
  graph.destroyRootNodes();

  drawer = NULL;
  camera = NULL;
  renderer = NULL;
  pool = NULL;

  input::Context::destroySingleton();
  GL::Context::destroySingleton();
}

bool Demo::init()
{
  const char* mediaPath = std::getenv("WENDY_MEDIA_DIR");
  if (!mediaPath)
    mediaPath = WENDY_MEDIA_DIR;

  if (!cache.addSearchPath(Path(mediaPath)))
    return false;

  if (!cache.addSearchPath(Path(mediaPath) + "sponza"))
    return false;

  GL::WindowConfig wc;
  wc.title = "Sponza Atrium";
  wc.resizable = false;

  if (!GL::Context::createSingleton(cache, wc))
    return false;

  GL::Context* context = GL::Context::getSingleton();

  const unsigned int width = context->getDefaultFramebuffer().getWidth();
  const unsigned int height = context->getDefaultFramebuffer().getHeight();

  if (!input::Context::createSingleton(*context))
    return false;

  pool = new render::GeometryPool(*context);

  renderer = deferred::Renderer::create(*pool, deferred::Config(width, height));
  if (!renderer)
    return false;

  Ref<render::Model> sponzaModel = render::Model::read(*context, Path("sponza.model"));
  if (!sponzaModel)
    return false;

  scene::ModelNode* sponzaNode = new scene::ModelNode();
  sponzaNode->setModel(sponzaModel);
  graph.addRootNode(*sponzaNode);

  // Collision configuration contains default setup for memory, collision setup
  collisionConfiguration = new btDefaultCollisionConfiguration();
  // Use the default collision dispatcher. For parallel processing you can use a diffent dispatcher (see Extras/BulletMultiThreaded)
  dispatcher = new btCollisionDispatcher(collisionConfiguration);
  broadphase = new btDbvtBroadphase();
  // The default constraint solver. For parallel processing you can use a different solver (see Extras/BulletMultiThreaded)
  btSequentialImpulseConstraintSolver* sol = new btSequentialImpulseConstraintSolver;
  solver = sol;
  dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
  dynamicsWorld->setGravity(btVector3(0,-10.f,0));

  {
    MeshReader reader(cache);
    Ref<Mesh> sponzaObj = reader.read(Path("sponza.obj"));
    sponzaShape = new btBvhTriangleMeshShape(bullet::convert(*sponzaObj, false), true);

    btTransform transform;
    transform.setIdentity();
    btScalar mass(0.); // Static object
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, NULL, sponzaShape);
    btRigidBody* body = new btRigidBody(rbInfo);
    dynamicsWorld->addRigidBody(body);
  }

  Ref<render::Model> model = render::Model::read(*context, Path("vase_round.model"));
  if (!model)
    return false;

  //MeshReader vaseReader(cache);
  //Ref<Mesh> vaseObj = vaseReader.read(Path("vase_round.obj"));
  //vaseShape = new btBvhTriangleMeshShape(bullet::convert(*vaseObj), true);
  vaseShape = new btBoxShape(btVector3(2., 3.5, 2.));

  btScalar vaseMass(100.f);
  btVector3 vaseLocalInertia(0,0,0);
  vaseShape->calculateLocalInertia(vaseMass, vaseLocalInertia);

  const int numVases = 6;
  float vasePos[numVases * 2] = {
    -61.5f, -21.f,
    -25.0f, -21.f,
     12.0f, -21.f,
     48.5f,  14.f,
     12.0f,  14.f,
    -25.0f,  14.f
  };
  for (int i = 0; i < numVases; ++i)
  {
    entities.push_back(new Entity());
    Entity& entity = *entities.back();
    entity.model = new scene::ModelNode();
    entity.model->setModel(model);
    entity.model->setLocalPosition(vec3(vasePos[i*2], 3.7f, vasePos[i*2+1]));
    graph.addRootNode(*entity.model);

    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(bullet::convert(entity.model->getLocalTransform().position));

    btDefaultMotionState* motionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(vaseMass, motionState, vaseShape, vaseLocalInertia);
    rbInfo.m_friction = 0.9f;
    entity.body = new btRigidBody(rbInfo);
    dynamicsWorld->addRigidBody(entity.body);
  }

  cameraShape = new btSphereShape(btScalar(3.));
  {
    btTransform transform;
    transform.setIdentity();
    transform.setOrigin(btVector3(0.f, 10.f, 0.f));

    btScalar mass(1.f);
    btVector3 localInertia(0,0,0);
    cameraShape->calculateLocalInertia(mass, localInertia);

    btDefaultMotionState* motionState = new btDefaultMotionState(transform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, cameraShape, localInertia);
    cameraBody = new btRigidBody(rbInfo);
    cameraBody->setSleepingThresholds(0., 0.);
    dynamicsWorld->addRigidBody(cameraBody);
  }

  camera = new render::Camera();
  camera->setFOV(60.f);
  camera->setNearZ(0.9f);
  camera->setFarZ(500.f);
  camera->setAspectRatio((float) width / height);

  cameraNode = new scene::CameraNode();
  cameraNode->setCamera(camera);
  graph.addRootNode(*cameraNode);

  render::LightRef light = new render::Light();
  light->setType(render::Light::POINT);
  light->setRadius(100.f);

  lightNode = new scene::LightNode();
  lightNode->setLight(light);
  graph.addRootNode(*lightNode);

  drawer = UI::Drawer::create(*pool);
  if (!drawer)
    return false;

  timer.start();

  {
    input::Context* context = input::Context::getSingleton();

    interface = new debug::Interface(*context, *drawer);

    context->setTarget(this);
    context->captureCursor();

    lastPosition =  context->getCursorPosition();
  }

  controller.setSpeed(25.f);
  controller.setPosition(bullet::convert(cameraBody->getCenterOfMassPosition()));

  return true;
}

void Demo::run()
{
  render::Scene scene(*pool, render::Technique::DEFERRED);
  scene.setAmbientIntensity(vec3(0.2f, 0.2f, 0.2f));

  GL::Context& context = pool->getContext();
  GL::Stats stats;
  context.setStats(&stats);

  do
  {
    const Time deltaTime = timer.getTime() - currentTime;
    currentTime += deltaTime;

    // Calculate and send current velocity from controller to Bullet
    vec3 p0 = controller.getTransform().position;
    controller.update(deltaTime);
    vec3 p1 = controller.getTransform().position;
    vec3 vel = float(1.0f / deltaTime) * (p1 - p0);
    cameraBody->setLinearVelocity(bullet::convert(vel));
    // Simulate
    dynamicsWorld->stepSimulation(deltaTime);
    // Set controller position according to simualtion results
    controller.setPosition(bullet::convert(cameraBody->getCenterOfMassPosition()));

    for (size_t i = 0; i < entities.size(); ++i)
      entities[i]->syncModelFromBody();

    lightNode->setLocalPosition(vec3(0.f, sinf((float) currentTime) * 40.f + 45.f, 0.f));
    cameraNode->setLocalTransform(controller.getTransform());

    graph.update();
    graph.enqueue(scene, *camera);

    context.clearDepthBuffer();
    context.clearColorBuffer();

    renderer->render(scene, *camera);

    scene.removeOperations();
    scene.detachLights();

    interface->update();
    if (drawdebug)
      interface->draw();
  }
  while (!quitting && context.update());
}

void Demo::onKeyPressed(input::Key key, bool pressed)
{
  controller.inputKeyPress(key, pressed);

  if (pressed)
  {
    switch (key)
    {
      case input::KEY_ESCAPE:
        quitting = true;
        return;
      case input::KEY_F1:
        drawdebug = !drawdebug;
        return;
    }
  }
}

void Demo::onButtonClicked(input::Button button, bool clicked)
{
  controller.inputButtonClick(button, clicked);

  if (clicked)
  {
    if (button == input::BUTTON_LEFT)
    {
      // TODO: Write screenshot to disk
    }
  }
}

void Demo::onCursorMoved(const ivec2& position)
{
  controller.inputCursorOffset(position - lastPosition);
  lastPosition = position;
}

} /*namespace*/

int main()
{
  Ptr<Demo> demo(new Demo());
  if (!demo->init())
  {
    logError("Failed to initialize demo");
    std::exit(EXIT_FAILURE);
  }

  demo->run();
  demo = NULL;

  std::exit(EXIT_SUCCESS);
}

