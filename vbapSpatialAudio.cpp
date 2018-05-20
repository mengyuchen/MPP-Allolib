#include "allocore/io/al_App.hpp"
#include "common.hpp"
#include "alloutil/al_AlloSphereAudioSpatializer.hpp"
#include "alloutil/al_AlloSphereSpeakerLayout.hpp"
#include "allocore/sound/al_Vbap.hpp"
#include "Gamma/Filter.h"
#include "Gamma/Oscillator.h"
#include "Gamma/SamplePlayer.h"
#include "alloutil/al_Simulator.hpp"
#include "Cuttlebone/Cuttlebone.hpp"

//Mengyu Chen, 2018
//mengyuchen@ucsb.edu

#define MAXIMUM_NUMBER_OF_SOUND_SOURCES (100)
#define BLOCK_SIZE (2048)
using namespace al;
using namespace std;

// SampleLooper made by Karl
typedef gam::SamplePlayer<float, gam::ipl::Cubic, gam::tap::Wrap>
    GammaSamplePlayerFloatCubicWrap;

struct DynamicSamplePlayer : GammaSamplePlayerFloatCubicWrap {
  DynamicSamplePlayer() : GammaSamplePlayerFloatCubicWrap() { zero(); }
  DynamicSamplePlayer(const DynamicSamplePlayer& other) {}

  // need this for some old version of gcc
  DynamicSamplePlayer& operator=(const DynamicSamplePlayer& other) {
    return *this;
  }
};

// Agent
struct Agent {
    Pose pose;
    Mesh body;
    float size;
    float spawnRadius;
    int mesh_Nv;

    //test with sample and sine
    DynamicSamplePlayer player;
    gam::Sine<> osc;

    Agent(){
        //define body of agent
        size = 3;
        mesh_Nv = addTetrahedron(body,size);
        for(int i=0; i<mesh_Nv; ++i){
			float f = float(i)/mesh_Nv;
			body.color(HSV(f*0.1,0.9,1));
		}
        body.decompress();
        body.generateNormals();

        //randomize a position of agent
        spawnRadius = 20.0;
        pose.pos() = Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()) * spawnRadius;

        //load sample data into player
        SearchPaths searchPaths;
        searchPaths.addSearchPath("..");
        string filePath = searchPaths.find("socialismgood.wav").filepath();
        player.load(filePath.c_str());

        //init sine wave
        osc.freq(440);
    }

};

struct MyApp : App, AlloSphereAudioSpatializer, InterfaceServerClient {
    Material material;
    Light light;

    //Vbap audio spatializer
    AudioScene vbap_scene; //to not confuse with scene() 
    SpeakerLayout* speakerLayout;
    //Vbap* panner;
    Spatializer* panner;
    Listener* listener;
    SoundSource *source[MAXIMUM_NUMBER_OF_SOUND_SOURCES];

    //agents
    vector<Agent> agents;
    State state;
    cuttlebone::Maker<State> maker;

    MyApp() : maker(Simulator::defaultBroadcastIP()),
        InterfaceServerClient(Simulator::defaultInterfaceServerIP()), 
        vbap_scene(BLOCK_SIZE)        {
        
        
        light.pos(0, 0, 0);              // place the light
        nav().pos(0, 0, 50);             // place the viewer //80
        lens().far(400);                 // set the far clipping plane
        
        background(Color(0.07));
        initWindow();
        
        //initialize agents and set up basic player parameters
        agents.resize(15);
        float playbackRate = 1;
        for (unsigned i = 0; i < agents.size(); ++i) {
            agents[i].player.rate(playbackRate);
        }

        //init spatialization 
        bool inSphere = system("ls /alloshare >> /dev/null 2>&1") == 0;
        AlloSphereAudioSpatializer::initSpatialization();

        //switch between personal computer or allosphere
        if (!inSphere){
            speakerLayout = new StereoSpeakerLayout();
            panner = new StereoPanner(*speakerLayout);
            panner->print();
        } else {
            speakerLayout = new AlloSphereSpeakerLayout();
            panner = new Vbap(*speakerLayout);
            panner->print();
        }
        //give listener panner
        listener = vbap_scene.createListener(panner);
        listener->compile(); // XXX need this?
        //setup sound source
        float near = 0.2;
        float listenRadius = 60;
        for (int i = 0; i < agents.size(); i++) {
            source[i] = new SoundSource();
            source[i]->nearClip(near);
            source[i]->farClip(listenRadius);
            source[i]->law(ATTEN_LINEAR);
            //source[i]->law(ATTEN_INVERSE_SQUARE);
            source[i]->dopplerType(DOPPLER_NONE); // XXX doppler kills when moving fast!
            //source[i].law(ATTEN_INVERSE);
            vbap_scene.addSource(*source[i]);
        }
        vbap_scene.usePerSampleProcessing(false);
        AlloSphereAudioSpatializer::initAudio("ECHO X5", 44100, BLOCK_SIZE, 60, 60);
        fflush(stdout);
    }

    void onAnimate(double dt) {
        maker.set(state);
    }
    void onDraw(Graphics& g) {
        //draw agents
        material();
        light();
        for (int i = 0; i < agents.size(); i ++){
            g.pushMatrix();
            g.translate(agents[i].pose.pos());
            g.draw(agents[i].body);
            g.popMatrix();
        }
    }
    virtual void onSound(AudioIOData& io) {
        gam::Sync::master().spu(AlloSphereAudioSpatializer::audioIO().fps());
        for (int i = 0; i < agents.size(); i++){
            source[i]->pos(agents[i].pose.pos().x,agents[i].pose.pos().y, agents[i].pose.pos().z);
                //double d = (source[i].pos() - listener->pos()).mag();
                //double a = source[i].attenuation(d);
                //double db = log10(a) * 20.0;
                //cout << d << "," << a << "," << db << endl;
        }
        float x = nav().pos().x;
        float y = nav().pos().y;
        float z = nav().pos().z;
        
        listener->pos(x,y,z);
        int numFrames = io.framesPerBuffer();
        for (int k = 0; k < numFrames; k++) {
            for (int i = 0; i < agents.size(); i++) {
                //io.frame(0);
                float f = 0;
                f = agents[i].osc();
                double d = isnan(f) ? 0.0 : (double)f; // XXX need this nan check?
                source[i]->writeSample(d);
                io.frame(0);
            }
        }
        
        vbap_scene.render(io);        
    }
};

int main() { 
    MyApp app;
    app.AlloSphereAudioSpatializer::audioIO().start();
    app.InterfaceServerClient::connect();
    app.maker.start();
    app.start(); 
}
