#ifndef INCLUDE_LOCATION_MANAGERS_HPP
#define INCLUDE_LOCATION_MANAGERS_HPP

#include "al/core/app/al_App.hpp"
#include "locations.hpp"
#include "meshes.hpp"
#include "status_manager.hpp"
#include "agents.hpp"

struct Miner;

struct Metropolis{    
    vector<MetroBuilding> mbs;
    int initial_num;
    float angle;
    Metropolis(){
        // initial_num = 15;
        // mbs.resize(initial_num);
    }

    MetroBuilding operator [](const int index) const {
        return mbs[index];
    }
    void generate(Capitalist_Entity& cs){
        for(int i = cs.cs.size() - 1; i >= 0; i --){
            MetroBuilding m;
            m.buildingID = i;
            m.maxBuildings = cs.cs.size();
            m.scaleFactor = 1;
            m.position.x = MetroRadius * 2 * sin(MapValue(m.buildingID, 0, m.maxBuildings, 0, M_PI * 2));
            m.position.y = MetroRadius * 2 * cos(MapValue(m.buildingID, 0, m.maxBuildings, 0, M_PI * 2));
            m.position.z = 0;
            mbs.push_back(m);
        }
    }
    void mapCapitalistStats(vector<Capitalist>& capitalists){
        for (size_t i = 0; i < capitalists.size(); i ++){
            mbs[i].maxBuildings = capitalists.size();
            mbs[i].scaleFactorZ = MapValue(capitalists[i].capitalHoldings, 0, 500000, 0.1, 20);
            mbs[i].position.x = MetroRadius * 2 * sin(MapValue(mbs[i].buildingID, 0, mbs[i].maxBuildings, 0, M_PI * 2));
            mbs[i].position.y = MetroRadius * 2 * cos(MapValue(mbs[i].buildingID, 0, mbs[i].maxBuildings, 0, M_PI * 2));
            mbs[i].position.z = 0;
        }
    }

    void run(){

        for (MetroBuilding& mb : mbs){
            mb.run();
        }

        angle += 0.5;
        if (angle > 360){
            angle = 0;
        }
    }
    void draw(Graphics& g){
        g.pushMatrix();
        g.rotate(angle);
        for (int i = mbs.size() - 1; i >= 0; i --){
            MetroBuilding& mb = mbs[i];
            mb.draw(g);
        }
        g.popMatrix();
    }

};

struct Factories {
    vector<Factory> fs;
    vector<Line> lines;
    int initial_num;
    bool drawingLinks;
    float resourceUnitPrice;
    Factories(){
        drawingLinks = true;
        // initial_num = 30;
        // fs.resize(initial_num);
    }
    void generate(Capitalist_Entity& cs){
        for(int i = cs.cs.size() - 1; i >= 0; i --){
            Factory f;
            Line l;
            f.factoryID = i;
            fs.push_back(f);
            lines.push_back(l);
        }
    }
    void drawLinks(Capitalist_Entity& cs){
        if (drawingLinks){
            for (int i = cs.cs.size() - 1; i >= 0; i --){
                lines[i].vertices()[0] = cs[i].pose().pos();
                lines[i].vertices()[1] = fs[i].position;
            }
        } else{
            for (int i = cs.cs.size() - 1; i >= 0; i --){
                lines[i].vertices()[0] = Vec3f(0,0,0);
                lines[i].vertices()[1] = Vec3f(0,0,0);
            }
        }
    }
    void getResource(Capitalist_Entity& cs){
        for (int i = cs.cs.size() - 1; i >= 0; i --){
            if (cs.cs[i].resourceClock == cs.cs[i].TimeToDistribute - 1){
                fs[i].materialStocks += cs.cs[i].resourceHoldings;
                fs[i].capitalReserve += cs.cs[i].workersPayCheck;
            }
        }
    }
    void getLaborPrice(MarketManager& market){
        for (int i = fs.size() - 1; i >= 0; i --){
            fs[i].resourceUnitPrice = market.resourceUnitPrice;
            fs[i].laborUnitPrice = market.laborUnitPrice; 
            fs[i].MinerCapitalistRatio = market.MinerCapitalistRatio;
            fs[i].maxWorkersAllowed = ceil(market.WorkerCapitalistRatio * 1.5);
    
        }

    }
    void checkWorkerNum(vector<Worker>& workers){
        for (int i = fs.size() - 1; i >= 0; i --){
            fs[i].workersWorkingNum = 0;
        }
        for (int i = workers.size() - 1; i >= 0; i--){
            if (workers[i].distToClosestFactory <= workers[i].workingDistance){
                if (fs[workers[i].id_ClosestFactory].workersWorkingNum < fs[workers[i].id_ClosestFactory].workersNeededNum){
                   fs[workers[i].id_ClosestFactory].whitelist[fs[workers[i].id_ClosestFactory].workersWorkingNum] = workers[i].workerID;
                    fs[workers[i].id_ClosestFactory].workersWorkingNum += 1;
                }
            }
        }
    }
    void payWorkers(MarketManager& market){
        for (int i = fs.size() - 1; i >= 0; i--){
            fs[i].individualSalary = market.laborUnitPrice / 60;
            fs[i].capitalReserve -= fs[i].workersWorkingNum * market.laborUnitPrice / 360;
        }
    }

    void run(Capitalist_Entity& cs){
        getResource(cs);
        for (int i = fs.size() - 1; i >= 0; i --){
            Factory& f = fs[i];
                //rather than destroy the object, stop animating
                //fs.erase(fs.begin() + i);
            f.run();
        }
    }

    void draw(Graphics& g){
        for (int i = fs.size() - 1; i >= 0; i --){
            Factory& f = fs[i];
            f.draw(g);
            g.color(1,0.55,0.4);
            g.draw(lines[i]);
        }
        
    }
};

struct NaturalResourcePointsCollection {
    vector<Natural_Resource_Point> nrps;
    int initial_num;
    NaturalResourcePointsCollection(){
        initial_num = 40;
        nrps.resize(initial_num);
    }

    void checkMinerPick(vector<Miner>& miners){
        for (int k = nrps.size() - 1; k >= 0; k --){
            Natural_Resource_Point& nrp = nrps[k];
            for (size_t i = 0; i < nrp.resources.size(); i ++){
                nrp.resources[i].beingPicked = false;
                
            }
        }
        for (size_t j = 0; j < miners.size(); j ++){
                if (miners[j].distToClosestResource < miners[j].pickingRange && miners[j].resourcePointFound == true){
                    nrps[miners[j].id_ClosestNRP].resources[miners[j].id_ClosestResource].beingPicked = true;
            } 
        }
    }
    
    void run(){
        for (int i = nrps.size() - 1; i >= 0; i --){
            Natural_Resource_Point& nrp = nrps[i];
            nrp.respawn_resource();
            nrp.update_resource();
        }
    }
    void draw(Graphics& g){
        for (int i = nrps.size() - 1; i >= 0; i --){
            Natural_Resource_Point& nrp = nrps[i];
            nrp.draw(g);
        }
    }
};



#endif
