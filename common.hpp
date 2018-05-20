#ifndef __COMMON__
#define __COMMON__

#include "al/core/app/al_App.hpp"

using namespace al;

struct State{
    //general stats
    float numMiners = 100;
    float numCapitalists = 15;
    float numWorkers = 75;
    float numResources = 40 * 7;
    float phase = 0;
    int renderModeSwitch = 1;
    float colorR = 1;
    float colorG = 1;
    float colorB = 1;
    float fogamount = 1;

    //miner
    Pose miner_pose[100];
    float miner_scale[100];
    float miner_poetryHoldings[100];
    bool miner_bankrupted[100];
    bool miner_fullpack[100];

    //worker
    Pose worker_pose[75];
    float worker_scale[75];
    float worker_poetryHoldings[75];
    bool worker_bankrupted[75];

    //capitalist
    Pose capitalist_pose[15];
    float capitalist_scale[15];
    float capitalist_poetryHoldigs[15];
    bool capitalist_bankrupted[15];

    //factory
    Vec3f factory_pos[15];
    float factory_rotation_angle[15];
    Quatf factory_facing_center[15];
    float factory_size[15];
    Color factory_color[15];

    //resources
    Vec3f resource_point_pos[40]; 
    Vec3f resource_pos[40 * 7]; // the positions of resource are already global
    float resource_angleA[40 * 7];
    float resource_angleB[40 * 7];
    float resource_scale[40 * 7];
    bool resource_picked[40 * 7];

    //lines
    Vec3f worker_lines_posA[75];
    Vec3f worker_lines_posB[75];
    Vec3f capitalist_lines_posA[15];
    Vec3f capitalist_lines_posB[15];
    Vec3f miner_lines_posA[100];
    Vec3f miner_lines_posB[100];

    //nav
    // Vec3f nav_pos;
    // Quatf nav_quat;
    Pose nav_pose;

    //buildings
    float metro_rotate_angle;
    Vec3f building_pos[15];
    float building_size[15];
    float building_scaleZ[15];

};

#endif
