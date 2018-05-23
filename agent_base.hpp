#ifndef INCLUDE_AGENT_BASE_HPP
#define INCLUDE_AGENT_BASE_HPP

#include "al/core/app/al_App.hpp"
#include "al/core/graphics/al_VAOMesh.hpp"

#include "al/util/scene/al_DynamicScene.hpp"

#define TESTING_WITHOUT_DYNAMIC_SCENE 0

float boundary_radius = 90.0f;

using namespace al;

class Agent : public PositionedVoice {
//class Agent {
public:
    Vec3f velocity, acceleration;
    Color c;
    Quatd q;
    Mesh body;
    float maxspeed;
    float minspeed;
    float mass;
    float initialRadius;
    float maxAcceleration;
    float maxforce;
    float target_senseRadius;
    float desiredseparation;
    float scaleFactor;
    int bioClock;
    float capitalHoldings;
    float poetryHoldings;
    float livingCost;
    float lastSavings;
    float currentSavings;
    float todayIncome;
    float monthlyTotal;
    float monthlyIncome;
    float dailyIncome;
    int moneyTimer;
    float incomeTax;
    float povertyWelfare;
    Vec3f movingTarget;
// #if TESTING_WITHOUT_DYNAMIC_SCENE
//     Pose pose() {
//         return Pose();
//     }

//     virtual void onProcess(AudioIOData& io) {}
// #endif

    void update(){
        velocity += acceleration;
        if (velocity.mag() > maxspeed){
            velocity.normalize(maxspeed);
        }
        pose().pos() += velocity;
        acceleration *= 0; //zeros acceleration
        
    }    
    void applyForce(Vec3f force){
      Vec3f f = force / mass;
      acceleration += f;
      if (acceleration.mag() > maxAcceleration){
        acceleration.normalize(maxAcceleration);
      }
    }

    void facingToward(Vec3f& target){
        //change facing direction based on target
        Vec3f src = Vec3f(pose().quat().toVectorZ()).normalize();
        Vec3f dst = Vec3f(target - pose().pos()).normalize();
        Quatd rot = Quatd::getRotationTo(src,dst);
        pose().quat() = rot * pose().quat();
    }

    Vec3f seek(Vec3f target){
        Vec3f desired = target - pose().pos();
        desired.normalized();
        desired *= maxspeed;
        Vec3f steer = desired - velocity;
        if (steer.mag() > maxforce){
            steer.normalize(maxforce);
        }
        return steer;
    }

    void inherentDesire(float desireLevel, float innerRadius, float outerRadius, int changeRate){
        //inherent desire that changes everyday
        //let them search for something in the metropolis
        bioClock++;
        if (bioClock % changeRate == 0) {
            movingTarget = r();
            Vec3f temp_pos = movingTarget;
            movingTarget = movingTarget * (outerRadius - innerRadius) + temp_pos.normalize(innerRadius);
        }
        if (bioClock > 1440 ){
            bioClock = 0;
        }
        Vec3f skTarget(seek(movingTarget));
        skTarget *= desireLevel;
        applyForce(skTarget);

        Vec3f ar(arrive(movingTarget));
        ar *= 0.3;
        applyForce(ar);
    }

    Vec3f arrive(Vec3f& target){
        Vec3f desired = target - pose().pos();
        float d = desired.mag();
        desired.normalize();
        if (d < target_senseRadius){
            float m = MapValue(d, 0, target_senseRadius, minspeed, maxspeed);
            desired *= m;
        } else {
            desired *= maxspeed;
        }
        Vec3f steer = desired - velocity;
        if (steer.mag() > maxforce){
            steer.normalize(maxforce);
        }
        return steer;
    }

    //border detect
    void borderDetect(){
        Vec3f origin(0,0,0);
        Vec3f distance = pose().pos() - origin;
        if (distance.mag() > boundary_radius) {
            Vec3f desired = origin - pose().pos();
            Vec3f steer = desired - velocity;
            if (steer.mag() > maxforce) {
                steer.normalize(maxforce);
            }
            applyForce(steer);
        } else {
            applyForce(Vec3f(0,0,0));
        }
    }
};


#endif
