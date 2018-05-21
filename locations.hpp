#ifndef INCLUDE_LOCATIONS_HPP
#define INCLUDE_LOCATIONS_HPP

#include "al/core/app/al_App.hpp"

#include "helper.hpp"
#include "location_base.hpp"
#include "agents.hpp"


//reference: Platonic Solids by Lance Putnam

using namespace al;
using namespace std;

float MetroRadius = 5.0f;
float FactoryRadius = 20.0f;
float NaturalRadius = 30.0f;
float CirclePadding = 2.5f;

 //forward declaration
struct Worker;
struct Miner;
struct Capitalist;

//interface declaration
struct Resource {
    Vec3f position;
    Color c;
    float angle1;
    float rotation_speed1;
    float angle2;
    float rotation_speed2;
    bool isPicked;
    bool beingPicked;
    int timer;
    float scaleFactor;

    Resource(){
        scaleFactor = rnd::uniform(1.0,3.0);
        position = r();
        c = HSV(rnd::uniformS(), 0.7, 1);
        angle1 = 0.0f;
        angle2 = 0.0f; 
        rotation_speed1 = rnd::uniform(0.8,1.8);
        rotation_speed2 = rnd::uniform(0.5,1.2);
        isPicked = false;
        beingPicked = false;
        timer = 0;
    }

    void update(){
        angle1 += rotation_speed1;
        if (angle1 > 360){
            angle1 = 0;
        }
        angle2 += rotation_speed2;
        if (angle2 > 360){
            angle2 = 0;
        }
    }
    
};


struct MetroBuilding : Location{
    int mesh_Nv;
    float buildingID;
    float maxBuildings;
    float scaleFactorZ;
    float scaleFactorZ_mark;
    float scaleZvalue;
    float scaleTimer;

    MetroBuilding(){
        scaleFactorZ = 1.0;
        scaleFactorZ_mark = 1.0;
        scaleTimer = 0;
        
        mesh_Nv = addCube(mesh);
        addCube(mesh_wire);
        for(int i=0; i<mesh_Nv; ++i){
            float f = float(i)/mesh_Nv;
            mesh.color(HSV(0.06 + f*0.1,0.8,1));
            mesh_wire.color(HSV(0.06 + f*0.1,0.8,1));
        }
        mesh.decompress();
        mesh.generateNormals();


        // mesh_wire.primitive(Mesh::LINE_LOOP);
        //c = HSV(rnd::uniform(), 0.7, 1);
    }

    void run(){
        scaleTimer ++;
        if (scaleTimer == 360){
            scaleFactorZ_mark = scaleFactorZ;
            scaleTimer = 0;
        }
        scaleZvalue = scaleFactorZ_mark + (scaleFactorZ - scaleFactorZ_mark) * MapValue(scaleTimer, 0, 360, 0, 1);
        
        //floating little bit
    }

    void draw(Graphics& g) {
        g.pushMatrix();
        g.translate(position);
        g.scale(scaleFactor, scaleFactor, scaleZvalue);
        g.meshColor();
        g.polygonMode(Graphics::FILL);
        g.draw(mesh);
        g.scale(1.4);
        g.polygonMode(Graphics::LINE);
        g.draw(mesh_wire);
        g.polygonMode(Graphics::FILL);
        g.popMatrix();
    }
};

struct Factory : Location{
    float working_radius;
    float meshOuterRadius;
    float meshInnerRadius;
    Vec3f temp_pos;
    float angle1;
    float angle2;
    float rotation_speed1;
    Quatd q;
    Quatd facing_center;
    float materialStocks;
    bool hiring;
    int workersNeededNum;
    int workersWorkingNum;
    vector<int> whitelist;
    int maxWorkersAllowed;
    int shutDownCountDown;
    int produceTimer;
    int profitTimer;
    float produceRate;
    float produceRateFactor;
    float grossProfits;
    float resourceUnitPrice;
    float laborUnitPrice;
    float capitalReserve;
    float materialConsumptionRate;
    int factoryID;
    float individualSalary;
    float produceRateAdjust;
    float MinerCapitalistRatio;
    Factory(){
        //drawing
        meshOuterRadius = 1.6f;
        meshInnerRadius = 0.2f;
        scaleFactor = 1;
        position = r();
        temp_pos = position;
        position = position * (FactoryRadius - MetroRadius) + temp_pos.normalize(MetroRadius + CirclePadding);
        
        //mesh body
        
        addTorus(mesh, meshInnerRadius, meshOuterRadius, r_int(3, 6), r_int(3, 6));
        addTorus(mesh_wire, meshInnerRadius * 1.4, meshOuterRadius * 1.4, r_int(3, 6), r_int(3, 6));
        mesh.decompress();
        mesh.generateNormals();
        mesh_wire.primitive(Mesh::LINE_LOOP);
        c = HSV(0.56, 0.3, 1);
        
        angle1 = 0.0f;
        angle2 = rnd::uniform(0, 360); // face toward?
        facing_center = Quatd::getRotationTo( Vec3f(q.toVectorZ().normalize()), Vec3f(Vec3f(0,0,0) - position).normalize()) * facing_center;

        //working stats
        working_radius = scaleFactor * meshOuterRadius; //for workers to earn wage
        materialStocks = r_int(15, 15);
        workersNeededNum = ceil((float)materialStocks / 6);
        maxWorkersAllowed = 40;
        whitelist.resize(maxWorkersAllowed);
        for (size_t i = 0; i < whitelist.size(); i ++){
            whitelist[i] = 99999;
        }
        workersWorkingNum = 0;
        hiring = true;
        produceTimer = 0;
        profitTimer = 0;
        produceRate = 1;
        produceRateFactor = 0.5;
        produceRateAdjust = 1.0;

        //money related
        grossProfits = 0;
        capitalReserve = 10000.0;
        

        //market related
        resourceUnitPrice = 150;
        laborUnitPrice = 250;
        individualSalary = laborUnitPrice / 60;

        //shutdown
        shutDownCountDown = 240;
    }
    void produce(){
        produceTimer ++;
        produceRate = floorf(60.0f / (produceRateFactor * produceRateAdjust));
        //cout << produceTimer<< endl;
        if (produceTimer % (int)produceRate == 0){
            
            
            materialConsumptionRate = MinerCapitalistRatio  * (float)workersWorkingNum / (float)workersNeededNum;
            //cout << workersWorkingNum / workersNeededNum << "w/w ratio" << endl;
            //cout << materialConsumptionRate << " mterial consumption rate" << endl;
            materialStocks -= 1 * materialConsumptionRate * (produceRateAdjust * 0.5);
            grossProfits += resourceUnitPrice * 10.0 * materialConsumptionRate * (produceRateAdjust * 0.5); //become a commodity, 10 times more than it was
            // cout << "producing = " << produceTimer << endl;
            // cout << " gross profits = " <<grossProfits << endl;
            if (materialStocks > 30){
                produceRateAdjust *= 1.2;
            } else if (materialStocks < 12){
                produceRateAdjust *= 0.8;
            }
            if (produceRateAdjust >= 4){
                produceRateAdjust = 4; //400% produce rate
            } else if (produceRateAdjust <= 0.25){
                produceRateAdjust = 0.25; // 25% produce rate
            }
        }
        if (produceTimer >= (int)produceRate * 24 - 1 ){
            produceTimer = 0;
        }

        if (materialStocks <= 0){
            materialStocks = 0;
        }
    }
    void visualChange(){
        float red = MapValue(materialStocks, 0, 50, 0, 0.98);
        c = RGB(red, 0.2, 0.2);
        scaleFactor = 1 + workersWorkingNum * 0.3;

    }
    void profit(){
        profitTimer ++;
        if (profitTimer == 360){
            grossProfits = 0;
            profitTimer = 0;
        }
    }
    void animate(){
        angle1 += produceRateAdjust * 1.5;
        if (angle1 > 360){
            angle1 = 0;
        }
    }
    void openPositions(){
        workersNeededNum = ceil((float)materialStocks / 6);
        if (workersNeededNum >= maxWorkersAllowed - 1){
            workersNeededNum = maxWorkersAllowed - 1;
        }
        //openings.resize(workersNeededNum);
        if (workersWorkingNum >= workersNeededNum){
            hiring = false;
        } else {
            hiring = true;
        }
        for (size_t i = workersNeededNum; i < whitelist.size(); i ++ ){
            whitelist[i] = 99999;
        }
    }
    void run(){
        //cout << workersWorkingNum << " = worker working here" << endl;
        openPositions();
        if (materialStocks > 0){
            if (workersWorkingNum >= 1){
                produce();
                animate();
            }
            shutDownCountDown = 240;
        } else {
            shutDownCountDown--;
            if (shutDownCountDown <= 0){
                shutDownCountDown = 0;
            } else {
                //animate();
            }
        } 

        profit();
        visualChange();
    }

    bool operating(){
        if (shutDownCountDown <= 0){
            return false;
        } else {
            return true;
        }
    }

    void draw(Graphics& g) {
        g.pushMatrix();
        g.translate(position);
        g.rotate(facing_center);
        g.rotate(angle1, 0,0,1);
        g.scale(scaleFactor);
        g.color(c);
        g.polygonMode(Graphics::FILL);
        g.draw(mesh);
        g.polygonMode(Graphics::LINE);
        g.draw(mesh_wire);
        g.polygonMode(Graphics::FILL);
        
        g.popMatrix();
    }
};

struct Natural_Resource_Point : Location{
    float meshRadius;
    float resource_spawn_radius;
    int respawn_timer;
    float regeneration_rate;
    Vec3f temp_pos;
    float mesh_Nv;
    float resource_distribution_density;
    int afterDrainTimer;
    size_t pickCount;
    int maxResourceNum;
    int r_index;
    int initResourceNum;
    float fruitfulness;

    vector<Resource> resources;
    vector<bool> drain_check;

    Natural_Resource_Point(){
        scaleFactor = 1.3;
        meshRadius = 0.4f;
        resource_distribution_density = 0.4f;
        resource_spawn_radius = meshRadius * (1/resource_distribution_density) * scaleFactor; // for resource to spawn
        position = r();
        temp_pos = position;
        position = position * (NaturalRadius - FactoryRadius) + temp_pos.normalize(FactoryRadius + CirclePadding * 2.0);
        //position = FactoryRadius + CirclePadding + r() * (NaturalRadius - FactoryRadius);
        mesh_Nv = addDodecahedron(mesh, meshRadius);
        addDodecahedron(mesh_wire, meshRadius * 1.4);
		for(int i=0; i<mesh_Nv; ++i){
			float f = float(i)/mesh_Nv;
			mesh.color(HSV(f*0.1,1,1));
            mesh_wire.color(HSV(f*0.1,1,1));
		}
        // mesh_wire.primitive(Mesh::LINE_LOOP);
        mesh.decompress();
        mesh.generateNormals();
        //c = HSV(rnd::uniform(), 0.7, 1);


        //respawn and drain
        respawn_timer = 0;
        regeneration_rate = rnd::uniform(0.5, 0.95); //based on 60fps, if 1, then every second, if 2, then half a second
        pickCount = 0;
        maxResourceNum = 7;
        r_index = 0;

        //initialize resource
        resources.resize(maxResourceNum);
        for (int i = resources.size() - 1; i >= 0; i--){
            Resource& r = resources[i];
            r.position = position + r.position * resource_spawn_radius;
            r.isPicked = true;
        }
        
        //drain check stuff
        drain_check.resize(maxResourceNum);
        for (int i = drain_check.size() - 1; i >= 0; i--){
            drain_check[i] = true;
        }

        initResourceNum = 5;
        for (int i = 0; i < initResourceNum; i ++){
            resources[i].isPicked = false;
             drain_check[i] = false;
        }
        afterDrainTimer = 0;

        fruitfulness = ((float)maxResourceNum - (float)pickCount) / (float)maxResourceNum;
    }

    void respawn_resource(){
        if (!drained()){
            respawn_timer++;

            if (respawn_timer % (int)floorf(60.0f / regeneration_rate) == 0){
                int guard_count = 0;
                while (resources[r_index].beingPicked){
                    guard_count ++;
                    if (guard_count > maxResourceNum * 2){
                        cout << "guarded!" << endl;
                        break;
                    }
                    if (r_index < maxResourceNum - 1){
                        r_index += 1;
                    } else if (r_index >= maxResourceNum - 1){
                        r_index = 0;
                    }
                }
                resources[r_index].isPicked = false;
                drain_check[r_index] = false;
                if (r_index < maxResourceNum - 1){
                        r_index += 1;
                } else if (r_index >= maxResourceNum - 1){
                        r_index = 0;
                }
            }

            //don't need to touch here
            if (respawn_timer > 1000) {
                respawn_timer = 0;
            }
            
        } else if (drained()){
            afterDrainTimer ++;
            if (afterDrainTimer == 1440){
                int r = r_int(0, maxResourceNum);
                //cout << r << " = index of resources generated" << endl;
                resources[r].isPicked = false;
                drain_check[r] = false;
                afterDrainTimer = 0;
            }
        }
    }

    void update_resource(){
        //check fruitfulness
        fruitfulness = ((float)maxResourceNum - (float)pickCount) / (float)maxResourceNum;
        //cout << fruitfulness << " = fruitfulness" << endl;
        for (int i = resources.size() - 1; i >= 0; i--){
            Resource& r = resources[i];
            r.update();
            if (r.beingPicked){
                r.timer++;
                if (r.timer == 120){
                    r.isPicked = true;
                    drain_check[i] = true;
                    //r.position.set(200,200,200);
                }
                if (r.timer == 180){
                    r.beingPicked = false;
                    r.timer = 0;
                    // resources.erase(resources.begin() + i);
                }
            }
        }
    }
    bool drained(){
        pickCount = 0;
        for (size_t i = 0; i < resources.size(); i++){
            if (drain_check[i]){
                pickCount += 1;
            }
        }
        if (pickCount >= resources.size()){
            return true;
        } else {
            return false;
        }
    }

    void draw(Graphics& g) {
        g.meshColor();
        for (int i = resources.size() - 1; i >= 0; i--){
            Resource& r = resources[i];
            if (!r.isPicked){
                g.pushMatrix();
                g.translate(r.position);
                g.rotate(r.angle2, 1,0,0);
                g.rotate(r.angle1, 0,1,0);
                g.scale(scaleFactor);
                g.polygonMode(Graphics::FILL);
                g.draw(mesh);
                g.polygonMode(Graphics::LINE);
                g.draw(mesh_wire);
                g.popMatrix();
            }
        }
        g.polygonMode(Graphics::FILL);        
    }
};



#endif
