#include "al/core/app/al_DistributedApp.hpp"
#include "al/core/math/al_Quat.hpp"
#include "al/core/spatial/al_Pose.hpp"
#include "helper.hpp"
#include "agent_managers.hpp"
#include "location_managers.hpp"
#include "common.hpp"
//#include "al/util/al_AlloSphereAudioSpatializer.hpp"
#include "al/util/al_AlloSphereSpeakerLayout.hpp"
#include "al/core/sound/al_StereoPanner.hpp"
#include "al/core/sound/al_Vbap.hpp"
#include "al/core/sound/al_AudioScene.hpp"
//#include "al/util/al_Simulator.hpp"
//#include "alloGLV/al_ControlGLV.hpp"
//#include "GLV/glv.h"

using namespace al;
using namespace std;

//Mengyu Chen, 2018
//mengyuchen@ucsb.edu

#define BLOCK_SIZE (256)
#define SAMPLE_RATE 44100
#define MAXIMUM_NUMBER_OF_SOUND_SOURCES (100)

 //Vbap audio spatializer
//static AudioScene  v_scene(BLOCK_SIZE); //to not confuse with scene()
static SpeakerLayout speakerLayout;
    //Vbap* panner;
static Spatializer *panner;
//static StereoPanner *panner;
static Listener *listener;
static SoundSource *source[15];
//static SoundSource *sourceWorker[75];

struct MyApp : DistributedApp<State> {
    Material material;
    Light light;

    //initial location managers
    Metropolis metropolis;
    Factories factories;
    NaturalResourcePointsCollection NaturalResourcePts; //manager for Natural Resource Points

    //initial agent managers
    Capitalist_Entity capitalists;
    Miner_Group miners;
    Worker_Union workers;

    //market manager
    MarketManager marketManager;
    // DynamicScene scene;

//    //for cuttlebone
//    State state;
//    cuttlebone::Maker<State> maker;

    //renderMode
    int renderModeSwitch = 1;
    float colorR = 1;
    float colorG = 0.85;
    float colorB = 0.4;
    float fogamount = 0.1;

    Color bgColor;

    //cameraMode
    int cameraSwitch = 0;

    //shader
    ShaderProgram shader;
    float phase;

    //background noise
    Mesh geom;



    void onCreate() override {
        
        light.pos(0, 0, 0);              // place the light
        nav().pos(0, 0, 50);             // place the viewer //80
        //lens().far(400);                 // set the far clipping plane


        //background geom noise
        Mat4f xfm;

		for(int i=0; i<1000; ++i){
			xfm.setIdentity();
			xfm.scale(Vec3f(0.4, 0.4, 0.4));
            Vec3f t = r();
            Vec3f temp = t;
			t = t * 60 + temp.normalize(40);
			xfm.translate(t);

			int Nv = addWireBox(geom);
			geom.transform(xfm, geom.vertices().size()-Nv);
		}

        //shader
        phase = 0;
        lens().near(0.1).far(250);        //for fog
        
        //allo audio
//        bool inSphere = system("ls /alloshare >> /dev/null 2>&1") == 0;
        //AlloSphereAudioSpatializer::initSpatialization();

        //switch between personal computer or allosphere
//        if (!inSphere){
//            speakerLayout = StereoSpeakerLayout();
//            panner = new StereoPanner(speakerLayout);
//            panner->print();
//        } else {
//            speakerLayout = AlloSphereSpeakerLayout();
//            //panner = new StereoPanner(speakerLayout);
//            panner = new Vbap(speakerLayout);
//            panner->print();
//        }
//        listener = v_scene.createListener(panner);
        //listener->compile(); // XXX need this?

//        float near = 0.2;
//        float listenRadius = 36;

        //load audio source, capitalists first
//        for (unsigned i = 0; i < capitalists.cs.size(); ++i) {
//            source[i] = new SoundSource();
//            source[i]->nearClip(near);
//            source[i]->farClip(listenRadius);
//            source[i]->law(ATTEN_LINEAR);
//            //source[i]->law(ATTEN_INVERSE_SQUARE);
//            source[i]->dopplerType(DOPPLER_NONE); // XXX doppler kills when moving fast!
//            //source[i].law(ATTEN_INVERSE);
//            v_scene.addSource(*source[i]);
//        }
        //  for (unsigned i = 0; i < workers.workers.size(); ++i) {
        //     sourceWorker[i] = new SoundSource();
        //     sourceWorker[i]->nearClip(near);
        //     sourceWorker[i]->farClip(listenRadius * 0.75);
        //     sourceWorker[i]->law(ATTEN_LINEAR);
        //     //source[i]->law(ATTEN_INVERSE_SQUARE);
        //     sourceWorker[i]->dopplerType(DOPPLER_NONE); // XXX doppler kills when moving fast!
        //     //source[i].law(ATTEN_INVERSE);
        //     v_scene.addSource(*sourceWorker[i]);
        // }
        
//        v_scene.usePerSampleProcessing(false);
        //AlloSphereAudioSpatializer::initAudio("ECHO X5", 44100, BLOCK_SIZE, 60, 60);
        //fflush(stdout);

        //generate factories according to number of capitalists
        factories.generate(capitalists);
        metropolis.generate(capitalists);
        marketManager.statsInit(capitalists, workers, miners);
        workers.initID();

        
    }

    void onAnimate(double dt) override {
        //shader phase
        //phase += 0.00017; if(phase>=1) --phase;
        //this updates your nav, especially you use a controller / joystick
//        while (InterfaceServerClient::oscRecv().recv()){

//        }

        //market
        marketManager.populationMonitor(capitalists, workers, miners, factories.fs);
        marketManager.capitalMonitor(capitalists, workers, miners, factories.fs);
        marketManager.updatePrice(capitalists, workers, miners);
        
        //related to market
        factories.getLaborPrice(marketManager);
        miners.calculateResourceUnitPrice(factories.fs);

        //locations
        metropolis.run();
        factories.run(capitalists);
        NaturalResourcePts.run();

        //agents
        capitalists.run(metropolis.mbs);
        miners.run(NaturalResourcePts.nrps, miners.ms, capitalists.cs);
        workers.run(factories.fs, workers.workers, capitalists.cs);
        
        //interaction between groups
        NaturalResourcePts.checkMinerPick(miners.ms);
        factories.checkWorkerNum(workers.workers);
        metropolis.mapCapitalistStats(capitalists.cs);
        capitalists.getResource(miners.ms);
        capitalists.getWorkersPaymentStats(factories.fs);

        //pay workers
        factories.payWorkers(marketManager);
       
        //locational behaviors
        factories.drawLinks(capitalists);

        //camera
        if (cameraSwitch == 1){
            nav().pos() = capitalists.cs[0].pose().pos() + Vec3f(0,0,-4);
            //nav().faceToward(capitalists.cs[0].movingTarget, 0.3*dt);
        } else if (cameraSwitch == 2) {
            nav().pos() = workers.workers[0].pose().pos()+ Vec3f(0,0,-4);
            //nav().faceToward(factories.fs[workers.workers[0].id_ClosestFactory].position, 0.3*dt);
        } else if (cameraSwitch == 3) {
            nav().pos() = miners.ms[0].pose().pos() + Vec3f(0,0,-4);
            //nav().faceToward(NaturalResourcePts.nrps[miners.ms[0].id_ClosestNRP].position, 0.3 * dt);
        } else {
            
        }
        //audio source position
        //capitlist sound position
        for (size_t i = 0; i < capitalists.cs.size(); i++){
        //FIXME AC put back setting sound source position
//            source[i]->pos(capitalists.cs[i].pose().pos().x,capitalists.cs[i].pose().pos().y, capitalists.cs[i].pose().pos().z);
                //double d = (source[i].pos() - listener->pos()).mag();
                //double a = source[i].attenuation(d);
                //double db = log10(a) * 20.0;
                //cout << d << "," << a << "," << db << endl;
        }
        //worker sound position
        // for (int i = 0; i < workers.workers.size(); i++){
        //     sourceWorker[i]->pos(workers.workers[i].pose().pos().x,workers.workers[i].pose().pos().y, workers.workers[i].pose().pos().z);
        //         //double d = (source[i].pos() - listener->pos()).mag();
        //         //double a = source[i].attenuation(d);
        //         //double db = log10(a) * 20.0;
        //         //cout << d << "," << a << "," << db << endl;
        // }

        //debug
        // cout << workers[0].id_ClosestFactory << " i m heading to " << endl;
        // cout << workers[0].distToClosestFactory << " this much far " << endl;
        // cout << factories.fs[0].workersWorkingNum << " =  workers " << endl;
        // cout << "  " << endl;
        // cout << factories.fs[0].materialStocks << "fc material" <<endl;
        // cout << capitalists.cs[0].resourceHoldings << "cp resource" << endl;
        // cout << capitalists.cs[0].resourceClock << "cp clock" <<endl;
        // cout << nrps.nrps[0].drained() << " drained?" << endl;
        // cout << nrps.nrps[0].regeneration_rate << " regen rate" << endl;
        // cout << nrps.nrps[0].afterDrainTimer << " timer" << endl;
        // cout << nrps.nrps[0].resources[0].isPicked << "  r0 is picked?" << endl;
        // cout << nrps.nrps[0].resources[0].beingPicked << "  r0 being picked?" << endl;
        // cout << nrps.nrps[0].pickCount << "  pickcount?" << endl;
        // cout << nrps.nrps[0].resources.size() << "size = count = " << nrps.nrps[0].pickCount << endl;

        //for cuttlebone
        state().numMiners = miners.ms.size();
        state().numWorkers = workers.workers.size();
        state().numCapitalists = capitalists.cs.size();
        state().numResources = NaturalResourcePts.nrps.size() * 7;
        state().phase = phase;

        for (size_t i = 0; i < miners.ms.size(); i ++){
            state().miner_pose[i] = miners.ms[i].pose();
            state().miner_scale[i] = miners.ms[i].scaleFactor;
            state().miner_poetryHoldings[i] = miners.ms[i].poetryHoldings;
            state().miner_bankrupted[i] = miners.ms[i].bankrupted();
            state().miner_fullpack[i] = miners.ms[i].fullpack;
            state().miner_lines_posA[i] = miners.lines[i].vertices()[0];
            state().miner_lines_posB[i] = miners.lines[i].vertices()[1];
    
        }
        for (size_t i = 0; i < workers.workers.size(); i ++){
            state().worker_pose[i] = workers.workers[i].pose();
            state().worker_scale[i] = workers.workers[i].scaleFactor;
            state().worker_poetryHoldings[i] = workers.workers[i].poetryHoldings;
            state().worker_bankrupted[i] = workers.workers[i].bankrupted();
            state().worker_lines_posA[i] = workers.lines[i].vertices()[0];
            state().worker_lines_posB[i] = workers.lines[i].vertices()[1];
        }
        for (size_t i = 0; i < capitalists.cs.size(); i ++){
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
        for (size_t i = 0; i < NaturalResourcePts.nrps.size(); i ++){
            state().resource_point_pos[i] = NaturalResourcePts.nrps[i].position;
            for (size_t j = 0; j < NaturalResourcePts.nrps[i].resources.size(); j ++){
                state().resource_pos[i * NaturalResourcePts.nrps[i].resources.size() + j] = NaturalResourcePts.nrps[i].resources[j].position;
                state().resource_angleA[i * NaturalResourcePts.nrps[i].resources.size() + j] = NaturalResourcePts.nrps[i].resources[j].angle1;
                state().resource_angleB[i * NaturalResourcePts.nrps[i].resources.size() + j] = NaturalResourcePts.nrps[i].resources[j].angle2;
                state().resource_scale[i * NaturalResourcePts.nrps[i].resources.size() + j] = NaturalResourcePts.nrps[i].resources[j].scaleFactor;
                state().resource_picked[i * NaturalResourcePts.nrps[i].resources.size() + j] = NaturalResourcePts.nrps[i].resources[j].isPicked;
            }
        }
        state().metro_rotate_angle = metropolis.angle;
        state().nav_pose = nav();
        state().renderModeSwitch = renderModeSwitch;
        state().colorR = colorR;
        state().colorG = colorG;
        state().colorB = colorB;
        state().fogamount = fogamount;

        //background(Color(0.07));
        if (renderModeSwitch == 3){
            bgColor = Color(1,0.85, 0.4);
        } else if (renderModeSwitch == 2){
            bgColor = Color(1, 0.5, 0.6);
        } else if (renderModeSwitch == 1){
            bgColor = Color(1,1,1);
        }
   
    }
    void onDraw(Graphics& g) override {
        g.clear(bgColor);
        if (renderModeSwitch == 1){
            // FIXME AC put back fog
//            g.fog(lens().far() * 2, lens().far(), background());
            g.depthTesting(false);
            g.blending(true);
            g.blendModeSub();
            
        } else if (renderModeSwitch == 2){
            //g.fog(lens().far(), lens().near(), background());
            g.depthTesting(false);
            g.blending(true);
            g.blendModeAdd();
        } else if (renderModeSwitch == 3) {

            // FIXME AC put back fog
//            g.fog(lens().far(), lens().near()+2, background());
            g.depthTesting(true);
            g.blending(false);
            // shader.begin();
			//shader.uniform("fogCurve", 4*cos(8*phase*6.2832));
        }
        // FIXME AC put back light and material
//            light();
//            material();

        g.lighting(true);
        g.light(light);
        
            
            //glEnable(GL_POINT_SPRITE);
            // scene.render(g);
            //draw all the entities
            metropolis.draw(g);
            factories.draw(g);
            NaturalResourcePts.draw(g);
            capitalists.draw(g);
            miners.draw(g);
            workers.draw(g);
            g.draw(geom);
        // if (renderModeSwitch == 3){
        //     shader.end();
        // }

    }
    void onSound(AudioIOData& io) override {
        // scene.listenerPose(nav());
        // scene.render(io);
//        int numFrames = io.framesPerBuffer();
//        for (int k = 0; k < numFrames; k++) {
//            //capitalist sample
//            for (int i = 0; i < capitalists.cs.size(); i++) {
//                //io.frame(0);
//                float f = 0;
//                capitalists.cs[i].onProcess(io); // XXX need this nan check?
//                // FIXME AC put back writing audio
////                source[i]->writeSample(d);
////                io.frame(0);
//            }
//            //worker sample
//            // for (int i = 0; i < workers.workers.size(); i ++){
//            //     float f = 0;
//            //     f = workers.workers[i].onProcess(io);
//            //     double d = isnan(f) ? 0.0 : (double)f;
//            //     sourceWorker[i]->writeSample(d);
//            //     io.frame(0);
//            // }
//        }
//        io.frame(0);
        
//        v_scene.render(io);
    }

    void onKeyDown(const Keyboard& k) override {
        switch(k.key()){
            case '7': factories.drawingLinks = !factories.drawingLinks; break;
            case '8': miners.drawingLinks = !miners.drawingLinks; break;
            case '9': workers.drawingLinks = !workers.drawingLinks;break;
            case '1': renderModeSwitch = 1; break;
            case '2': renderModeSwitch = 2; break;
            case '3': renderModeSwitch = 3; break;
            case '4': cameraSwitch = 1; break;
            case '5': cameraSwitch = 2; break;
            case '6': cameraSwitch = 3; break;
            case '0': cameraSwitch = 0; nav().pos(0,0,80);nav().faceToward(Vec3f(0,0,0), 1);
            case 'y': if (colorR < 1.0) {colorR += 0.01;} cout << "R = " << colorR << endl; break;
            case 'h': if (colorR > 0.0) {colorR -= 0.01;} cout << "R = " << colorR << endl; break;
            case 'u': if (colorG < 1.0) {colorG += 0.01;} cout << "G = " << colorG << endl; break;
            case 'j': if (colorG > 0.0) {colorG -= 0.01;} cout << "G = " << colorG << endl; break;
            case 'i': if (colorB < 1.0) {colorB += 0.01;} cout << "B = " << colorB << endl; break;
            case 'k': if (colorB > 0.0) {colorB -= 0.01;} cout << "B = " << colorB << endl; break;
            case '=': if (fogamount < 1.0){fogamount += 0.1;} cout << "fog = " << fogamount << endl; break;
            case '-': if (fogamount > 0.0){fogamount -= 0.1;} cout << "fog = " << fogamount << endl; break;
       }
    }

};

int main() { 
    MyApp app;
    //app.AlloSphereAudioSpatializer::audioIO().start();

//    app.initWindow();

    app.initAudio(SAMPLE_RATE, BLOCK_SIZE, 2, 0);
    gam::Sync::master().spu(app.audioIO().fps());
    app.start(); 
}
