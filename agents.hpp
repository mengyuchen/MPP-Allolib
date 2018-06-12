#ifndef INCLUDE_AGENTS_HPP
#define INCLUDE_AGENTS_HPP

#include "al/core/app/al_App.hpp"
#include "al/core/math/al_Quat.hpp"
#include "al/core/spatial/al_Pose.hpp"
#include "helper.hpp"
#include "agent_base.hpp"
#include "locations.hpp"
#include "Gamma/Filter.h"
#include "Gamma/Envelope.h"
#include "Gamma/DFT.h"
#include "Gamma/Effects.h"
#include "Gamma/Delay.h"
#include "Gamma/Noise.h"
#include "Gamma/Oscillator.h"
#include "Gamma/SamplePlayer.h"

//#include "alloutil/al_AlloSphereAudioSpatializer.hpp"
//#include "alloutil/al_Simulator.hpp"

using namespace al;
using namespace std;

struct Capitalist : Agent{

    int mesh_Nv;
    //Vec3f movingTarget;
    int desireChangeRate;
    float resourceHoldings;
    int TimeToDistribute;
    int resourceClock;
    float workersPayCheck;
    float laborUnitPrice;
    //float resourceUnitPrice;
    int numWorkers;
    int capitalistID;
    float bodyRadius;
    float bodyHeight;
    float totalResourceHoldings;

    //audio params

    //gamma effects
    //gam::LFO<> osc;
    gam::SineD<> sine;


    gam::Osc<> sin;
        gam::Env<3> sinADR;
        gam::Accum<> sinTmr;
        float sinDur;
        float baseFreq = 180.0f;
        
        
        float periods[8] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0, 1.0};
        //float periods[8] = {0.5,2,0.25,1.5,1.0, 4.0, 8.0, 3.0};
        float freqs[4] = {baseFreq * 2.0f, baseFreq * 1.95f, baseFreq *1.8f, baseFreq*1.2f};
        int index = 0;
        int tmrIndex = 0;

        //bigger control
        // gam::Accum<> sinTmr2;
        // gam::Env<3> ampEnv;
        // gam::Accum<> ampTmr;
        float shiftFreq;
        float finalFreq;

        //delay line
        gam::Delay<float, gam::ipl::Trunc> delay;
        float delayDur;

    Capitalist(){
        //initial params
        maxAcceleration = 1;
        mass = 1.0;
        maxspeed = 0.6;
        minspeed = 0.1;
        maxforce = 0.1;
        initialRadius = 5;
        target_senseRadius = 10.0;
        desiredseparation = 3.0f;
        acceleration = Vec3f(0,0,0);
        velocity = Vec3f(0,0,0);
        pose().pos() = r() * initialRadius;
        bioClock = 0;
        movingTarget = r();
        temp_pos = movingTarget;
        desireChangeRate = r_int(50, 150);

        //capitals
        resourceHoldings = (float)r_int(0, 10);
        totalResourceHoldings = resourceHoldings;
        capitalHoldings = (float)r_int(20000, 20000);
        poetryHoldings = 0.0;
        laborUnitPrice = 420.0;
        //resourceUnitPrice = 280.0;
        numWorkers = 0;
        workersPayCheck = laborUnitPrice * numWorkers;
        livingCost = 10.0;

        //factory relation
        TimeToDistribute = 360;

        //draw body
        scaleFactor = 0.3; //richness?
        bodyRadius = MapValue(capitalHoldings, 0, 100000.0, 3, 6);
        bodyHeight = bodyRadius * 3;
        mesh_Nv = addTetrahedron(body,bodyRadius);
        //mesh_Nv = addCone(body, bodyRadius, Vec3f(0,0,bodyHeight));
        for(int i=0; i<mesh_Nv; ++i){
			float f = float(i)/mesh_Nv;
			body.color(HSV(f*0.1,0.9,1));
		}
        body.decompress();
        body.generateNormals();
  
         //set up instrument
        gam::ArrayPow2<float> tbSaw(2048);
        gam::addSinesPow<1>(tbSaw, 9,1);

        gam::ArrayPow2<float> tbSin(2048);
        gam::addSine(tbSin);

        gam::ArrayPow2<float> tbSqr(2048);
        gam::addSinesPow<1>(tbSqr, 9,2);

        gam::ArrayPow2<float> tbOcean(2048); //bowl sound
        {    float A[] = {1, 0.4, 0.65, 0.3, 0.18, 0.08};
        float C[] = {1,4,7,11,15,18};
        gam::addSines(tbOcean, A,C,6);
        }
        gam::ArrayPow2<float> tbSizhu(2048);
        {    float A[] = {0.5,0.8,0.7,1,0.3,0.4,0.2,0.12}; // harmonic amplitudes of series
        float C[] = {3,4,7,8,11,12,15,16}; //cycles harmonic numbers of series
        gam::addSines(tbSizhu, A,C,8); //8 is number of harmonics, same with array size above
        }

        //sin A
        sin.freq(20);
        sin.source(tbSaw);
        sinDur = 0.25;
        baseFreq = 120;
        shiftFreq = rnd::uniform(baseFreq / 3, baseFreq / 6);

        sinADR.levels(0,0.2,0.2,0);
        sinADR.lengths(sinDur/6,sinDur/3,sinDur/6);
        //note length should be changed beneath
        sinADR.curve(-4);
        sinTmr.period(sinDur);


        delayDur = rnd::uniform(0.05,0.45);
        delay.maxDelay(0.5);	// Allocate delay-line memory for 400 ms
		delay.delay(delayDur);

        // //bigger
        // sinTmr2.period(sinDur * 8);
        
        // //amp env
        // ampEnv.levels(0,0.2,0.2,0);
        // ampEnv.lengths(12, 30, 18);
        // ampTmr.period(sinDur * 16);

    }
    virtual ~Capitalist(){

    }
    virtual void onProcess(AudioIOData& io) override{
        while (io()){
            
            if (sinTmr()){
                    sinADR.reset();
                    //tmrIndex = rnd::uniform(8,0);
                    //cout << tmrIndex << endl;
                    //sinADR.levels(0,0.2,0.2,0);
                    sinADR.lengths(sinDur/6,sinDur/3,sinDur/6);
                    //sinADR.curve(-4);
                    //flucFreq = floor(al::rnd::uniform(5, 75));
                    //finalFreq = freqs[index] + shiftFreq + flucFreq;
                    shiftFreq = capitalHoldings / 1000;
                    finalFreq = freqs[index] + shiftFreq;
                    //cout << finalFreq << endl;
                    sin.freq(finalFreq);
                    if (index == 0){
                        index ++;
                    } else if (index == 1){
                        index = 0;
                    }
                    //sinTmr.period(periods[rnd::uniform(8,0)]);
                    // index ++;
                    // if (index == 4){
                    //     index = 0;
                    // }

                    //sineD
                    //sine.set(gam::rnd::uni(10,1)*50, 0.2, gam::rnd::lin(2., 0.1));
                    //targetFreq = freqs[index] + shiftFreq + flucFreq;
                    //oldFreq = finalFreq;
                }

                // if (sinTmr2()){
                //     //sinTmr.period(sinDur / 2);
                //     // shiftFreq += baseFreq;
                //     // if (shiftFreq > baseFreq * 3) {
                //     //     shiftFreq = baseFreq * 0.5;
                //     // }
                    
                //     // sinDur = sinDur * 0.8;
                //     // sinTmr.period(sinDur);
                //     // if (sinDur < 1.0){
                //     //     sinDur = 6.0;
                //     // }
                // }
                // if (ampTmr()){
                //     ampEnv.reset();
                //     ampEnv.lengths(12, 30, 18);
                // }

                // float s = sine() * sinADR() * 0.2 + sin() * sinADR() * 0.2;//* ampEnv()
                
                float s = delay(sin() * 0.15 * sinADR());//* ampEnv()
                io.out(0) = isnan(s) ? 0.0 : (double)s;
        }
    }

 
    bool bankrupted(){
        if (capitalHoldings <= -50000) {
            return true;
        } else {
            return false;
        }
    }


    void draw(Graphics& g){

        g.pushMatrix();
        g.translate(pose().pos());
        g.rotate(pose().quat());
        g.scale(scaleFactor);
        if (!bankrupted()){
            g.meshColor();
            g.draw(body);
        }
        g.popMatrix();
    }
};


struct Miner : Agent {
    int mesh_Nv;
    Vec3f movingTarget;
    Vec3f temp_pos;

    bool resourcePointFound;
    float distToClosestNRP;
    float distToClosestResource;
    int id_ClosestNRP;
    int id_ClosestResource;
    float searchResourceForce;
    float collectResourceForce;
    float sensitivityNRP;
    float sensitivityResource;
    float pickingRange;
    float sensitivityCapitalist;
    float desireLevel;
    float separateForce;
    float resourceHoldings;
    int collectTimer;
    int tradeTimer;
    float distToClosestCapitalist;
    int id_ClosestCapitalist;
    bool capitalistNearby;
    int desireChangeRate;
    float businessDistance;
    int unloadTimeCost;
    float collectRate;
    float maxLoad;
    bool fullpack;
    float resourceUnitPrice;
    float bodyRadius;
    float bodyHeight;
    float numNeighbors;
    float neightSenseRange;
    float friendliness;
    float patienceLimit;
    float patienceTimer;
    bool exchanging;
    Mesh resource;

    //sin A
    gam::Osc<> sin;
    gam::Env<3> sinADR;
    gam::Accum<> sinTmr;
    float sinDur;
    float baseFreq;
    float periods[8] = {1.0, 1.5, 0.25,1.5,1.0, 2.5, 3.0, 4.0};
    float freqs[4] = {baseFreq * 2.4f, baseFreq * 2.4f, baseFreq *1.8f, baseFreq*1.2f};
    int index = 0;
    int tmrIndex = 0;

    //delay
    // gam::Delay<float, gam::ipl::Trunc> delay;
    // float delayDur;

    //bigger control
    // gam::Accum<> sinTmr2;
    // gam::Env<3> ampEnv;
    // gam::Accum<> ampTmr;
    float shiftFreq;
    float finalFreq;
    // float flucFreq;
    // float oldFreq;
    // float targetFreq;

    Miner(){
        //set up instrument
        // gam::ArrayPow2<float> tbSaw(2048);
        // gam::addSinesPow<1>(tbSaw, 9,1);

        gam::ArrayPow2<float> tbSin(2048);
        gam::addSine(tbSin);

        gam::ArrayPow2<float> tbSqr(2048);
        gam::addSinesPow<1>(tbSqr, 9,2);

        gam::ArrayPow2<float> tbOcean(2048);
        {    float A[] = {1, 0.4, 0.65, 0.3, 0.18, 0.08};
        float C[] = {1,4,7,11,15,18};
        gam::addSines(tbOcean, A,C,6);
        }
        gam::ArrayPow2<float> tb__2(2048);
        {    float A[] = {0.5,0.8,0.7,1,0.3,0.4,0.2,0.12}; // harmonic amplitudes of series
        float C[] = {3,4,7,8,11,12,15,16}; //cycles harmonic numbers of series
        gam::addSines(tb__2, A,C,8); //8 is number of harmonics, same with array size above
        }

        //sin A
        sin.freq(20);
        sin.source(tbOcean);
        sinDur = 1;
        baseFreq = 240;
        
        sinADR.levels(0,0.2,0.2,0);
        sinADR.lengths(sinDur/4,sinDur/2,sinDur/4);
        sinADR.curve(-4);
        sinTmr.period(sinDur + rnd::uniform(0.45, 0.05));
        //sinA baseFreqs
        shiftFreq = baseFreq * 0.5;
        finalFreq = baseFreq * 2 + shiftFreq;

        // delayDur = rnd::uniform(0.05,0.45);
        // delay.maxDelay(0.5);	// Allocate delay-line memory for 400 ms
		// delay.delay(delayDur);
        

        // flucFreq = baseFreq / 5.0f;
        // targetFreq = baseFreq;
        //bigger
        // sinTmr2.period(sinDur * 8);
        // //amp env
        
        // ampEnv.levels(0,0.2,0.2,0);
        // ampEnv.lengths(12, 30, 18);
        // ampTmr.period(sinDur * 16);

        //behavioral params start here
        maxAcceleration = 1;
        mass = 1.0;
        maxspeed = 0.5;
        minspeed = 0.1;
        maxforce = 0.03;
        initialRadius = 5;
        target_senseRadius = 10.0;
        desiredseparation = 3.0f;
        acceleration = Vec3f(0,0,0);
        velocity = Vec3f(0,0,0);
        pose().pos() = r();
        temp_pos = pose().pos();
        pose().pos() = pose().pos() * (NaturalRadius - FactoryRadius) + temp_pos.normalize(FactoryRadius + CirclePadding * 2.0);
        bioClock = 0;
        movingTarget = r();
        desireChangeRate = r_int(60, 90);

        //relation to resource point
        distToClosestNRP = 120.0f;
        distToClosestResource = 120.0f;
        id_ClosestNRP = 0;
        id_ClosestResource = 0;
        resourcePointFound = false;
        searchResourceForce = 1.0;
        collectResourceForce = 1.0;
        sensitivityNRP = 30.0;
        sensitivityResource = 8.0;
        pickingRange = 2.0;
        collectTimer = 0;
        collectRate = 0.5;
        maxLoad = 12.0;

        //relation to capitalist
        distToClosestCapitalist = 120.0f;
        id_ClosestCapitalist = 0;
        sensitivityCapitalist = 160.0f;
        capitalistNearby = false;
        businessDistance = 8.0f;
        tradeTimer = 0;
        unloadTimeCost = 30;
        exchanging = false;

        //capitals
        resourceHoldings = (float)r_int(0, 5);
        capitalHoldings = rnd::uniform(4500, 7000);
        poetryHoldings = 0.0;
        resourceUnitPrice = 120.0;
        livingCost = 1.0;

        //human nature
        desireLevel = 0.5;
        separateForce = 1.5;
        numNeighbors = 0.0;
        neightSenseRange = 10.0;
        friendliness = r_int(3,12);
        patienceTimer = 0;
        patienceLimit = (float)r_int(30, 120);

        //draw body
        scaleFactor = 0.3;
        bodyRadius = MapValue(capitalHoldings, 0, 100000.0, 1, 3);
        bodyHeight = bodyRadius * 3;
        mesh_Nv = addCone(body, bodyRadius, Vec3f(0,0,bodyHeight));
        for(int i=0; i<mesh_Nv; ++i){
			float f = float(i)/mesh_Nv;
			body.color(HSV(f*0.1+0.2,1,1));
		}
        addCube(resource, 4);
        resource.generateNormals();
        body.decompress();
        body.generateNormals();
    }

    virtual void onProcess(AudioIOData& io) override {
        while (io()){
            
            //finalFreq = (1 - sinTmr.phase()) * oldFreq + sinTmr.phase() * targetFreq;
            if (!fullpack){    
                if (sinTmr()){
                    sinADR.reset();
                    sinDur = 1;
                    sinTmr.period(sinDur * 1.5);
                    // tmrIndex = rnd::uniform(8,0);
                    //cout << tmrIndex << endl;
                    //sinADR.levels(0,0.2,0.2,0);
                    // sinADR.lengths(sinDur/8 * periods[tmrIndex],sinDur/4* periods[tmrIndex],sinDur/8* periods[tmrIndex]);
                    
                    //sinADR.lengths(sinDur/4,sinDur/2,sinDur/4);
                    sinADR.lengths(sinDur/6,sinDur/3,sinDur/6);
                    //sinADR.curve(-4);
                    // flucFreq = floor(al::rnd::uniform(5, 75));
                    shiftFreq = resourceHoldings * 20;
                    // finalFreq = freqs[index] + shiftFreq + flucFreq;
                    finalFreq = freqs[index] + shiftFreq;
                    sin.freq(finalFreq);
                    // sinTmr.period(periods[rnd::uniform(8,0)]);
                    if (index == 0){
                        index ++;
                    } else if (index == 1){
                        index = 0;
                    }




                    //do an FM???




                    
                    //targetFreq = freqs[index] + shiftFreq + flucFreq;
                    //oldFreq = finalFreq;
                }

                // if (sinTmr2()){
                //     //sinTmr.period(sinDur / 2);
                //     shiftFreq += baseFreq;
                //     if (shiftFreq > baseFreq * 3) {
                //         shiftFreq = baseFreq * 0.5;
                //     }
                    
                //     // sinDur = sinDur * 0.8;
                //     // sinTmr.period(sinDur);
                //     // if (sinDur < 1.0){
                //     //     sinDur = 6.0;
                //     // }
                // }
                // if (ampTmr()){
                //     ampEnv.reset();
                //     ampEnv.lengths(12, 30, 18);
                // }

                float s = sin() * sinADR() * 0.4;//* ampEnv()

                io.out(0) = isnan(s) ? 0.0 : (double)s;
            } else {

                if (sinTmr()){
                    sinADR.reset();
                    sinDur = 0.25;
                    sinTmr.period(sinDur);
                    //sinADR.levels(0,0.2,0.2,0);
                    sinADR.lengths(sinDur/4,sinDur/2,sinDur/4);
                    //sinADR.curve(-4);
                    // flucFreq = floor(al::rnd::uniform(5, 75));
                    // finalFreq = freqs[index] + shiftFreq + flucFreq;
                    shiftFreq = capitalHoldings / 10;
                    finalFreq = freqs[index] + shiftFreq;
                    //cout << finalFreq << endl;
                    sin.freq(finalFreq);
                    //sinTmr.period(periods[rnd::uniform(0,4)]);
                    // index ++;
                    // if (index == 4){
                    //     index = 0;
                    // }
                    if (index == 0){
                        index ++;
                    } else if (index == 1){
                        index = 0;
                    }
                    
                    //targetFreq = freqs[index] + shiftFreq + flucFreq;
                    //oldFreq = finalFreq;
                }

                // if (sinTmr2()){
                //     // shiftFreq += baseFreq;
                //     // if (shiftFreq > baseFreq * 3) {
                //     //     shiftFreq = baseFreq * 0.5;
                //     // }
                    
                //     // sinDur = sinDur * 0.8;
                //     // sinTmr.period(sinDur);
                //     // if (sinDur < 1.0){
                //     //     sinDur = 6.0;
                //     // }
                // }
                // if (ampTmr()){
                //     ampEnv.reset();
                //     ampEnv.lengths(12, 30, 18);
                // }

                float s = sin() * sinADR() * 0.4;//* ampEnv()

                io.out(0) = isnan(s) ? 0.0 : (double)s;
            }

        }
    }


     bool bankrupted(){
        if (capitalHoldings <= -1000) {
            return true;
        } else {
            return false;
        }
    }
 
    
    void draw(Graphics& g){
        g.pushMatrix();
        g.translate(pose().pos());
        g.rotate(pose().quat());
        g.scale(scaleFactor);
        if (!bankrupted()){
            g.meshColor();
            if (resourceHoldings >= 12){ g.draw(resource);}
            if (true){ g.draw(resource);}
            g.draw(body);
        }
        g.popMatrix();
    }

};

struct Worker : Agent {
    int mesh_Nv;
    Vec3f temp_pos;
    Vec3f workTarget;
    int desireChangeRate;
    float distToClosestFactory;
    int id_ClosestFactory;
    bool FactoryFound;
    float sensitivityFactory;
    float separateForce;
    float diligency;
    float mood;
    float workingDistance;
    float desireLevel;
    bool jobHunting;
    bool positionSecured;
    float patienceLimit;
    int patienceTimer;
    bool depression;
    float bodyRadius;
    float bodyHeight;
    int workerID;
    float neighborNum;
    float noiseLevel;

    //sin A
    gam::Osc<> sin;
    gam::Env<3> sinADR;
    gam::Accum<> sinTmr;
    float sinDur;
    float baseFreq;
    float freqs[4] = {baseFreq * 2, baseFreq * 2.2f, baseFreq * 2.4f, baseFreq *1.8f};
    float periods[8] = {0.5,2,0.25,1.5,1.0, 4.0, 6.0, 0.75};
    int tmrIndex = 0;
    int index = 0;

    //bigger control
    gam::Accum<> sinTmr2;
    float shiftFreq;
    float finalFreq;
    // float flucFreq;
    // float oldFreq;
    // float targetFreq;

    Worker(){
        maxAcceleration = 1;
        mass = 1.0;
        maxspeed = 0.3;
        minspeed = 0.1;
        maxforce = 0.03;
        initialRadius = 5;
        target_senseRadius = 10.0;
        desiredseparation = 3.0f;
        acceleration = Vec3f(0,0,0);
        velocity = Vec3f(0,0,0);
        pose().pos() = r();
        temp_pos = pose().pos();
        pose().pos() = pose().pos() * (FactoryRadius - MetroRadius) + temp_pos.normalize(MetroRadius);
        bioClock = 0;
        movingTarget = r();
        workTarget = r();

        //human nature
        desireLevel = 0.5;
        desireChangeRate = r_int(60, 60); //60 ~ 120
        diligency = rnd::uniform(0.7, 1.4); //0.7 ~ 1.4
        mood = r_int(30, 90);
        patienceLimit = (float)r_int(10, 90);
        patienceTimer = 0;

        //relation to factory
        distToClosestFactory = 200;
        id_ClosestFactory = 0;
        sensitivityFactory = 45;
        workingDistance = 8;
        jobHunting = true;
        positionSecured = false;
        FactoryFound = false;
        depression = false;

        //capitals
        capitalHoldings = (float)r_int(4000, 2000);
        poetryHoldings = 0.0;
        livingCost = 3.0;
        

        //draw body
        scaleFactor = 0.3;
        bodyRadius = MapValue(capitalHoldings, 0, 100000.0, 1, 3);
        bodyHeight = bodyRadius * 3;
        mesh_Nv = addCone(body, bodyRadius, Vec3f(0,0,bodyHeight));
        for(int i=0; i<mesh_Nv; ++i){
			float f = float(i)/mesh_Nv;
			body.color(HSV(f*0.2+0.4,1,1));
		}
        body.decompress();
        body.generateNormals();


        //set up instrument
         gam::ArrayPow2<float>
        tbSaw(2048), tbSqr(2048), tbImp(2048), tbSin(2048), tbPls(2048),
        tb__1(2048), tb__2(2048), tb__3(2048), tb__4(2048);

    gam::addSinesPow<1>(tbSaw, 9,1);
    gam::addSinesPow<1>(tbSqr, 9,2);
    gam::addSinesPow<0>(tbImp, 9,1);
    gam::addSine(tbSin);

    {    float A[] = {1,1,1,1,0.7,0.5,0.3,0.1};
        gam::addSines(tbPls, A,8);
    }

    {    float A[] = {1, 0.4, 0.65, 0.3, 0.18, 0.08};
        float C[] = {1,4,7,11,15,18};
        gam::addSines(tb__1, A,C,6);
    }

    // inharmonic partials
    {    float A[] = {0.5,0.8,0.7,1,0.3,0.4,0.2,0.12}; // harmonic amplitudes of series
        float C[] = {3,4,7,8,11,12,15,16}; //cycles harmonic numbers of series
        gam::addSines(tb__2, A,C,8); //8 is number of harmonics, same with array size above
    }

    // inharmonic partials
    {    float A[] = {1, 0.7, 0.45, 0.3, 0.15, 0.08};
        float C[] = {10, 27, 54, 81, 108, 135};
        gam::addSines(tb__3, A,C,6);
    }

    // harmonics 20-27
    {    float A[] = {0.2, 0.4, 0.6, 1, 0.7, 0.5, 0.3, 0.1};
        gam::addSines(tb__4, A,8, 20);
    }

        //sin A
        sin.freq(30);
        sin.source(tb__4);
        //sinDur = 4.0;
        sinDur = 0.5f;
        baseFreq = 120;
        sinADR.levels(0,0.2,0.2,0);
        sinADR.lengths(sinDur/4, sinDur/2,sinDur/4);
        sinADR.curve(-4);
        sinTmr.period(sinDur * 4 + rnd::uniform(sinDur / 2.0f, 0.0f));
        //sinA baseFreqs
        shiftFreq = baseFreq * 0.5;
        finalFreq = baseFreq * 2 + shiftFreq;
        // flucFreq = baseFreq / 5.0f;
        // targetFreq = baseFreq;
        // //bigger
        // sinTmr2.period(sinDur * 12);
        

        
    }
    // void noiseLevelUpdate(){
    //     noiseLevel = MapValue(neighborNum, 0, 100, 0.01, 0.4);
    // }

    virtual void onProcess(AudioIOData& io) override {
        while (io()){
            
            if (jobHunting){
                //finalFreq = (1 - sinTmr.phase()) * oldFreq + sinTmr.phase() * targetFreq;
                //sin.freq(finalFreq);
                if (sinTmr()){
                    sinADR.reset();
                    
                    //sinADR.levels(0,0.2,0.2,0);
                    // tmrIndex = rnd::uniform(8,0);
                    //sinADR.lengths(sinDur * periods[tmrIndex] /4 ,sinDur * periods[tmrIndex] /2,sinDur  * periods[tmrIndex] /4);
                    sinDur = 0.5f;
                    sinADR.lengths(sinDur / 6, sinDur / 3, sinDur / 6);
                    sinTmr.period(sinDur);
                    //sinADR.curve(-4);
                    //flucFreq = floor(al::rnd::uniform(75, 10));
                    shiftFreq = capitalHoldings / 30;
                    finalFreq = freqs[index] + shiftFreq;

                    index ++;
                    if (index == 4){
                        index = 0;
                    }
                    // targetFreq = freqs[index] + shiftFreq + flucFreq;
                    // oldFreq = finalFreq;
                    sin.freq(finalFreq);
                    
                    sinTmr.period(periods[rnd::uniform(8,0)]);
                }

                // if (sinTmr2()){
                //     shiftFreq += baseFreq;
                //     if (shiftFreq > baseFreq * 4) {
                //         shiftFreq = baseFreq * 0.5;
                //     }
                //     // sinDur = sinDur * 0.8;
                //     // sinTmr.period(sinDur);
                //     // if (sinDur < 1.0){
                //     //     sinDur = 6.0;
                //     // }
                // }

                float s = sin() * sinADR() * 0.2;
                io.out(0) = isnan(s) ? 0.0 : (double)s;
            } else {
                //finalFreq = (1 - sinTmr.phase()) * oldFreq + sinTmr.phase() * targetFreq;
                
                if (sinTmr()){
                    sinADR.reset();
                    
                    //sinADR.levels(0,0.2,0.2,0);
                    // tmrIndex = rnd::uniform(8,0);
                    //sinADR.lengths(sinDur * periods[tmrIndex] /4 ,sinDur * periods[tmrIndex] /2,sinDur  * periods[tmrIndex] /4);
                    sinDur = 5.0f;
                    sinADR.lengths(sinDur / 6, sinDur / 3, sinDur / 6);
                    sinTmr.period(sinDur);
                    //sinADR.curve(-4);
                    //flucFreq = floor(al::rnd::uniform(75, 10));
                    shiftFreq = capitalHoldings / 10;
                    finalFreq = freqs[index] + shiftFreq;

                    index ++;
                    if (index == 4){
                        index = 0;
                    }
                    // targetFreq = freqs[index] + shiftFreq + flucFreq;
                    // oldFreq = finalFreq;
                    sin.freq(finalFreq);
                    
                    sinTmr.period(periods[rnd::uniform(8,0)]);
                }

                // if (sinTmr2()){
                //     shiftFreq += baseFreq;
                //     if (shiftFreq > baseFreq * 4) {
                //         shiftFreq = baseFreq * 0.5;
                //     }
                //     // sinDur = sinDur * 0.8;
                //     // sinTmr.period(sinDur);
                //     // if (sinDur < 1.0){
                //     //     sinDur = 6.0;
                //     // }
                // }

                float s = sin() * sinADR() * 0.2;
                io.out(0) = isnan(s) ? 0.0 : (double)s;
            }
            
    

        }
    }
 
    bool bankrupted(){
        if (capitalHoldings <= -2000){
            return true;
        } else {
            return false;
        }
    }
  
    void draw(Graphics& g){
        g.pushMatrix();
        g.translate(pose().pos());
        g.rotate(pose().quat());
        g.scale(scaleFactor);
        if (!bankrupted()){
            g.meshColor();
            g.draw(body);
        }
        g.popMatrix();
    }


};



// #include "locations.hpp"

#endif
