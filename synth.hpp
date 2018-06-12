#ifndef INCLUDE_SYNTH_HPP
#define INCLUDE_SYNTH_HPP

#include "Gamma/Filter.h"
#include "Gamma/Envelope.h"
#include "Gamma/DFT.h"
#include "Gamma/Effects.h"
#include "Gamma/Delay.h"
#include "Gamma/Noise.h"
#include "Gamma/Oscillator.h"
#include "Gamma/SamplePlayer.h"
#include "al/core/app/al_App.hpp"

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

struct Tables{
    
    // gam::ArrayPow2<float> tbSaw, tbSqr, tbImp, tbSin, tbPls, tbOcean, tbSizhu, tb__3, tb__4;
    
    public:
    Tables(){
        gam::ArrayPow2<float>
        tbSaw(2048), tbSqr(2048), tbImp(2048), tbSin(2048), tbPls(2048),
        tbOcean(2048), tbSizhu(2048), tb__3(2048), tb__4(2048);
       
        gam::addSinesPow<1>(tbSaw, 9,1);
        gam::addSinesPow<1>(tbSqr, 9,2);
        gam::addSinesPow<0>(tbImp, 9,1);
        gam::addSine(tbSin);

        {    float A[] = {1,1,1,1,0.7,0.5,0.3,0.1};
            gam::addSines(tbPls, A,8);
        }

        {    float A[] = {1, 0.4, 0.65, 0.3, 0.18, 0.08};
            float C[] = {1,4,7,11,15,18};
            gam::addSines(tbOcean, A,C,6);
        }

        // inharmonic partials
        {    float A[] = {0.5,0.8,0.7,1,0.3,0.4,0.2,0.12}; // harmonic amplitudes of series
            float C[] = {3,4,7,8,11,12,15,16}; //cycles harmonic numbers of series
            gam::addSines(tbSizhu, A,C,8); //8 is number of harmonics, same with array size above
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

#endif