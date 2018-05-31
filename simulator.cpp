#include "al/core/app/al_DistributedApp.hpp"
#include "agent_managers.hpp"
#include "al/core/math/al_Quat.hpp"
#include "al/core/spatial/al_Pose.hpp"
#include "common.hpp"
#include "helper.hpp"
#include "location_managers.hpp"
//#include "al/util/al_AlloSphereAudioSpatializer.hpp"
#include "al/core/sound/al_AudioScene.hpp"
#include "al/core/sound/al_StereoPanner.hpp"
#include "al/core/sound/al_Vbap.hpp"
#include "al/util/al_AlloSphereSpeakerLayout.hpp"
//#include "al/util/al_Simulator.hpp"
//#include "alloGLV/al_ControlGLV.hpp"
//#include "GLV/glv.h"

using namespace al;
using namespace std;

// Mengyu Chen, 2018
// mengyuchen@ucsb.edu

#define BLOCK_SIZE (256)
#define SAMPLE_RATE 44100
#define MAXIMUM_NUMBER_OF_SOUND_SOURCES (100)

// Vbap audio spatializer
// static AudioScene  v_scene(BLOCK_SIZE); //to not confuse with scene()
// static SpeakerLayout speakerLayout;
//     //Vbap* panner;
// static Spatializer *panner;
// //static StereoPanner *panner;
// static Listener *listener;
// static SoundSource *source[15];
// static SoundSource *sourceWorker[75];

struct RendererThings {
  // meshes
  Mesh miner_body;
  Mesh miner_resource;
  int miner_nv;
  Mesh worker_body;
  int worker_nv;
  Mesh capitalist_body;
  int capitalist_nv;

  // needs to be vector, cause everyone is different
  vector<Mesh> factory_bodies;
  vector<Mesh> factory_bodies_wires;

  Mesh metro_body;
  Mesh metro_body_wires;
  int metro_nv;
  Mesh resource_body;
  Mesh resource_body_wires;
  int resource_nv;
  Mesh packed_resource;
  Mesh packed_resource_wire;
  int packed_resource_nv;

  vector<Line> capitalist_lines;
  vector<Line> worker_lines;
  vector<Line> miner_lines;

  float phase;

  int renderModeSwitch = 1;

  void init(const State &state) {
    // shader
    phase = 0;

    // initialize mesh
    // miner body
    miner_nv = addCone(miner_body, MapValue(5000, 0, 100000.0, 1, 3),
                       Vec3f(0, 0, MapValue(5000, 0, 100000.0, 1, 3) * 3));
    for (int i = 0; i < miner_nv; ++i) {
      float f = float(i) / miner_nv;
      miner_body.color(HSV(f * 0.1 + 0.2, 1, 1));
    }
    miner_body.decompress();
    miner_body.generateNormals();
    addCube(miner_resource, 4);
    miner_resource.generateNormals();

    // miner's packed resource
    packed_resource_nv = addCube(packed_resource);
    addCube(packed_resource_wire);
    packed_resource_wire.primitive(Mesh::LINE_LOOP);
    for (int i = 0; i < packed_resource_nv; ++i) {
      float f = float(i) / packed_resource_nv;
      packed_resource.color(HSV(f * 0.1 + 0.2, 1, 1));
      packed_resource_wire.color(HSV(f * 0.1 + 0.2, 1, 1));
    }
    packed_resource.decompress();
    packed_resource.generateNormals();

    // worker body
    worker_nv = addCone(worker_body, MapValue(4000, 0, 100000.0, 1, 3),
                        Vec3f(0, 0, MapValue(4000, 0, 100000.0, 1, 3) * 3));
    for (int i = 0; i < worker_nv; ++i) {
      float f = float(i) / worker_nv;
      worker_body.color(HSV(f * 0.2 + 0.4, 1, 1));
    }
    worker_body.decompress();
    worker_body.generateNormals();

    // capitalist body
    capitalist_nv =
        addTetrahedron(capitalist_body, MapValue(25000, 0, 100000.0, 3, 6));
    for (int i = 0; i < capitalist_nv; ++i) {
      float f = float(i) / capitalist_nv;
      capitalist_body.color(HSV(f * 0.1, 0.9, 1));
    }
    capitalist_body.decompress();
    capitalist_body.generateNormals();

    // metro block
    metro_nv = addCube(metro_body);
    addCube(metro_body_wires);
    metro_body_wires.primitive(Mesh::LINE_LOOP);
    for (int i = 0; i < metro_nv; ++i) {
      float f = float(i) / metro_nv;
      metro_body.color(HSV(0.06 + f * 0.1, 0.8, 1));
      metro_body_wires.color(HSV(0.06 + f * 0.1, 0.8, 1));
    }
    metro_body.decompress();
    metro_body.generateNormals();

    // factory shape
    factory_bodies.resize(state.numCapitalists);
    factory_bodies_wires.resize(state.numCapitalists);
    for (int i = 0; i < state.numCapitalists; i++) {
      addTorus(factory_bodies[i], 0.2, 1.6, r_int(3, 6), r_int(3, 6));
      addTorus(factory_bodies_wires[i], 0.2 * 1.2, 1.6 * 1.2, r_int(3, 6),
               r_int(3, 6));
      factory_bodies_wires[i].primitive(Mesh::LINE_LOOP);
      factory_bodies[i].generateNormals();
    }

    // resource points
    resource_nv = addDodecahedron(resource_body, 0.3);
    addDodecahedron(resource_body_wires, 0.3 * 1.2);
    resource_body_wires.primitive(Mesh::LINE_LOOP);
    for (int i = 0; i < resource_nv; ++i) {
      float f = float(i) / resource_nv;
      resource_body.color(HSV(f * 0.1, 1, 1));
      resource_body_wires.color(HSV(f * 0.1, 1, 1));
    }
    resource_body.decompress();
    resource_body.generateNormals();

    // lines
    capitalist_lines.resize(15);
    worker_lines.resize(75);
    miner_lines.resize(100);
  }

  void draw(Graphics &g, const State &state) {
    for (unsigned i = 0; i < state.numMiners; i++) {
      if (!state.miner_bankrupted[i]) {
        g.pushMatrix();
        g.translate(state.miner_pose[i].pos());
        g.rotate(state.miner_pose[i].quat());
        g.scale(state.miner_scale[i]);
        g.meshColor();
        g.draw(miner_body);
        if (state.miner_fullpack[i]) {
          g.pushMatrix();
          g.translate(0, 0, 6);
          g.scale(1);
          g.draw(packed_resource);
          g.translate(0, 0, -1.5);
          g.scale(2);
          g.draw(packed_resource_wire);
          g.popMatrix();
        }
        g.popMatrix();
        g.color(0.6, 1, 0.6);
        g.draw(miner_lines[i]);
      }
    }
    // draw workers
    for (unsigned i = 0; i < state.numWorkers; i++) {
      if (!state.worker_bankrupted[i]) {
        g.pushMatrix();
        g.translate(state.worker_pose[i].pos());
        g.rotate(state.worker_pose[i].quat());
        g.scale(state.worker_scale[i]);
        g.meshColor();
        g.draw(worker_body);
        g.popMatrix();
        // lines
        g.color(0.4, 0.65, 1);
        g.draw(worker_lines[i]);
      }
    }
    // draw capitalists, factories, and buildings
    for (unsigned i = 0; i < state.numCapitalists; i++) {
      // capitalists
      if (!state.capitalist_bankrupted[i]) {
        g.pushMatrix();
        g.translate(state.capitalist_pose[i].pos());
        g.rotate(state.capitalist_pose[i].quat());
        g.scale(state.capitalist_scale[i]);
        g.meshColor();
        g.draw(capitalist_body);
        g.popMatrix();
        // lines
        g.color(1, 0.55, 0.4);
        g.draw(capitalist_lines[i]);
      }
      // factories
      g.pushMatrix();
      g.translate(state.factory_pos[i]);
      g.rotate(state.factory_facing_center[i]);
      g.rotate(state.factory_rotation_angle[i], 0, 0, 1);
      g.scale(state.factory_size[i]);
      g.color(state.factory_color[i]);
      g.draw(factory_bodies[i]);
      g.draw(factory_bodies_wires[i]);
      g.popMatrix();
      // metropolis
      g.pushMatrix();
      g.rotate(state.metro_rotate_angle);
      g.pushMatrix();
      g.translate(state.building_pos[i]);
      g.scale(state.building_size[i]);
      g.scale(state.building_size[i], state.building_size[i],
              state.building_scaleZ[i]);
      g.meshColor();
      g.draw(metro_body);
      g.scale(1.2);
      g.draw(metro_body_wires);
      g.popMatrix();
      g.popMatrix();
    }

    // resource
    g.meshColor();
    for (int i = 0; i < state.numResources; i++) {
      if (!state.resource_picked[i]) {
        g.pushMatrix();
        g.translate(state.resource_pos[i]);
        g.rotate(state.resource_angleA[i], 0, 1, 0);
        g.rotate(state.resource_angleB[i], 1, 0, 0);
        g.scale(state.resource_scale[i]);
        g.draw(resource_body);
        g.draw(resource_body_wires);
        g.popMatrix();
      }
    }
  }

}; // struct RendererThings

struct MyApp : DistributedApp<State> {
  Material material;
  Light light;

  RendererThings renderer;

  // initial location managers
  Metropolis metropolis;
  Factories factories;
  NaturalResourcePointsCollection
      NaturalResourcePts; // manager for Natural Resource Points

  // initial agent managers
  Capitalist_Entity capitalists;
  Miner_Group miners;
  Worker_Union workers;

  // market manager
  MarketManager marketManager;
  DynamicScene scene;

  //    //for cuttlebone
  //    State state;
  //    cuttlebone::Maker<State> maker;

  // renderMode
  int renderModeSwitch = 1;
  float colorR = 1;
  float colorG = 0.85;
  float colorB = 0.4;
  float fogamount = 0.1;

  Color bgColor;

  // cameraMode
  int cameraSwitch = 0;

  // shader
  // ShaderProgram shader;
  float phase;

  // background noise
  Mesh geom;

  void onInit() override {

    std::cout << "I am " << roleName() <<std::endl;
    // set up window size to match two projectors on one machine
    if (role() == ROLE_RENDERER) {
        cout << "this is renderer" << endl;
        if (sphere::is_renderer()) {
            int width, height;
            sphere::get_fullscreen_dimension(&width, &height);
            if (width != 0 && height != 0) {
            dimensions(0, 0, width, height);
            decorated(false);
            }
            else {
            throw std::runtime_error("[!] in sphere renderer but calculated"
                                    "width and/or height are/is zero");
            }
        }
        else { // Not in sphere
            cout << "  but not sphere machin" << endl;
            dimensions(50, 50, 640, 480);
        }
        
    }

    if (role() == DistributedApp::ROLE_SIMULATOR) {
      for (auto &capitalist: capitalists.cs) {
//            scene.insertFreeVoice(&capitalist);
            
            scene.triggerOn(&capitalist);
        }
        for (auto &worker: workers.workers) {
//            scene.insertFreeVoice(&worker);
            scene.triggerOn(&worker);
        }
        for (auto &miner: miners.ms) {
//            scene.insertFreeVoice(&worker);
            scene.triggerOn(&miner);
        }
        scene.distanceAttenuation().farClip(6);
        scene.distanceAttenuation().nearClip(0.01);
        // scene.distanceAttenuation().law(ATTEN_INVERSE_SQUARE);
        // scene.distanceAttenuation().attenuation(1);

      SpeakerLayout sl = AlloSphereSpeakerLayout();
      scene.setSpatializer<Vbap>(sl);
        scene.configureAudio(audioIO());

    }
  }

  void onCreate() override {

    // update some variables for omni drawing and load warp-blend data
    running_in_sphere_renderer = sphere::is_renderer();
    window_is_stereo_buffered = Window::displayMode() & Window::STEREO_BUF;

    std::cout << "In sphere: " << running_in_sphere_renderer;
    std::cout << " stereo buffered:" << window_is_stereo_buffered << std::endl;
    if (role() == DistributedApp::ROLE_RENDERER) {
      load_perprojection_configuration();
      cursorHide(true);
    } else {

    }

    // common for both sim and ren
    {
      light.pos(0, 0, 0);  // place the light
      nav().pos(0, 0, 50); // place the viewer //80

      // background geom noise
      Mat4f xfm;
      for (int i = 0; i < 1000; ++i) {
        xfm.setIdentity();
        xfm.scale(Vec3f(0.4, 0.4, 0.4));
        Vec3f t = r();
        Vec3f temp = t;
        t = t * 60 + temp.normalize(40);
        xfm.translate(t);
        int Nv = addWireBox(geom);
        geom.transform(xfm, geom.vertices().size() - Nv);
      }
    }

    // generate factories according to number of capitalists
    if (role() == DistributedApp::ROLE_SIMULATOR) {
      lens().near(0.1).far(250); // for fog
      phase = 0;
      factories.generate(capitalists);
      metropolis.generate(capitalists);
      marketManager.statsInit(capitalists, workers, miners);
      workers.initID();
    }

    if (role() == DistributedApp::ROLE_RENDERER) {
      lens().near(0.1).far(150);
      renderer.init(state());
    }
  }

  void simulator_system_update() {
    // market
    marketManager.populationMonitor(capitalists, workers, miners, factories.fs);
    marketManager.capitalMonitor(capitalists, workers, miners, factories.fs);
    marketManager.updatePrice(capitalists, workers, miners);
    // related to market
    factories.getLaborPrice(marketManager);
    miners.calculateResourceUnitPrice(factories.fs);

    // locations
    metropolis.run();
    factories.run(capitalists);
    NaturalResourcePts.run();

    // agents
    capitalists.run(metropolis.mbs);
    miners.run(NaturalResourcePts.nrps, miners.ms, capitalists.cs);
    workers.run(factories.fs, workers.workers, capitalists.cs);

    // interaction between groups
    NaturalResourcePts.checkMinerPick(miners.ms);
    factories.checkWorkerNum(workers.workers);
    metropolis.mapCapitalistStats(capitalists.cs);
    capitalists.getResource(miners.ms);
    capitalists.getWorkersPaymentStats(factories.fs);

    // pay workers
    factories.payWorkers(marketManager);

    // locational behaviors
    factories.drawLinks(capitalists);

    // camera
    if (cameraSwitch == 1) {
      nav().pos() = capitalists.cs[0].pose().pos() + Vec3f(0, 0, -4);
      // nav().faceToward(capitalists.cs[0].movingTarget, 0.3*dt);
    } else if (cameraSwitch == 2) {
      nav().pos() = workers.workers[0].pose().pos() + Vec3f(0, 0, -4);
      // nav().faceToward(factories.fs[workers.workers[0].id_ClosestFactory].position,
      // 0.3*dt);
    } else if (cameraSwitch == 3) {
      nav().pos() = miners.ms[0].pose().pos() + Vec3f(0, 0, -4);
      // nav().faceToward(NaturalResourcePts.nrps[miners.ms[0].id_ClosestNRP].position,
      // 0.3 * dt);
    } else {
    }

    // background(Color(0.07));
    if (renderModeSwitch == 3) {
      bgColor = Color(1, 0.85, 0.4);
    } else if (renderModeSwitch == 2) {
      bgColor = Color(1, 0.5, 0.6);
    } else if (renderModeSwitch == 1) {
      bgColor = Color(1, 1, 1);
    }
  }

  void simulator_state_update() {
    state().numMiners = miners.ms.size();
    state().numWorkers = workers.workers.size();
    state().numCapitalists = capitalists.cs.size();
    state().numResources = NaturalResourcePts.nrps.size() * 7;
    state().phase = phase;

    for (size_t i = 0; i < miners.ms.size(); i++) {
      state().miner_pose[i] = miners.ms[i].pose();
      state().miner_scale[i] = miners.ms[i].scaleFactor;
      state().miner_poetryHoldings[i] = miners.ms[i].poetryHoldings;
      state().miner_bankrupted[i] = miners.ms[i].bankrupted();
      state().miner_fullpack[i] = miners.ms[i].fullpack;
      state().miner_lines_posA[i] = miners.lines[i].vertices()[0];
      state().miner_lines_posB[i] = miners.lines[i].vertices()[1];
    }
    for (size_t i = 0; i < workers.workers.size(); i++) {
      state().worker_pose[i] = workers.workers[i].pose();
      state().worker_scale[i] = workers.workers[i].scaleFactor;
      state().worker_poetryHoldings[i] = workers.workers[i].poetryHoldings;
      state().worker_bankrupted[i] = workers.workers[i].bankrupted();
      state().worker_lines_posA[i] = workers.lines[i].vertices()[0];
      state().worker_lines_posB[i] = workers.lines[i].vertices()[1];
    }
    for (size_t i = 0; i < capitalists.cs.size(); i++) {
      state().capitalist_pose[i] = capitalists.cs[i].pose();
      state().capitalist_scale[i] = capitalists.cs[i].scaleFactor;
      state().capitalist_poetryHoldigs[i] = capitalists.cs[i].poetryHoldings;
      state().capitalist_bankrupted[i] = capitalists.cs[i].bankrupted();
      state().capitalist_lines_posA[i] = factories.lines[i].vertices()[0];
      state().capitalist_lines_posB[i] = factories.lines[i].vertices()[1];
      state().factory_pos[i] = factories.fs[i].position;
      state().factory_rotation_angle[i] = factories.fs[i].angle1;
      state().factory_facing_center[i] = factories.fs[i].facing_center;
      state().factory_size[i] = factories.fs[i].scaleFactor;
      state().factory_color[i] = factories.fs[i].c;
      state().building_pos[i] = metropolis.mbs[i].position;
      state().building_size[i] = metropolis.mbs[i].scaleFactor;
      state().building_scaleZ[i] = metropolis.mbs[i].scaleZvalue;
    }
    for (size_t i = 0; i < NaturalResourcePts.nrps.size(); i++) {
      state().resource_point_pos[i] = NaturalResourcePts.nrps[i].position;
      for (size_t j = 0; j < NaturalResourcePts.nrps[i].resources.size(); j++) {
        state()
            .resource_pos[i * NaturalResourcePts.nrps[i].resources.size() + j] =
            NaturalResourcePts.nrps[i].resources[j].position;
        state()
            .resource_angleA[i * NaturalResourcePts.nrps[i].resources.size() +
                             j] =
            NaturalResourcePts.nrps[i].resources[j].angle1;
        state()
            .resource_angleB[i * NaturalResourcePts.nrps[i].resources.size() +
                             j] =
            NaturalResourcePts.nrps[i].resources[j].angle2;
        state().resource_scale[i * NaturalResourcePts.nrps[i].resources.size() +
                               j] =
            NaturalResourcePts.nrps[i].resources[j].scaleFactor;
        state()
            .resource_picked[i * NaturalResourcePts.nrps[i].resources.size() +
                             j] =
            NaturalResourcePts.nrps[i].resources[j].isPicked;
      }
    }
    state().metro_rotate_angle = metropolis.angle;
    state().nav_pose = nav();
    state().renderModeSwitch = renderModeSwitch;
    state().colorR = colorR;
    state().colorG = colorG;
    state().colorB = colorB;
    state().fogamount = fogamount;
  }

  void onAnimate(double dt) override {

    if (role() == DistributedApp::ROLE_SIMULATOR) {
      simulator_system_update();
      simulator_state_update();
    }

    if (role() == DistributedApp::ROLE_RENDERER) {
      nav().set(state().nav_pose);
      // per-projection renderer needs to know where the camera is
      pp_render.pose(state().nav_pose);
    }
  }

  void draw_omni_scene(Graphics& g)
  {
    if (state().renderModeSwitch == 3) {
      g.clear(1, 0.85, 0.4);
      g.depthTesting(true);
      g.blending(false);
    }
    else if (state().renderModeSwitch == 2) {
      g.clear(0.31, 0, 0.27);
      g.depthTesting(false);
      g.blending(true);
      g.blendModeAdd();
    }
    else if (state().renderModeSwitch == 1) {
      g.clear(1, 1, 1);
      g.depthTesting(false);
      g.blending(true);
      g.blendModeSub();
    }

    g.lighting(true);
    g.light(light);

    renderer.draw(g, state());
    g.color(0);
    g.draw(geom);

#if 0
    Mesh mesh;
    addIcosahedron(mesh);
    for(int aa = -5; aa <= 5; aa++)
      for(int bb = -5; bb <= 5; bb++)
        for(int cc = -5; cc <= 5; cc++)  {
          if(aa == 0 && bb == 0 && cc == 0) continue;
          g.pushMatrix();
          g.translate(aa * 2, bb * 2, cc * 2);
          g.rotate(sin(2 * sec()), 0, 0, 1);
          g.rotate(sin(3 * sec()), 0, 1, 0);
          g.scale(0.3, 0.3, 0.3);
          g.color((aa + 5)/10.0, (bb + 5)/10.0, (cc + 5)/10.0);
          g.draw(mesh);
          g.popMatrix();
    }
#endif
    
  }

  void perprojection_capture();

  void onDraw(Graphics &g) override {

    if (role() == DistributedApp::ROLE_RENDERER) {
      perprojection_capture();
      return;
    }

    // simulator from now on

    g.clear(bgColor);

    if (renderModeSwitch == 1) {
      // FIXME AC put back fog
      // g.fog(lens().far() * 2, lens().far(), background());
      g.depthTesting(false);
      g.blending(true);
      g.blendModeSub();
    } else if (renderModeSwitch == 2) {
      // g.fog(lens().far(), lens().near(), background());
      g.depthTesting(false);
      g.blending(true);
      g.blendModeAdd();
    } else if (renderModeSwitch == 3) {
      // FIXME AC put back fog
      // g.fog(lens().far(), lens().near()+2, background());
      // shader.uniform("fogCurve", 4*cos(8*phase*6.2832));
      g.depthTesting(true);
      g.blending(false);
    }

    g.lighting(true);
    g.light(light);

    // scene.render(g);
    // draw all the entities
    metropolis.draw(g);
    factories.draw(g);
    NaturalResourcePts.draw(g);
    capitalists.draw(g);
    miners.draw(g);
    workers.draw(g);
    g.draw(geom);

    scene.listenerPose(nav());
  }

  void onSound(AudioIOData &io) override {
    scene.render(io);
  }

  void onKeyDown(const Keyboard &k) override {
    switch (k.key()) {
    case '7':
      factories.drawingLinks = !factories.drawingLinks;
      break;
    case '8':
      miners.drawingLinks = !miners.drawingLinks;
      break;
    case '9':
      workers.drawingLinks = !workers.drawingLinks;
      break;
    case '1':
      renderModeSwitch = 1;
      break;
    case '2':
      renderModeSwitch = 2;
      break;
    case '3':
      renderModeSwitch = 3;
      break;
    case '4':
      cameraSwitch = 1;
      break;
    case '5':
      cameraSwitch = 2;
      break;
    case '6':
      cameraSwitch = 3;
      break;
    case '0':
      cameraSwitch = 0;
      nav().pos(0, 0, 80);
      nav().faceToward(Vec3f(0, 0, 0), 1);
    case 'y':
      if (colorR < 1.0) {
        colorR += 0.01;
      }
      cout << "R = " << colorR << endl;
      break;
    case 'h':
      if (colorR > 0.0) {
        colorR -= 0.01;
      }
      cout << "R = " << colorR << endl;
      break;
    case 'u':
      if (colorG < 1.0) {
        colorG += 0.01;
      }
      cout << "G = " << colorG << endl;
      break;
    case 'j':
      if (colorG > 0.0) {
        colorG -= 0.01;
      }
      cout << "G = " << colorG << endl;
      break;
    case 'i':
      if (colorB < 1.0) {
        colorB += 0.01;
      }
      cout << "B = " << colorB << endl;
      break;
    case 'k':
      if (colorB > 0.0) {
        colorB -= 0.01;
      }
      cout << "B = " << colorB << endl;
      break;
    case '=':
      if (fogamount < 1.0) {
        fogamount += 0.1;
      }
      cout << "fog = " << fogamount << endl;
      break;
    case '-':
      if (fogamount > 0.0) {
        fogamount -= 0.1;
      }
      cout << "fog = " << fogamount << endl;
      break;
    }
  }

};

int main() {
  MyApp app;
  // app.AlloSphereAudioSpatializer::audioIO().start();

  // QUESTION for Andrés:
  //   does DistributedApp handle 'not opening audio io if it is renderer'
  app.initAudio(SAMPLE_RATE, BLOCK_SIZE, 2, 0);
  gam::Sync::master().spu(app.audioIO().fps());
  if (app.role() == DistributedApp<State>::ROLE_RENDERER) {
    app.displayMode(Window::DEFAULT_BUF | Window::STEREO_BUF);
  } else {
    app.displayMode(Window::DEFAULT_BUF);
  }
  app.start();
}

void MyApp::perprojection_capture() {
  // start drawing to perprojection fbos
  mGraphics.omni(true);
  // pushes fbo, viewport, viewmat, projmat, lens, shader
  pp_render.begin(mGraphics);
  glDrawBuffer(GL_COLOR_ATTACHMENT0); // for fbo's output
  if (render_stereo) {
    for (int eye = 0; eye < 2; eye += 1) {
      pp_render.set_eye(eye);
      for (int i = 0; i < pp_render.num_projections(); i++) {
        pp_render.set_projection(i);
        mGraphics.depthTesting(true);
        mGraphics.depthMask(true);
        mGraphics.blending(false);
        draw_omni_scene(mGraphics);
      }
    }
  } else {
    pp_render.set_eye(eye_to_render);
    for (int i = 0; i < pp_render.num_projections(); i++) {
      pp_render.set_projection(i);
      mGraphics.depthTesting(true);
      mGraphics.depthMask(true);
      mGraphics.blending(false);
      draw_omni_scene(mGraphics);
    }
  }
  pp_render.end(); // pops everything pushed before

  // no stereo and no omni when actually displaying sampled result
  mGraphics.omni(false);
  mGraphics.eye(Graphics::MONO_EYE);

  // perprojection compositing changes viewport, save it
  mGraphics.pushViewport(0, 0, fbWidth(), fbHeight());

  mGraphics.blending(false);
  mGraphics.depthTesting(false);

  // now sample the results
  if (running_in_sphere_renderer) {
    if (window_is_stereo_buffered) {
      // rendering stereo in sphere
      glDrawBuffer(GL_BACK_LEFT);
      mGraphics.clearColor(0, 0, 0);
      mGraphics.clearDepth(1);
      pp_render.composite(mGraphics, 0);
      glDrawBuffer(GL_BACK_RIGHT);
      mGraphics.clearColor(0, 0, 0);
      mGraphics.clearDepth(1);
      pp_render.composite(mGraphics, 1);
    } else { // rendering mono in sphere
      // std::cout << "sampling mono in sphere setup" << std::endl;
      glDrawBuffer(GL_BACK_LEFT);
      mGraphics.clearColor(1, 0, 0);
      mGraphics.clearDepth(1);
      pp_render.composite(mGraphics, (eye_to_render == 1) ? 1 : 0);
    }
  } else {
    if (window_is_stereo_buffered) {
      // rendering stereo on display other than sphere
      glDrawBuffer(GL_BACK_LEFT);
      mGraphics.clearColor(0, 0, 0);
      mGraphics.clearDepth(1);
      pp_render.composite_desktop(mGraphics, 0); // texture[0]: left
      glDrawBuffer(GL_BACK_RIGHT);
      mGraphics.clearColor(0, 0, 0);
      mGraphics.clearDepth(1);
      pp_render.composite_desktop(mGraphics, 1); // texture[1]: right
    } else { // rendering mono on display other than sphere
      // std::cout << "sampling mono on flat display" << std::endl;
      glDrawBuffer(GL_BACK_LEFT);
//      mGraphics.clearColor(0.2, 0.2, 0.2);
      mGraphics.clearColor(1, 0.2, 0.2);
      mGraphics.clearDepth(1);
      pp_render.composite_desktop(
          mGraphics,
          (eye_to_render == 1) ? 1 : 0 // mono and left eye is
                                       // rendered on texture[0],
                                       // right eye is on texture[1]
      );
    }
  }
  mGraphics.popViewport();
  // put back default drawbuffer
  glDrawBuffer(GL_BACK_LEFT);
}
