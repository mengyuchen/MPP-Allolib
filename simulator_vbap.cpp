#include "allocore/io/al_App.hpp"
#include "allocore/math/al_Quat.hpp"
#include "allocore/spatial/al_Pose.hpp"
#include "helper.hpp"
#include "agent_managers.hpp"
#include "location_managers.hpp"
#include "common.hpp"
#include "alloutil/al_AlloSphereAudioSpatializer.hpp"
#include "alloutil/al_AlloSphereSpeakerLayout.hpp"
#include "allocore/sound/al_Vbap.hpp"
#include "Gamma/Filter.h"
#include "Gamma/SamplePlayer.h"
#include "alloutil/al_Simulator.hpp"
#include "Cuttlebone/Cuttlebone.hpp"

#define MAXIMUM_NUMBER_OF_SOUND_SOURCES (300)
#define BLOCK_SIZE (2048)
using namespace al;
using namespace std;

//Mengyu Chen, 2018
//mengyuchen@ucsb.edu

struct MyApp : App, AlloSphereAudioSpatializer, InterfaceServerClient {
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

    //for cuttlebone
    State state;
    cuttlebone::Maker<State> maker;

    //renderMode
    int renderModeSwitch = 1;

    //cameraMode
    int cameraSwitch = 0;
    //shader
    ShaderProgram shader;
    float phase;

    //background noise
    Mesh geom;

    //Vbap audio spatializer
    AudioScene vbap_scene;
    SpeakerLayout* speakerLayout;
    Spatializer* panner;
    Listener* listener;
    SoundSource *source[MAXIMUM_NUMBER_OF_SOUND_SOURCES];

    MyApp() : maker(Simulator::defaultBroadcastIP()),
        InterfaceServerClient(Simulator::defaultInterfaceServerIP()), vbap_scene(BLOCK_SIZE)        {
        bool inSphere = system("ls /alloshare >> /dev/null 2>&1") == 0;
        light.pos(0, 0, 0);              // place the light
        nav().pos(0, 0, 50);             // place the viewer //80
        //lens().far(400);                 // set the far clipping plane
        
        //background(Color(0.07));
        background(Color(1, 0.85, 0.4, 1.0));
        initWindow();
        //initAudio(44100);
        

        //background geom noise
        Mat4f xfm;

		for(int i=0; i<1000; ++i){
			xfm.setIdentity();
			xfm.scale(Vec3f(0.2, 0.2, 0.2));
            Vec3f t = r();
            Vec3f temp = t;
			t = t * 60 + temp.normalize(40);
			xfm.translate(t);

			int Nv = addWireBox(geom);
			geom.transform(xfm, geom.vertices().size()-Nv);
		}

        //shader
        phase = 0;
        lens().near(0.1).far(150);        //for fog
        shader.compile(
		R"(
			/* 'fogCurve' determines the distribution of fog between the near and far planes.
			Positive values give more dense fog while negative values give less dense
			fog. A value of	zero results in a linear distribution. */
			uniform float fogCurve;

			/* The fog amount in [0,1] passed to the fragment shader. */
			varying float fogFactor;

			void main(){
				gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
				gl_FrontColor = gl_Color;

				float z = gl_Position.z;
				//float z = gl_FragCoord.z / gl_FragCoord.w; /* per-frament fog would use this */
				fogFactor = (z - gl_Fog.start) * gl_Fog.scale;
				fogFactor = clamp(fogFactor, 0., 1.);
				if(fogCurve != 0.){
					fogFactor = (1. - exp(-fogCurve*fogFactor))/(1. - exp(-fogCurve));
				}
			}
		)",
		R"(
			varying float fogFactor;

			void main(){
				gl_FragColor = mix(gl_Color, gl_Fog.color, fogFactor);
			}
		)"
		);

        //set speaker layout
        // const int numSpeakers = 2;
        // Speaker speakers[numSpeakers] = {
        //     Speaker(0, 45, 0),
        //     Speaker(1, -45, 0),
        // };

        // speakerLayout.addSpeaker(speakers[0]);
        // speakerLayout.addSpeaker(speakers[1]);

        // spat = new AmbisonicsSpatializer(speakerLayout, 2, 1);

        // scene.usePerSampleProcessing(false);
        //listener = scene()->createListener(spat);

        //allo audio
        //AlloSphereAudioSpatializer::initAudio();
        AlloSphereAudioSpatializer::initSpatialization();
        for (unsigned i = 0; i < capitalists.cs.size(); ++i) {
            //scene()->addSource(*capitalists.cs[i].soundSource);
            capitalists.cs[i].v_player.rate(1);
        }
        // if (!inSphere){
        //     speakerLayout = new HeadsetSpeakerLayout();
        //     panner = new StereoPanner(*speakerLayout);
        //     //panner->print();
        //     //panner = new Vbap(*speakerLayout, true);
        //     panner->print();
        // } else {
        //     speakerLayout = new AlloSphereSpeakerLayout();
        //     panner = new Vbap(*speakerLayout);
        //     panner->print();

        // }
        speakerLayout = new AlloSphereSpeakerLayout();
            panner = new Vbap(*speakerLayout);
            panner->print();

        listener = vbap_scene.createListener(panner);
        listener->compile(); // XXX need this?
        float near = 0.2;
        float listenRadius = 60;
        for (int i = 0; i < capitalists.cs.size(); i++) {
            source[i] = new SoundSource();
            source[i]->nearClip(near);
            source[i]->farClip(listenRadius);
            source[i]->law(ATTEN_LINEAR);
            //source[i].law(ATTEN_INVERSE_SQUARE);
            source[i]->dopplerType(DOPPLER_NONE); // XXX doppler kills when moving fast!
            //source[i].law(ATTEN_INVERSE);
            vbap_scene.addSource(*source[i]);
        }
        vbap_scene.usePerSampleProcessing(false);
        //scene()->usePerSampleProcessing(false);
        //audioIO().device(AudioDevice("TASCAM"));
        //initAudio(44100, BLOCK_SIZE);
        //App::audioIO().device(AudioDevice("ECHO X5"));
        AlloSphereAudioSpatializer::initAudio("ECHO X5", 44100, BLOCK_SIZE, 60, 60);
        //App::initAudio(44100, BLOCK_SIZE, 4, 0);
        //cout << "Using audio device: " << endl;
        //audioIO().print();
        fflush(stdout);


        //generate factories according to number of capitalists
        factories.generate(capitalists);
        metropolis.generate(capitalists);
        marketManager.statsInit(capitalists, workers, miners);
        workers.initID();

        
    }
    void onAnimate(double dt) {
        //shader phase
        phase += 0.00017; if(phase>=1) --phase;
        //this updates your nav, especially you use a controller / joystick
        while (InterfaceServerClient::oscRecv().recv()){

        }

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
            nav().pos() = capitalists.cs[0].pose.pos() + Vec3f(0,0,-4);
            //nav().faceToward(capitalists.cs[0].movingTarget, 0.3*dt);
        } else if (cameraSwitch == 2) {
            nav().pos() = workers.workers[0].pose.pos()+ Vec3f(0,0,-4);
            //nav().faceToward(factories.fs[workers.workers[0].id_ClosestFactory].position, 0.3*dt);
        } else if (cameraSwitch == 3) {
            nav().pos() = miners.ms[0].pose.pos() + Vec3f(0,0,-4);
            //nav().faceToward(NaturalResourcePts.nrps[miners.ms[0].id_ClosestNRP].position, 0.3 * dt);
        } else {
            
        }

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
        state.numMiners = miners.ms.size();
        state.numWorkers = workers.workers.size();
        state.numCapitalists = capitalists.cs.size();
        state.numResources = NaturalResourcePts.nrps.size() * 7;
        state.phase = phase;

        for (int i = 0; i < miners.ms.size(); i ++){
            state.miner_pose[i] = miners.ms[i].pose;
            state.miner_scale[i] = miners.ms[i].scaleFactor;
            state.miner_poetryHoldings[i] = miners.ms[i].poetryHoldings;
            state.miner_bankrupted[i] = miners.ms[i].bankrupted();
            state.miner_fullpack[i] = miners.ms[i].fullpack;
            state.miner_lines_posA[i] = miners.lines[i].vertices()[0];
            state.miner_lines_posB[i] = miners.lines[i].vertices()[1];
    
        }
        for (int i = 0; i < workers.workers.size(); i ++){
            state.worker_pose[i] = workers.workers[i].pose;
            state.worker_scale[i] = workers.workers[i].scaleFactor;
            state.worker_poetryHoldings[i] = workers.workers[i].poetryHoldings;
            state.worker_bankrupted[i] = workers.workers[i].bankrupted();
            state.worker_lines_posA[i] = workers.lines[i].vertices()[0];
            state.worker_lines_posB[i] = workers.lines[i].vertices()[1];
        }
        for (int i = 0; i < capitalists.cs.size(); i ++){
            state.capitalist_pose[i] = capitalists.cs[i].pose;
            state.capitalist_scale[i] = capitalists.cs[i].scaleFactor;
            state.capitalist_poetryHoldigs[i] = capitalists.cs[i].poetryHoldings;
            state.capitalist_bankrupted[i] = capitalists.cs[i].bankrupted();
            state.capitalist_lines_posA[i] = factories.lines[i].vertices()[0];
            state.capitalist_lines_posB[i] = factories.lines[i].vertices()[1];
            state.factory_pos[i] = factories.fs[i].position;
            state.factory_rotation_angle[i] = factories.fs[i].angle1;
            state.factory_facing_center[i] = factories.fs[i].facing_center;
            state.factory_size[i] = factories.fs[i].scaleFactor;
            state.factory_color[i] = factories.fs[i].c;
            state.building_pos[i] = metropolis.mbs[i].position;
            state.building_size[i] = metropolis.mbs[i].scaleFactor;
            state.building_scaleZ[i] = metropolis.mbs[i].scaleZvalue;
        } 
        for (int i = 0; i < NaturalResourcePts.nrps.size(); i ++){
            state.resource_point_pos[i] = NaturalResourcePts.nrps[i].position;
            for (int j = 0; j < NaturalResourcePts.nrps[i].resources.size(); j ++){
                state.resource_pos[i * NaturalResourcePts.nrps[i].resources.size() + j] = NaturalResourcePts.nrps[i].resources[j].position; 
                state.resource_angleA[i * NaturalResourcePts.nrps[i].resources.size() + j] = NaturalResourcePts.nrps[i].resources[j].angle1;
                state.resource_angleB[i * NaturalResourcePts.nrps[i].resources.size() + j] = NaturalResourcePts.nrps[i].resources[j].angle2;
                state.resource_scale[i * NaturalResourcePts.nrps[i].resources.size() + j] = NaturalResourcePts.nrps[i].resources[j].scaleFactor;
                state.resource_picked[i * NaturalResourcePts.nrps[i].resources.size() + j] = NaturalResourcePts.nrps[i].resources[j].isPicked;
            }
        }
        state.metro_rotate_angle = metropolis.angle;
        state.nav_pose = nav();
        state.renderModeSwitch = renderModeSwitch;

        maker.set(state);
   
    }
    void onDraw(Graphics& g) {
        if (renderModeSwitch == 1){
            g.blendSub();
            g.fog(lens().far(), lens().near()+2, background());
            shader.begin();
			shader.uniform("fogCurve", 4*cos(8*phase*6.2832));
        } else if (renderModeSwitch == 2){
            g.blendAdd();
        } else {
            g.blending(false);
        }
        
            material();
            light();
            //glEnable(GL_POINT_SPRITE);
            
            //draw all the entities
            metropolis.draw(g);
            factories.draw(g);
            NaturalResourcePts.draw(g);
            capitalists.draw(g);
            miners.draw(g);
            workers.draw(g);
            g.draw(geom);
        if (renderModeSwitch == 1){
            shader.end();
        }

    }
    virtual void onSound(AudioIOData& io) {
        gam::Sync::master().spu(AlloSphereAudioSpatializer::audioIO().fps());
        float x = nav().pos().x;
        float y = nav().pos().y;
        float z = nav().pos().z;

        //vector<unsigned> n;
        for (int i = 0; i < capitalists.cs.size(); i++){
            source[i]->pos(capitalists.cs[i].pose.pos().x,capitalists.cs[i].pose.pos().y, capitalists.cs[i].pose.pos().z);
        //double d = (source[i].pos() - listener->pos()).mag();
        //double a = source[i].attenuation(d);
        //double db = log10(a) * 20.0;
        //cout << d << "," << a << "," << db << endl;
        
      }
//cout << source[0]->pos() << " source 0 pos" << endl;
        listener->pos(x, y, z);
        int numFrames = io.framesPerBuffer();
        for (int k = 0; k < numFrames; k++) {
            for (int i = 0; i < capitalists.cs.size(); i++) {
                    //io.frame(0);
                    float f = 0;
                    f = capitalists.cs[i].v_player();
                    double d = isnan(f) ? 0.0 : (double)f; // XXX need this nan check?
                    source[i]->writeSample(d);
                    io.frame(0);
            }
        }
        vbap_scene.render(io);
        // for (unsigned i = 0; i < 15; ++i){
           
        //    capitalists.cs[i].updateAuidoPose();
        //    capitalists.cs[i].onProcess(io);
        //    io.frame(0);
        // }
        //listener()->pose(nav());
        //io.frame(0);
        //scene()->render(io);
   
    }
    void onKeyDown(const ViewpointWindow&, const Keyboard& k) {
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
        }
    }

};

int main() { 
    MyApp app;
    app.AlloSphereAudioSpatializer::audioIO().start();
    app.InterfaceServerClient::connect();
    app.maker.start();
    app.start(); 
}
