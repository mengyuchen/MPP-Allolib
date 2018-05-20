#ifndef INCLUDE_HELPER_HPP
#define INCLUDE_HELPER_HPP

#include "al/core/app/al_App.hpp"
#include "al/core/math/al_Random.hpp"

using namespace al;

// helper function: makes a random vector
Vec3f r() { return Vec3f(rnd::uniformS(), rnd::uniformS(), rnd::uniformS()); }
//map function
float MapValue(float x, float in_min, float in_max, float out_min, float out_max){
  return (x - in_min) * (out_max  - out_min) / (in_max - in_min) + out_min;
}
//random int, span doesn't include span
int r_int(int init, int span){
    int v = rand() % span + init;
    //int v = rand_r(12) % span + init; set the seed

    return v;
}

#endif
