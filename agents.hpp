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


// SampleLooper from Karl
//
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

class Vibrato{
public:
	Vibrato(float modAmount=1./400, float modFreq=5)
	:	modAmount(modAmount),
		delay(0.1, 0), mod(modFreq)
	{}
	
	float operator()(float i){
		delay.delay(mod.hann()*modAmount + 0.0001);
		return delay(i);
	}

	float modAmount;
	gam::Delay<> delay;
	gam::LFO<> mod;
};

// struct Resource;
// struct Factory;
// struct Natural_Resource_Point;
// struct MetroBuilding;
struct Capitalist : Agent{

    int mesh_Nv;
    Vec3f movingTarget;
    int desireChangeRate;
    float resourceHoldings;
    int TimeToDistribute;
    int resourceClock;
    float workersPayCheck;
    float laborUnitPrice;
    float resourceUnitPrice;
    int numWorkers;
    int capitalistID;
    float bodyRadius;
    float bodyHeight;
    float totalResourceHoldings;

    //audio params
    // SoundSource *soundSource;
//    using Agent::pose;
    // float oscPhase = 0;
    // float oscEnv = 1;
    // float rate;
    //double audioTimer;
    // gam::SamplePlayer<float, gam::ipl::Linear, gam::phsInc::Loop> player;
    // gam::OnePole<> smoothRate;
    // DynamicSamplePlayer player;

    //gamma effects
    //gam::LFO<> osc;
    gam::SineD<> sine;
	// gam::LFO<> shiftMod;
    // gam::LFO<> mod;
	// gam::Hilbert<> hil;
	// gam::CSine<> shifter;
    // // gam::Biquad<> bq;
    // gam::OnePole<> onePole;
    // gam::Accum<> tmr;
    // gam::NoisePink<> s_noise;
    // gam::Delay<float, gam::ipl::Trunc> delay;
    // Vibrato vibrato;
    // float baseFreq;

    gam::Osc<> sin;
        gam::Env<3> sinADR;
        gam::Accum<> sinTmr;
        float sinDur = 0.6f;
        float baseFreq = 180.0f;
        float periods[8] = {0.5,2,0.25,1.5,1.0, 4.0, 8.0, 3.0};
        float freqs[4] = {baseFreq * 1.6f, baseFreq * 2.4f, baseFreq *1.8f, baseFreq*1.2f};
        int index = 0;
        int tmrIndex = 0;

        //bigger control
        gam::Accum<> sinTmr2;
        gam::Env<3> ampEnv;
        gam::Accum<> ampTmr;
        float shiftFreq;
        float finalFreq;
        float flucFreq;
        float oldFreq;
        float targetFreq;    


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
        desireChangeRate = r_int(50, 150);

        //capitals
        resourceHoldings = (float)r_int(0, 10);
        totalResourceHoldings = resourceHoldings;
        capitalHoldings = 25000.0;
        poetryHoldings = 0.0;
        laborUnitPrice = 420.0;
        resourceUnitPrice = 280.0;
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

        //effects
        //sine
        baseFreq = rnd::uniform(110, 880);
        sine.freq(20);
        
        // //for sample
        // //smoothRate.freq(3.14159);
        // //smoothRate = -2.5;
        // //smoothRate = 0.07;

        // //for hilbert
		// shiftMod.period(16);
        // shifter.freq(baseFreq / 4);

        // //for one pole
        // mod.period(120);
        // mod.phase(0.5);
		
        // //biquad
        // // bq.res(4);
        // // bq.level(2);

        // //delay
        // tmr.period();
        // tmr.phaseMax();
        // // delay.maxDelay(0.4);
        // // delay.delay(0.2);

       

         //set up instrument
        // gam::ArrayPow2<float> tbSaw(2048);
        // gam::addSinesPow<1>(tbSaw, 9,1);

        gam::ArrayPow2<float> tbSin(2048);
        gam::addSine(tbSin);

        // gam::ArrayPow2<float> tbSqr(2048);
        // gam::addSinesPow<1>(tbSqr, 9,2);

        // gam::ArrayPow2<float> tbOcean(2048);
        // {    float A[] = {1, 0.4, 0.65, 0.3, 0.18, 0.08};
        // float C[] = {1,4,7,11,15,18};
        // gam::addSines(tbOcean, A,C,6);
        // }
        gam::ArrayPow2<float> tb__2(2048);
        {    float A[] = {0.5,0.8,0.7,1,0.3,0.4,0.2,0.12}; // harmonic amplitudes of series
        float C[] = {3,4,7,8,11,12,15,16}; //cycles harmonic numbers of series
        gam::addSines(tb__2, A,C,8); //8 is number of harmonics, same with array size above
        }

        //sin A
        sin.freq(20);
        sin.source(tb__2);
        //sinDur = 4.0;
        
        sinADR.levels(0,0.2,0.2,0);
        sinADR.lengths(sinDur/4,sinDur/2,sinDur/2);
        sinADR.curve(-4);
        sinTmr.period(sinDur * 4);
        //sinA baseFreqs
        shiftFreq = baseFreq * 0.5;
        finalFreq = baseFreq * 2 + shiftFreq;
        flucFreq = baseFreq / 5.0f;
        targetFreq = baseFreq;
        //bigger
        sinTmr2.period(sinDur * 8);
        //amp env
        
        ampEnv.levels(0,0.2,0.2,0);
        ampEnv.lengths(12, 30, 18);
        ampTmr.period(sinDur * 16);

    }
    virtual ~Capitalist(){

    }
    virtual void onProcess(AudioIOData& io) override{
        while (io()){
            //player.rate(smoothRate());
            // if (tmr()){
            //     //source = player();
            //    sine.set(gam::rnd::uni(10,1)*50, 0.2, gam::rnd::lin(2., 0.1)); 
            // }
            // float source = sine();
            // float sineClick = sine();
            // //experimental area
            
            // //hilbert transformation
            // gam::Complex<float> c = hil(source);
            // shifter.freq(shiftMod.hann()*200);
		    // c *= shifter();
            // float sr = c.r;
            // float si = c.i;

            // //one pole
            // float cutoff = gam::scl::pow3(mod.triU()) * 2000;
            // onePole.freq(1000 + cutoff * 0.2);
            // //float s = onePole(sr) * 0.3 + onePole(si) * 0.3;
            
            // //float s = onePole(sr + si) * 0.2 + s_noise() * gam::scl::pow3(mod.triU()) * 0.06;
            // float s = onePole(sr + si) * 0.2;
            // s = vibrato(s);

            //biquad
            // bq.type(gam::BAND_PASS);
            // bq.freq(500 + cutoff * 0.08);
            // float sample = s * 0.7 + sineClick * 0.3;

            if (sinTmr()){
                    sinADR.reset();
                    tmrIndex = rnd::uniform(8,0);
                    //cout << tmrIndex << endl;
                    //sinADR.levels(0,0.2,0.2,0);
                    sinADR.lengths(sinDur/8 * periods[tmrIndex],sinDur/4* periods[tmrIndex],sinDur/8* periods[tmrIndex]);
                    //sinADR.curve(-4);
                    flucFreq = floor(al::rnd::uniform(5, 75));
                    finalFreq = freqs[index] + shiftFreq + flucFreq;
                    sin.freq(finalFreq);
                    sinTmr.period(periods[rnd::uniform(8,0)]);
                    index ++;
                    if (index == 4){
                        index = 0;
                    }
                    //sineD
                    sine.set(gam::rnd::uni(10,1)*50, 0.2, gam::rnd::lin(2., 0.1));
                    //targetFreq = freqs[index] + shiftFreq + flucFreq;
                    //oldFreq = finalFreq;
                }

                if (sinTmr2()){
                    //sinTmr.period(sinDur / 2);
                    shiftFreq += baseFreq;
                    if (shiftFreq > baseFreq * 3) {
                        shiftFreq = baseFreq * 0.5;
                    }
                    
                    // sinDur = sinDur * 0.8;
                    // sinTmr.period(sinDur);
                    // if (sinDur < 1.0){
                    //     sinDur = 6.0;
                    // }
                }
                if (ampTmr()){
                    ampEnv.reset();
                    ampEnv.lengths(12, 30, 18);
                }

                float s = sine() * sinADR() * 0.2 + sin() * sinADR() * 0.2;//* ampEnv()

                io.out(0) = isnan(s) ? 0.0 : (double)s;

            // io.out(0) = isnan(sample) ? 0.0 : (double)sample;
//            return sample;
            //delay
            // if (tmr()) {
            //     sample = bq(s);
            // }
            // sample += delay(sample + delay()*0.2);
            //sample += delay(sample) + delay.read(0.15) + delay.read(0.39);
            
            //write sample
            //soundSource->writeSample(sample);
            //soundSource->writeSample(source * 0.01); 

            //for bypass only
            //io.out(0) += sample;
            //io.out(1) += sample;
        }
    }

    void run(vector<MetroBuilding>& mbs){
        //cout << capitalHoldings << "i m capitalist" << endl;
        //cout << "capitalistss??" << endl;
        //basic behaviors
        Vec3f ahb(avoidHittingBuilding(mbs));
        ahb *= 0.8;
        applyForce(ahb);

        //factory related
        distributeResources();

        //default behaviors
        borderDetect();
        inherentDesire(0.5, MetroRadius * 0.6, MetroRadius * 2, desireChangeRate);
        facingToward(movingTarget);
        update();
        moneyConsumption();
        //updateSamplePlayer();
    }
    // void updateSamplePlayer(){
    //     audioTimer += 1 / 60;
    //     if (audioTimer > 5.0) {
    //         audioTimer -= 5.0;
    //         float begin, end;
    //         for (int t = 0; t < 100; t++) {
    //             begin = rnd::uniform(player.frames());
    //             end = rnd::uniform(player.frames());

    //             if (abs(player[int(begin)] - player[int(end)]) < 0.125) break;
    //         }
    //         if (begin > end) {
    //             float t = begin;
    //             begin = end;
    //             end = t;
    //         }
    //         // tell the player the begin and end points
    //         //
    //         player.min(begin);
    //         player.max(end);
    //         // set playback rate. negative rates play in reverse.
    //         //
    //         float r = pow(2, rnd::uniformS(1.0f));
    //         if (rnd::prob(0.3)) r *= -1;
    //         smoothRate = r;

    //         // start sample from beginning
    //         //
    //         player.reset();
    //     }
    // }
    // void updateAuidoPose(){
    //     //audio
    //     soundSource->pose(Agent::pose);
    // }
    void moneyConsumption(){
        moneyTimer ++;
        if (moneyTimer == 0){
            lastSavings = capitalHoldings;
            monthlyTotal = 0;
        }
        if (moneyTimer % 60 == 0){
            currentSavings = capitalHoldings;
            todayIncome = currentSavings - lastSavings;
            monthlyTotal += todayIncome;
            lastSavings = currentSavings;
        }
        if (moneyTimer > 60 * 15){
            monthlyIncome = monthlyTotal;
            dailyIncome = monthlyIncome / 30;
            moneyTimer = 0;
        }
        //cout << dailyIncome << " daily income " << endl;
        //cout << monthlyIncome << " monthly income " << endl;
        if (monthlyIncome > 5000 && monthlyIncome <= 8000){
            incomeTax = dailyIncome * 0.008;
            livingCost = 8.0;
        } else if (monthlyIncome > 8000 && monthlyIncome <= 15000){
            incomeTax = dailyIncome * 0.012;
            livingCost = 9.0;
        } else if (monthlyIncome > 15000 ){
            incomeTax = dailyIncome * 0.018;
            livingCost = 10.0;
        } else if (monthlyIncome <= 5000){
            incomeTax = 0;
            livingCost = 6.0;
        }

        capitalHoldings -= livingCost + incomeTax;

        if (capitalHoldings <= -50000){
            capitalHoldings = -50000;
        } else if (capitalHoldings >= 9999999){
            capitalHoldings = 9999999;
        }
    }
    void distributeResources(){
        resourceClock ++;
        //every 12 seconds, half a day, distribute resource
        if (resourceClock == TimeToDistribute) {
            resourceHoldings = 0;
            capitalHoldings -= workersPayCheck;
            resourceClock = 0;
        }
    }

    void learnPoems(){

    }
    bool bankrupted(){
        if (capitalHoldings <= -50000) {
            return true;
        } else {
            return false;
        }
    }

    Vec3f avoidHittingBuilding(vector<MetroBuilding>& mbs){
        Vec3f sum;
        int count = 0;
        for (MetroBuilding mb : mbs){
            Vec3f difference = pose().pos() - mb.position;
            float d = difference.mag();
            if ((d > 0) && (d < desiredseparation * mb.scaleFactor)){
                Vec3d diff = difference.normalize();
                sum += diff;
                count++;
            }
        }
        if (count > 0){
            sum /= count;
            sum.mag(maxspeed);
            Vec3f steer = sum - velocity;
            if (steer.mag() > maxforce) {
                steer.normalize(maxforce);
            }
            return steer;
        } else {
            return Vec3f(0,0,0);
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
    float sinDur = 0.3f;
    float baseFreq = 180.0f;
    float periods[8] = {0.5,2,0.25,1.5,1.0, 2.5, 3.0, 4.0};
    float freqs[4] = {baseFreq * 1.6f, baseFreq * 2.4f, baseFreq *1.8f, baseFreq*1.2f};
    int index = 0;
    int tmrIndex = 0;

    //bigger control
    gam::Accum<> sinTmr2;
    gam::Env<3> ampEnv;
    gam::Accum<> ampTmr;
    float shiftFreq;
    float finalFreq;
    float flucFreq;
    float oldFreq;
    float targetFreq;

    Miner(){
        //set up instrument
        // gam::ArrayPow2<float> tbSaw(2048);
        // gam::addSinesPow<1>(tbSaw, 9,1);

        gam::ArrayPow2<float> tbSin(2048);
        gam::addSine(tbSin);

        // gam::ArrayPow2<float> tbSqr(2048);
        // gam::addSinesPow<1>(tbSqr, 9,2);

        // gam::ArrayPow2<float> tbOcean(2048);
        // {    float A[] = {1, 0.4, 0.65, 0.3, 0.18, 0.08};
        // float C[] = {1,4,7,11,15,18};
        // gam::addSines(tbOcean, A,C,6);
        // }
        // gam::ArrayPow2<float> tb__2(2048);
        // {    float A[] = {0.5,0.8,0.7,1,0.3,0.4,0.2,0.12}; // harmonic amplitudes of series
        // float C[] = {3,4,7,8,11,12,15,16}; //cycles harmonic numbers of series
        // gam::addSines(tb__2, A,C,8); //8 is number of harmonics, same with array size above
        // }

        //sin A
        sin.freq(20);
        sin.source(tbSin);
        //sinDur = 4.0;
        
        sinADR.levels(0,0.2,0.2,0);
        sinADR.lengths(sinDur/4,sinDur/2,sinDur/2);
        sinADR.curve(-4);
        sinTmr.period(sinDur * 4);
        //sinA baseFreqs
        shiftFreq = baseFreq * 0.5;
        finalFreq = baseFreq * 2 + shiftFreq;
        flucFreq = baseFreq / 5.0f;
        targetFreq = baseFreq;
        //bigger
        sinTmr2.period(sinDur * 8);
        //amp env
        
        ampEnv.levels(0,0.2,0.2,0);
        ampEnv.lengths(12, 30, 18);
        ampTmr.period(sinDur * 16);

        


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
        resourceHoldings = 0.0;
        capitalHoldings = 5000.0;
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
                    tmrIndex = rnd::uniform(8,0);
                    //cout << tmrIndex << endl;
                    //sinADR.levels(0,0.2,0.2,0);
                    sinADR.lengths(sinDur/8 * periods[tmrIndex],sinDur/4* periods[tmrIndex],sinDur/8* periods[tmrIndex]);
                    //sinADR.curve(-4);
                    flucFreq = floor(al::rnd::uniform(5, 75));
                    finalFreq = freqs[index] + shiftFreq + flucFreq;
                    sin.freq(finalFreq);
                    sinTmr.period(periods[rnd::uniform(8,0)]);
                    index ++;
                    if (index == 4){
                        index = 0;
                    }
                    
                    //targetFreq = freqs[index] + shiftFreq + flucFreq;
                    //oldFreq = finalFreq;
                }

                if (sinTmr2()){
                    //sinTmr.period(sinDur / 2);
                    shiftFreq += baseFreq;
                    if (shiftFreq > baseFreq * 3) {
                        shiftFreq = baseFreq * 0.5;
                    }
                    
                    // sinDur = sinDur * 0.8;
                    // sinTmr.period(sinDur);
                    // if (sinDur < 1.0){
                    //     sinDur = 6.0;
                    // }
                }
                if (ampTmr()){
                    ampEnv.reset();
                    ampEnv.lengths(12, 30, 18);
                }

                float s = sin() * sinADR() * 0.35;//* ampEnv()

                io.out(0) = isnan(s) ? 0.0 : (double)s;
            } else {

                if (sinTmr()){
                    sinADR.reset();
                    
                    //sinADR.levels(0,0.2,0.2,0);
                    sinADR.lengths(sinDur/4,sinDur/2,sinDur/4);
                    //sinADR.curve(-4);
                    flucFreq = floor(al::rnd::uniform(5, 75));
                    finalFreq = freqs[index] + shiftFreq + flucFreq;
                    sin.freq(finalFreq);
                    sinTmr.period(periods[rnd::uniform(0,4)]);
                    index ++;
                    if (index == 4){
                        index = 0;
                    }
                    
                    //targetFreq = freqs[index] + shiftFreq + flucFreq;
                    //oldFreq = finalFreq;
                }

                if (sinTmr2()){
                    shiftFreq += baseFreq;
                    if (shiftFreq > baseFreq * 3) {
                        shiftFreq = baseFreq * 0.5;
                    }
                    
                    // sinDur = sinDur * 0.8;
                    // sinTmr.period(sinDur);
                    // if (sinDur < 1.0){
                    //     sinDur = 6.0;
                    // }
                }
                if (ampTmr()){
                    ampEnv.reset();
                    ampEnv.lengths(12, 30, 18);
                }

                float s = sin() * sinADR() * 0.35;//* ampEnv()

                io.out(0) = isnan(s) ? 0.0 : (double)s;
            }

        }
    }

    void run(vector<Natural_Resource_Point>& nrps, vector<Miner>& others, vector<Capitalist>& capitalists){
        if (resourceHoldings < maxLoad){
            fullpack = false;
            //resource mining
            patienceTimer += 1;
            if (patienceTimer == patienceLimit){
                if (numNeighbors > friendliness){
                senseFruitfulPoints(nrps);
                } else {
                senseResourcePoints(nrps);
                }
                patienceTimer = 0;
            }
            
            if (resourcePointFound == true){
                separateForce = 1.2;
                if (distToClosestNRP > sensitivityResource){
                    seekResourcePoint(nrps);
                } else if (distToClosestNRP < sensitivityResource && distToClosestResource >= 0) {
                    collectResource(nrps);
                    if (distToClosestResource < pickingRange){
                        collectTimer ++;
                        if (collectTimer % (int)floorf(60.0 / collectRate) == 0){
                            resourceHoldings += 1;
                            //cout << collectTimer << endl;
                        }
                        if (collectTimer >= (int)floorf(60.0 / collectRate) * 12 - 1){
                            collectTimer = 0;
                        }
                    }
                }
            } else {
                separateForce = 0.3;
                inherentDesire(desireLevel, FactoryRadius, NaturalRadius, desireChangeRate);
                facingToward(movingTarget);
            } 
        } else if (resourceHoldings >= maxLoad) {
            fullpack = true;
            //find capitalist for a trade
            resourcePointFound = false;
            senseCapitalists(capitalists);
            if (capitalistNearby){
                if (distToClosestCapitalist > businessDistance){
                    seekCapitalist(capitalists);
                } else if (distToClosestCapitalist <= businessDistance && distToClosestCapitalist >= 0){
                    exchangeResource(capitalists);
                    exchanging = true;
                    tradeTimer ++;
                }
            } else {
                inherentDesire(desireLevel, FactoryRadius, NaturalRadius, desireChangeRate);
                facingToward(movingTarget);
            }
            if (tradeTimer == unloadTimeCost){
                //cout << "transaction finished" << endl;
                capitalHoldings += resourceUnitPrice * resourceHoldings;
                resourceHoldings = 0;
                exchanging = false;
                tradeTimer = 0;
            }

        }
        
        // cout << resourcePointFound << "found??" << endl;
        // cout << distToClosestNRP << " dist to closeset nrp"<< endl;
        // cout<< id_ClosestNRP << "  NRP" << endl;
        // cout << id_ClosestResource << " resource" << endl;

        //default behaviors
        Vec3f sep(separate(others));
        sep *= separateForce;
        applyForce(sep);     

        borderDetect();
        update();
        moneyConsumption();
    }
   
    void senseResourcePoints(vector<Natural_Resource_Point>& nrps){
        float min = 999;
        int min_id = 0;
        for (size_t i = 0; i < nrps.size(); i++){
            if (!nrps[i].drained()){
                Vec3f dist_difference = pose().pos() - nrps[i].position;
                float dist = dist_difference.mag();
                if (dist < min){
                    min = dist;
                    min_id = i;
                        //update universal variable for other functions to use
                    distToClosestNRP = dist;
                    id_ClosestNRP = min_id;
                }
            }
        }
        if (distToClosestNRP < sensitivityNRP){
            resourcePointFound = true;
        } else {
            resourcePointFound = false;
        }
    }

    void senseFruitfulPoints(vector<Natural_Resource_Point>& nrps){
        float maxFruitfulness = 0;
        int max_id = 0;     
        for (size_t i = 0; i < nrps.size(); i++){
            if (!nrps[i].drained()){
                Vec3f dist_difference = pose().pos() - nrps[i].position;
                float dist = dist_difference.mag();
                if (dist < sensitivityNRP){
                    if (nrps[i].fruitfulness > maxFruitfulness){
                        maxFruitfulness = nrps[i].fruitfulness;
                        max_id = i;
                        
                        //update universal variable for other functions to use
                        distToClosestNRP = dist;
                        id_ClosestNRP = max_id;
                    }
                }
            }
        }
        if (distToClosestNRP < sensitivityNRP){
            resourcePointFound = true;
        } else {
            resourcePointFound = false;
        }
    }
    void senseCapitalists(vector<Capitalist>& capitalists){
        float min_resources = 999999;
        float max_capitals = 0;
        int min_resource_id = 0;
        int max_rich_id = 0;
        for (size_t i = 0; i < capitalists.size(); i++){
            if (!capitalists[i].bankrupted()){

                //find the one needs resource
                if (capitalists[i].totalResourceHoldings < min_resources){
                    min_resources = capitalists[i].totalResourceHoldings;
                    min_resource_id = i;
                }
                //also find the one who is richest
                if (capitalists[i].capitalHoldings > max_capitals){
                    max_capitals = capitalists[i].capitalHoldings;
                    max_rich_id = i;
                }
            }
        }
        Vec3f dist_difference = pose().pos() - capitalists[min_resource_id].pose().pos();
        float dist_resource = dist_difference.mag();
        Vec3f dist_difference_2 = pose().pos() - capitalists[max_rich_id].pose().pos();
        float dist_rich = dist_difference_2.mag();
        
        if (dist_resource > dist_rich){
            distToClosestCapitalist = dist_resource;
            id_ClosestCapitalist = min_resource_id;
        } else if (dist_resource <= dist_rich){
            distToClosestCapitalist = dist_rich;
            id_ClosestCapitalist = max_rich_id;
        }

        if (distToClosestCapitalist < sensitivityCapitalist){
            capitalistNearby = true;
        } else {
            capitalistNearby = false;
        }
    }
    void seekResourcePoint(vector<Natural_Resource_Point>& nrps){
        if (!nrps[id_ClosestNRP].drained()){
            Vec3f skNRP(seek(nrps[id_ClosestNRP].position));
            skNRP *= searchResourceForce;
            applyForce(skNRP);
            facingToward(nrps[id_ClosestNRP].position);
        } 
    }
    void seekCapitalist(vector<Capitalist>& capitalists){
        if (!capitalists[id_ClosestCapitalist].bankrupted()){
            Vec3f skCP(seek(capitalists[id_ClosestCapitalist].pose().pos()));
            skCP *= 1.0;
            applyForce(skCP);
            Vec3f t = capitalists[id_ClosestCapitalist].pose().pos();
            facingToward(t);
        }
    }
    void exchangeResource(vector<Capitalist>& capitalists){
        Vec3f t = capitalists[id_ClosestCapitalist].pose().pos();
        Vec3f arCP(arrive(t));
        arCP *= 1.0;
        applyForce(arCP);
    }

    void collectResource(vector<Natural_Resource_Point>& nrps){
        float min = 9999;
        int min_id = 0;
        if (!nrps[id_ClosestNRP].drained()){
            for (int i = nrps[id_ClosestNRP].resources.size() - 1; i >= 0; i--){
                if (!nrps[id_ClosestNRP].resources[i].isPicked){
                    Vec3f dist_difference = pose().pos() - nrps[id_ClosestNRP].resources[i].position;
                    double dist = dist_difference.mag();
                    if (dist < min){
                        min = dist;
                        min_id = i;
                        id_ClosestResource = min_id;
                        distToClosestResource = dist;
                    }
                }
            }
            Vec3f collectNR(seek(nrps[id_ClosestNRP].resources[min_id].position));
            collectNR *= collectResourceForce;
            applyForce(collectNR);
            facingToward(nrps[id_ClosestNRP].resources[min_id].position);
        }
        findPoems(); 
    }
    void findPoems(){
        //30% probability
        if (rnd::prob(0.0001)) {
            poetryHoldings += 1;
        };
    }
    void moneyConsumption(){
        moneyTimer ++;
        if (moneyTimer == 0){
            lastSavings = capitalHoldings;
            monthlyTotal = 0;
        }
        if (moneyTimer % 60 == 0){
            currentSavings = capitalHoldings;
            todayIncome = currentSavings - lastSavings;
            monthlyTotal += todayIncome;
            lastSavings = currentSavings;
        }
        if (moneyTimer > 60 * 15){
            monthlyIncome = monthlyTotal;
            dailyIncome = monthlyIncome / 30;
            moneyTimer = 0;
        }
        if (monthlyIncome > 5000 && monthlyIncome <= 8000){
            incomeTax = dailyIncome * 0.008;
        } else if (monthlyIncome > 8000 && monthlyIncome <= 15000){
            incomeTax = dailyIncome * 0.012;
        } else if (monthlyIncome > 15000 ){
            incomeTax = dailyIncome * 0.018;
        } else if (monthlyIncome <= 5000){
            incomeTax = 0;
            povertyWelfare = - livingCost * 0.3;
        }

        capitalHoldings -= livingCost + incomeTax + povertyWelfare;
        if (capitalHoldings <= -1000){
            capitalHoldings = -1000;
        } else if (capitalHoldings >= 9999999){
            capitalHoldings = 9999999;
        }
    }
     bool bankrupted(){
        if (capitalHoldings <= -1000) {
            return true;
        } else {
            return false;
        }
    }
    Vec3f separate(vector<Miner>& others){
        Vec3f sum;
        int count = 0;
        int neighborCount = 0;
        for (Miner m : others){
            Vec3f difference = pose().pos() - m.pose().pos();
            float d = difference.mag();
            if ((d > 0) && (d < desiredseparation)){
                Vec3f diff = difference.normalize();
                sum += diff;
                count ++;
            }
            if ((d >0 ) && (d < neightSenseRange)){
                neighborCount ++;
            }
        }
        numNeighbors = neighborCount;
        //cout << numNeighbors << " num neighbors" << endl;
        if (count > 0){
            sum /= count;
            sum.mag(maxspeed);
            Vec3f steer = sum - velocity;
            if (steer.mag() > maxforce){
                steer.normalize(maxforce);
            }
            return steer;
        } else {
            return Vec3f(0,0,0);
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
    float baseFreq = 50.0f;
    float freqs[4] = {baseFreq * 2, baseFreq * 4, baseFreq *3, baseFreq *2.5f};
    float periods[8] = {0.5,2,0.25,1.5,1.0, 4.0, 6.0, 0.75};
    int tmrIndex = 0;
    int index = 0;

    //bigger control
    gam::Accum<> sinTmr2;
    float shiftFreq;
    float finalFreq;
    float flucFreq;
    float oldFreq;
    float targetFreq;

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
        capitalHoldings = 4000.0;
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
        gam::ArrayPow2<float> tbSaw(2048);
        gam::addSinesPow<1>(tbSaw, 9,1);

        gam::ArrayPow2<float> tbOcean(2048);
        {    float A[] = {1, 0.4, 0.65, 0.3, 0.18, 0.08};
        float C[] = {1,4,7,11,15,18};
        gam::addSines(tbOcean, A,C,6);
        }

        gam::ArrayPow2<float> tb__3(2048);
        {    float A[] = {1, 0.7, 0.45, 0.3, 0.15, 0.08};
        float C[] = {10, 27, 54, 81, 108, 135};
        gam::addSines(tb__3, A,C,6);
        }

        //sin A
        sin.freq(30);
        sin.source(tb__3);
        //sinDur = 4.0;
        sinDur = 0.2f;
        sinADR.levels(0,0.2,0.2,0);
        sinADR.lengths(sinDur/4, sinDur/2,sinDur/4);
        sinADR.curve(-4);
        sinTmr.period(sinDur * 4);
        //sinA baseFreqs
        shiftFreq = baseFreq * 0.5;
        finalFreq = baseFreq * 2 + shiftFreq;
        flucFreq = baseFreq / 5.0f;
        targetFreq = baseFreq;
        //bigger
        sinTmr2.period(sinDur * 12);
        

        
    }
    void noiseLevelUpdate(){
        noiseLevel = MapValue(neighborNum, 0, 100, 0.01, 0.4);
    }

    virtual void onProcess(AudioIOData& io) override {
        while (io()){
            
            if (jobHunting){
                //finalFreq = (1 - sinTmr.phase()) * oldFreq + sinTmr.phase() * targetFreq;
                //sin.freq(finalFreq);
                if (sinTmr()){
                    sinADR.reset();
                    
                    //sinADR.levels(0,0.2,0.2,0);
                    tmrIndex = rnd::uniform(8,0);
                    sinADR.lengths(sinDur * periods[tmrIndex] /4 ,sinDur * periods[tmrIndex] /2,sinDur  * periods[tmrIndex] /4);
                    //sinADR.curve(-4);
                    flucFreq = floor(al::rnd::uniform(75, 10));
                    
                    index ++;
                    if (index == 4){
                        index = 0;
                    }
                    targetFreq = freqs[index] + shiftFreq + flucFreq;
                    oldFreq = finalFreq;
                    sin.freq(targetFreq);
                    
                    sinTmr.period(periods[rnd::uniform(8,0)]);
                }

                if (sinTmr2()){
                    shiftFreq += baseFreq;
                    if (shiftFreq > baseFreq * 4) {
                        shiftFreq = baseFreq * 0.5;
                    }
                    // sinDur = sinDur * 0.8;
                    // sinTmr.period(sinDur);
                    // if (sinDur < 1.0){
                    //     sinDur = 6.0;
                    // }
                }

                float s = sin() * sinADR() * 0.2;
                io.out(0) = isnan(s) ? 0.0 : (double)s;
            } else {
                //finalFreq = (1 - sinTmr.phase()) * oldFreq + sinTmr.phase() * targetFreq;
                
                if (sinTmr()){
                    sinADR.reset();
                    
                    //sinADR.levels(0,0.2,0.2,0);
                     sinADR.lengths(sinDur * periods[tmrIndex] ,sinDur * periods[tmrIndex],sinDur  * periods[tmrIndex]);
                    //sinADR.lengths(sinDur,sinDur,sinDur);
                    //sinADR.curve(-4);
                    flucFreq = floor(al::rnd::uniform(75, 10));
                    
                    index ++;
                    if (index == 4){
                        index = 0;
                    }
                    targetFreq = freqs[index] + shiftFreq + flucFreq;
                    oldFreq = finalFreq;
                    sin.freq(targetFreq);
                }

                if (sinTmr2()){
                    sinTmr.period(sinDur * 12);
                    shiftFreq += baseFreq;
                    if (shiftFreq > baseFreq * 4) {
                        shiftFreq = baseFreq * 0.5;
                    }
                    // sinDur = sinDur * 0.8;
                    // sinTmr.period(sinDur);
                    // if (sinDur < 1.0){
                    //     sinDur = 6.0;
                    // }
                }
                float s = sin() * sinADR() * 0.2;
                io.out(0) = isnan(s) ? 0.0 : (double)s;
            }
            
            
            



        }
    }
    void run(vector<Factory>& fs, vector<Worker>& others, vector<Capitalist>& capitalist){
        if (jobHunting){
            patienceTimer += 1;
            if (patienceTimer == patienceLimit){
                senseFactory(fs);
                patienceTimer = 0;
            }
            
        }

        if (depression){
            jobHunting = true;
            if (capitalHoldings <= 2000){
                seekCapitalist(capitalist);
                separateForce = 1.5;
            } else {
                inherentDesire(desireLevel, MetroRadius, FactoryRadius, desireChangeRate);
                facingToward(movingTarget);
            }
        } else {
            if (FactoryFound){
                findPoems();
                if (distToClosestFactory > workingDistance){
                    seekFactory(fs);
                    separateForce = 0.3;               
                } else if (distToClosestFactory <= workingDistance && distToClosestFactory >= 0){
                    work(diligency, mood, fs[id_ClosestFactory].meshOuterRadius, fs);  
                    capitalHoldings += fs[id_ClosestFactory].individualSalary;
                     //earn salary here!! depends on ratio of workers needed and actual
                    //if (fs[id_ClosestFactory].workersWorkingNum <= fs[id_ClosestFactory].workersNeededNum){
                    if (std::find(fs[id_ClosestFactory].whitelist.begin(), fs[id_ClosestFactory].whitelist.end(), workerID) != fs[id_ClosestFactory].whitelist.end()){
                        jobHunting = false;
                    } else {
                        //think about jobhunting, while waiting for other people to opt out first
                        jobHunting = true;
                    }
                }
            } else {
                inherentDesire(desireLevel, MetroRadius, FactoryRadius, desireChangeRate);
                facingToward(movingTarget);
            }
        }
        //default behaviors
        Vec3f sep(separate(others));
        sep *= separateForce;
        applyForce(sep);     

        noiseLevelUpdate();
        borderDetect();
        update();
        moneyConsumption();
    }
    void moneyConsumption(){
        moneyTimer ++;
        if (moneyTimer == 0){
            lastSavings = capitalHoldings;
            monthlyTotal = 0;
        }
        if (moneyTimer % 60 == 0){
            currentSavings = capitalHoldings;
            todayIncome = currentSavings - lastSavings;
            monthlyTotal += todayIncome;
            lastSavings = currentSavings;
        }
        if (moneyTimer > 60 * 15){
            monthlyIncome = monthlyTotal;
            dailyIncome = monthlyIncome / 30;
            moneyTimer = 0;
        }
        //cout << dailyIncome << " daily income " << endl;
        //cout << monthlyIncome << " monthly income " << endl;'
        if (monthlyIncome > 5000 && monthlyIncome <= 8000){
            incomeTax = dailyIncome * 0.008;
            livingCost = 2.0;
        } else if (monthlyIncome > 8000 && monthlyIncome <= 15000){
            incomeTax = dailyIncome * 0.012;
            livingCost = 3.0;
        } else if (monthlyIncome > 15000 ){
            incomeTax = dailyIncome * 0.018;
            livingCost = 5.0;
        } else if (monthlyIncome <= 5000){
            incomeTax = 0;
            livingCost = 1.5;
        }


        capitalHoldings -= livingCost + incomeTax;
        if (capitalHoldings <= -2000){
            capitalHoldings = -2000;
        } else if (capitalHoldings >= 9999999){
            capitalHoldings = 9999999;
        }
    }
    bool bankrupted(){
        if (capitalHoldings <= -2000){
            return true;
        } else {
            return false;
        }
    }
    void findPoems(){
        //30% probability
        if (rnd::prob(0.0001)) {
            poetryHoldings += 1;
        };
    }
    
    void senseFactory(vector<Factory>& fs){
        float min_emptyOpeningRatio = 100;
        int min_EOR_id = 0;
        float max_material = 0;
        int max_material_id = 0;
        int openingCount = 0;     
        for (size_t i = 0; i < fs.size(); i++){
            if (fs[i].operating() && fs[i].hiring){
                openingCount += 1;
                // Vec3f dist_difference = pose().pos() - fs[i].position;
                // float dist = dist_difference.mag();
                if ( ( (fs[i].workersWorkingNum + 1) / fs[i].workersNeededNum) < min_emptyOpeningRatio){
                    min_emptyOpeningRatio = (fs[i].workersWorkingNum + 1) / fs[i].workersNeededNum;
                    min_EOR_id = i;
                }
            }
        }
        for (size_t i = 0; i < fs.size(); i ++){
            if (fs[i].operating() && fs[i].hiring){
                if (fs[i].materialStocks > max_material){
                    max_material = fs[i].materialStocks;
                    max_material_id = i;
                }
            }
        }
        Vec3f dist_differenceA = pose().pos() - fs[min_EOR_id].position;
        float distA = dist_differenceA.mag();
        Vec3f dist_differenceB = pose().pos() - fs[max_material_id].position;
        float distB = dist_differenceB.mag();

        //cout << min_EOR_id << " most empty fac" << fs[min_EOR_id].workersWorkingNum / fs[min_EOR_id].workersNeededNum << "  empty ratio" <<endl;
        //cout << max_material_id << "  = most masterial fac " << fs[max_material_id].materialStocks << " material num " << endl;
        
        if (distA < distB){
            distToClosestFactory = distA;
            id_ClosestFactory = min_EOR_id;
        } else {
            distToClosestFactory = distB;
            id_ClosestFactory = max_material_id;
        }

        //cout << openingCount << endl;
        if (openingCount == 0){
            depression = true;
        } else {
            depression = false;
        }
        if (distToClosestFactory < sensitivityFactory){
            FactoryFound = true;
        } else {
            FactoryFound = false;
        }
    }
    void seekFactory(vector<Factory>& fs){
        if (fs[id_ClosestFactory].operating()){
            Vec3f skFS(seek(fs[id_ClosestFactory].position));
            skFS *= 1.0;
            applyForce(skFS);
            Vec3f t = fs[id_ClosestFactory].position;
            facingToward(t);
        }
    }
    void seekCapitalist(vector<Capitalist>& capitalist){
        if (!capitalist[id_ClosestFactory].bankrupted()){
            Vec3f skCP(seek(capitalist[id_ClosestFactory].pose().pos()));
            skCP *= 1.0;
            applyForce(skCP);
            Vec3f t = capitalist[id_ClosestFactory].pose().pos();
            facingToward(t);
        } else {
            inherentDesire(desireLevel, MetroRadius, FactoryRadius, desireChangeRate);
            facingToward(movingTarget);
        }
    }
    void work(float diligency, int mood, float radius, vector<Factory>& fs){
        if (bioClock == 0){
            workTarget = r() * radius + fs[id_ClosestFactory].position;
        }
        if (bioClock % mood == 0) {
            workTarget = r() * radius + fs[id_ClosestFactory].position;
        }
        if (bioClock >= mood * 12 - 1){
            bioClock = 0;
            mood = r_int(30, 90);
        }
        bioClock ++;

        Vec3f workAround(seek(workTarget));
        workAround *= diligency;
        applyForce(workAround);
        facingToward(workTarget);
    }
    Vec3f separate(vector<Worker>& others){
        Vec3f sum;
        int count = 0;
        neighborNum = 0;
        for (Worker w : others){
            Vec3f difference = pose().pos() - w.pose().pos();
            float d = difference.mag();
            if ((d > 0) && (d < desiredseparation)){
                Vec3f diff = difference.normalize();
                sum += diff;
                count ++;
                neighborNum ++;
            }
        }
        if (count > 0){
            sum /= count;
            sum.mag(maxspeed);
            Vec3f steer = sum - velocity;
            if (steer.mag() > maxforce){
                steer.normalize(maxforce);
            }
            return steer;
        } else {
            return Vec3f(0,0,0);
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



#include "locations.hpp"

#endif
