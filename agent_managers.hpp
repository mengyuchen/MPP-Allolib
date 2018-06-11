#ifndef INCLUDE_AGENT_MANAGERS_HPP
#define INCLUDE_AGENT_MANAGERS_HPP

#include "al/core/app/al_App.hpp"
#include "meshes.hpp"
#include "agents.hpp"

struct Capitalist_Entity{
    vector<Capitalist> cs;
    int initial_num;
    float resourceUnitPrice = 280;

    Capitalist_Entity(){
        initial_num = 15;
        cs.resize(initial_num);
        cout << "Capitalist Entity Joined" << endl;

    }
    Capitalist operator[] (const int index) const{
        return cs[index];
    }
    void initID(){
        for (int i = cs.size() - 1; i >= 0; i --){
            cs[i].capitalistID = i;
        }
    }

    void getResource(vector<Miner>& miners){
    
        for (int j = miners.size() - 1; j >= 0; j --){
            if (miners[j].exchanging == true){
                if (miners[j].tradeTimer == miners[j].unloadTimeCost - 1){
                    //cout << " i m getting resource" << endl;
                    if (!cs[miners[j].id_ClosestCapitalist].bankrupted()){
                        cs[miners[j].id_ClosestCapitalist].resourceHoldings += miners[j].resourceHoldings;
                        cs[miners[j].id_ClosestCapitalist].totalResourceHoldings += miners[j].resourceHoldings;
                        cs[miners[j].id_ClosestCapitalist].capitalHoldings -= miners[j].resourceHoldings * resourceUnitPrice;
                    }
                }
            }
        }
    }
    void getWorkersPaymentStats(vector<Factory>& fs){
        resourceUnitPrice = fs[0].resourceUnitPrice;
        for (int i = cs.size() - 1; i >= 0; i --){
            if (!cs[i].bankrupted()){
                cs[i].numWorkers = fs[i].workersWorkingNum;
                cs[i].workersPayCheck = fs[i].laborUnitPrice * cs[i].numWorkers;
                if (fs[i].profitTimer == 359){
                    //cout << "earning profiting" << endl;
                    cs[i].capitalHoldings += fs[i].grossProfits;
                }
            }
        }
    }
    void run(vector<MetroBuilding>& mbs){
        for (int i = cs.size() - 1; i >= 0; i --){
            Capitalist& c = cs[i];
            if (!c.bankrupted()){
                
                //avoid hitting block
                // Vec3f ahb(avoidHittingBuilding(mbs, c));
                // ahb *= 0.8;
                // applyForce(ahb, c);

                borderDetect(c);
                inherentDesire(0.5, MetroRadius * 0.6, MetroRadius * 2, c.desireChangeRate, c);
                if (i == 5){
                //     //cout << c.movingTarget << " moving target"<< endl;
                //     //cout << c.pose().quat().x << " " << c.pose().quat().y << " " << c.pose().quat().z << " " << c.pose().quat().w << " cRot"<< endl;
                //     // cout << c.pose().quat().toVectorZ() << endl;
                //     // cout << c.srcRot << " src"<< endl;
                //     // cout << c.dstRot << " dst"<< endl;
                // cout << abs(Vec3f(c.movingTarget - c.pose().pos())) << endl;
                //     // cout << Vec3f(c.movingTarget - c.pose().pos())) << endl;
                //      //cout << c.cRot.x << " " << c.cRot.y << " " << c.cRot.z << " " << c.cRot.w << " cRot"<< endl;
                }
                facingToward(c.movingTarget,c);
                update(c);

                //money, resource related
                distributeResources(c);
                moneyConsumption(c);
            }
        }
    }
    void draw(Graphics& g){
        for (int i = cs.size() - 1; i >= 0; i --){
            Capitalist& c = cs[i];
            c.draw(g);
        }
    }

    //grouped functions
    void moneyConsumption(Capitalist& c){
        c.moneyTimer ++;
        if (c.moneyTimer == 0){
            c.lastSavings = c.capitalHoldings;
            c.monthlyTotal = 0;
        }
        if (c.moneyTimer % 60 == 0){
            c.currentSavings = c.capitalHoldings;
            c.todayIncome = c.currentSavings - c.lastSavings;
            c.monthlyTotal += c.todayIncome;
            c.lastSavings = c.currentSavings;
        }
        if (c.moneyTimer > 60 * 15){
            c.monthlyIncome = c.monthlyTotal;
            c.dailyIncome = c.monthlyIncome / 30;
            c.moneyTimer = 0;
        }
        //cout << dailyIncome << " daily income " << endl;
        //cout << monthlyIncome << " monthly income " << endl;
        if (c.monthlyIncome > 5000 && c.monthlyIncome <= 8000){
            c.incomeTax = c.dailyIncome * 0.008;
            c.livingCost = 8.0;
        } else if (c.monthlyIncome > 8000 && c.monthlyIncome <= 15000){
            c.incomeTax = c.dailyIncome * 0.012;
            c.livingCost = 9.0;
        } else if (c.monthlyIncome > 15000 ){
            c.incomeTax = c.dailyIncome * 0.018;
            c.livingCost = 10.0;
        } else if (c.monthlyIncome <= 5000){
            c.incomeTax = 0;
            c.livingCost = 6.0;
        }
        //formula for life consumption
        c.capitalHoldings -= c.livingCost + c.incomeTax;
        //minimum and maximum value boundary
        if (c.capitalHoldings <= -50000){
            c.capitalHoldings = -50000;
        } else if (c.capitalHoldings >= 9999999){
            c.capitalHoldings = 9999999;
        }
    } // money consumption

    void distributeResources(Capitalist& c){
        c.resourceClock ++;
        //every 12 seconds, half a day, distribute resource
        if (c.resourceClock == c.TimeToDistribute) {
            c.resourceHoldings = 0;
            c.capitalHoldings -= c.workersPayCheck;
            c.resourceClock = 0;
        }
    } //distribute resources

    Vec3f avoidHittingBuilding(vector<MetroBuilding>& mbs, Capitalist& c){
        Vec3f sum;
        int count = 0;
        for (MetroBuilding mb : mbs){
            Vec3f difference = c.pose().pos() - mb.position;
            float d = difference.mag();
            if ((d > 0) && (d < c.desiredseparation * mb.scaleFactor)){
                Vec3d diff = difference.normalize();
                sum += diff;
                count++;
            }
        }
        if (count > 0){
            sum /= count;
            sum.mag(c.maxspeed);
            Vec3f steer = sum - c.velocity;
            if (steer.mag() > c.maxforce) {
                steer.normalize(c.maxforce);
            }
            return steer;
        } else {
            return Vec3f(0,0,0);
        }
    } // avoid hitting building

    //base behavior//

    void update(Capitalist& c){
        c.velocity += c.acceleration;
        if (c.velocity.mag() > c.maxspeed){
            c.velocity.normalize(c.maxspeed);
        }
        c.pose().pos() += c.velocity;
        c.acceleration *= 0; //zeros acceleration
        
    } // Capitalistr -> update

    Vec3f seek(Vec3f target, Capitalist& c){
        Vec3f desired = target - c.pose().pos();
        desired.normalized();
        desired *= c.maxspeed;
        Vec3f steer = desired - c.velocity;
        if (steer.mag() > c.maxforce){
            steer.normalize(c.maxforce);
        }
        return steer;
    } // Capitalist->seek

    void applyForce(Vec3f force, Capitalist& c){
      Vec3f f = force / c.mass;
      c.acceleration += f;
      if (c.acceleration.mag() > c.maxAcceleration){
        c.acceleration.normalize(c.maxAcceleration);
      }
    } //Capitalist -> applyForce

    void facingToward(Vec3f target, Capitalist& c){
        //change facing direction based on target
        Vec3f dst = Vec3f(target - c.pose().pos());

        //if too close, zero the distance
        //to make it not drifting back and forth
        if (abs(dst) < 0.05){
            dst = Vec3f(0,0,0);
        }

        c.srcRot = Vec3f(c.pose().quat().toVectorZ()).normalize();
        c.dstRot = dst.normalize();
        //c.dstRot = Vec3f(target - c.pose().pos()).normalize();
        c.cRot = Quatd::getRotationTo(c.srcRot,c.dstRot);
        c.pose().quat() = c.cRot * c.pose().quat();
    } //Capitalist -> facingToward

    void inherentDesire(float desireLevel, float innerRadius, float outerRadius, int changeRate, Capitalist& c){
        //inherent desire that changes everyday
        //let them search for something in the metropolis
        c.bioClock++;
        if (c.bioClock % changeRate == 0) {
            c.movingTarget = r();
            c.temp_pos = c.movingTarget;
            c.movingTarget = c.movingTarget * (outerRadius - innerRadius) + c.temp_pos.normalize(innerRadius);
        }
        if (c.bioClock > 1440 ){
            c.bioClock = 0;
        }
        Vec3f skTarget(seek(c.movingTarget, c));
        skTarget *= desireLevel;
        applyForce(skTarget, c);

        Vec3f ar(arrive(c.movingTarget, c));
        ar *= 0.3;
        applyForce(ar, c);
    } // Capitalist -> inherentDesire

    Vec3f arrive(Vec3f& target, Capitalist& c){
        Vec3f desired = target - c.pose().pos();
        float d = desired.mag();
        desired.normalize();
        if (d < c.target_senseRadius){
            float m = MapValue(d, 0, c.target_senseRadius, c.minspeed, c.maxspeed);
            desired *= m;
        } else {
            desired *= c.maxspeed;
        }
        Vec3f steer = desired - c.velocity;
        if (steer.mag() > c.maxforce){
            steer.normalize(c.maxforce);
        }
        return steer;
    } // Capitalist -> arrive

    void borderDetect(Capitalist& c){
        Vec3f origin(0,0,0);
        Vec3f distance = c.pose().pos() - origin;
        if (distance.mag() > boundary_radius) {
            Vec3f desired = origin - c.pose().pos();
            Vec3f steer = desired - c.velocity;
            if (steer.mag() > c.maxforce) {
                steer.normalize(c.maxforce);
            }
            applyForce(steer, c);
        } else {
            applyForce(Vec3f(0,0,0), c);
        }
    } // Capitalist -> borderDetect

};

struct Worker_Union{
    vector<Worker> workers;
    int initial_num;

    //visualize relations;
    vector<Line> lines;
    bool drawingLinks;

    Worker_Union(){
        initial_num = 75;
        workers.resize(initial_num);
        lines.resize(workers.size());
        drawingLinks = true;
        cout << "Worker Union joined" << endl;
    }
    Worker operator[] (const int index) const{
        return workers[index];
    }
    void initID(){
        for (int i = workers.size() - 1; i >= 0; i --){
            workers[i].workerID = i;
        }
    }
    void run(vector<Factory>& fs, vector<Worker>& others, vector<Capitalist>& capitalist){
        for (int i = workers.size() - 1; i >= 0; i --){
            Worker& w = workers[i];
            if (!w.bankrupted()){
                //w.run(fs, others, capitalist);
                compute(fs, others, capitalist, w);
            }
        }
        visualize(fs);
    }

    void compute(vector<Factory>& fs, vector<Worker>& others, vector<Capitalist>& capitalist, Worker& w){
        if (w.jobHunting){
            w.patienceTimer += 1;
            if (w.patienceTimer == w.patienceLimit){
                senseFactory(fs,w);
                w.patienceTimer = 0;
            }
            
        }
        if (w.depression){
            w.jobHunting = true;
            if (w.capitalHoldings <= 2000){
                seekCapitalist(capitalist, w);
                w.separateForce = 1.5;
            } else {
                inherentDesire(w.desireLevel, MetroRadius, FactoryRadius, w.desireChangeRate, w);
                facingToward(w.movingTarget, w);
            }
        } else {
            if (w.FactoryFound){
                findPoems(w);
                if (w.distToClosestFactory > w.workingDistance){
                    seekFactory(fs, w);
                    w.separateForce = 0.3;               
                } else if (w.distToClosestFactory <= w.workingDistance && w.distToClosestFactory >= 0){
                    work(w.diligency, w.mood, fs[w.id_ClosestFactory].meshOuterRadius, fs, w);  
                    w.capitalHoldings += fs[w.id_ClosestFactory].individualSalary;
                     //earn salary here!! depends on ratio of workers needed and actual
                    //if (fs[id_ClosestFactory].workersWorkingNum <= fs[id_ClosestFactory].workersNeededNum){
                    if (std::find(fs[w.id_ClosestFactory].whitelist.begin(), fs[w.id_ClosestFactory].whitelist.end(), w.workerID) != fs[w.id_ClosestFactory].whitelist.end()){
                        w.jobHunting = false;
                    } else {
                        //think about jobhunting, while waiting for other people to opt out first
                        w.jobHunting = true;
                    }
                }
            } else {
                inherentDesire(w.desireLevel, MetroRadius, FactoryRadius, w.desireChangeRate, w);
                facingToward(w.movingTarget, w);
            }
        }
        //default behaviors
        Vec3f sep(separate(others, w));
        sep *= w.separateForce;
        applyForce(sep, w);     

        //noiseLevelUpdate();
        borderDetect(w);
        update(w);
        moneyConsumption(w);
    }

    void moneyConsumption(Worker& w){
        w.moneyTimer ++;
        if (w.moneyTimer == 0){
            w.lastSavings = w.capitalHoldings;
            w.monthlyTotal = 0;
        }
        if (w.moneyTimer % 60 == 0){
            w.currentSavings = w.capitalHoldings;
            w.todayIncome = w.currentSavings - w.lastSavings;
            w.monthlyTotal += w.todayIncome;
            w.lastSavings = w.currentSavings;
        }
        if (w.moneyTimer > 60 * 15){
            w.monthlyIncome = w.monthlyTotal;
            w.dailyIncome = w.monthlyIncome / 30;
            w.moneyTimer = 0;
        }
        //cout << dailyIncome << " daily income " << endl;
        //cout << monthlyIncome << " monthly income " << endl;'
        if (w.monthlyIncome > 5000 && w.monthlyIncome <= 8000){
            w.incomeTax = w.dailyIncome * 0.008;
            w.livingCost = 2.0;
        } else if (w.monthlyIncome > 8000 && w.monthlyIncome <= 15000){
            w.incomeTax = w.dailyIncome * 0.012;
            w.livingCost = 3.0;
        } else if (w.monthlyIncome > 15000 ){
            w.incomeTax = w.dailyIncome * 0.018;
            w.livingCost = 5.0;
        } else if (w.monthlyIncome <= 5000){
            w.incomeTax = 0;
            w.livingCost = 1.5;
        }


        w.capitalHoldings -= w.livingCost + w.incomeTax;
        if (w.capitalHoldings <= -2000){
            w.capitalHoldings = -2000;
        } else if (w.capitalHoldings >= 9999999){
            w.capitalHoldings = 9999999;
        }
    } //money consumption

    void work(float diligency, int mood, float radius, vector<Factory>& fs, Worker& w){
        if (w.bioClock == 0){
            w.workTarget = r() * radius + fs[w.id_ClosestFactory].position;
        }
        if (w.bioClock % mood == 0) {
            w.workTarget = r() * radius + fs[w.id_ClosestFactory].position;
        }
        if (w.bioClock >= mood * 12 - 1){
            w.bioClock = 0;
            mood = r_int(30, 90);
        }
        w.bioClock ++;

        Vec3f workAround(seek(w.workTarget, w));
        workAround *= diligency;
        applyForce(workAround, w);
        facingToward(w.workTarget, w);
    }// work

    Vec3f separate(vector<Worker>& others, Worker& w){
        Vec3f sum;
        int count = 0;
        w.neighborNum = 0;
        for (Worker w : others){
            Vec3f difference = w.pose().pos() - w.pose().pos();
            float d = difference.mag();
            if ((d > 0) && (d < w.desiredseparation)){
                Vec3f diff = difference.normalize();
                sum += diff;
                count ++;
                w.neighborNum ++;
            }
        }
        if (count > 0){
            sum /= count;
            sum.mag(w.maxspeed);
            Vec3f steer = sum - w.velocity;
            if (steer.mag() > w.maxforce){
                steer.normalize(w.maxforce);
            }
            return steer;
        } else {
            return Vec3f(0,0,0);
        }
    } // seperate

    void senseFactory(vector<Factory>& fs, Worker& w){
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
        Vec3f dist_differenceA = w.pose().pos() - fs[min_EOR_id].position;
        float distA = dist_differenceA.mag();
        Vec3f dist_differenceB = w.pose().pos() - fs[max_material_id].position;
        float distB = dist_differenceB.mag();

        //cout << min_EOR_id << " most empty fac" << fs[min_EOR_id].workersWorkingNum / fs[min_EOR_id].workersNeededNum << "  empty ratio" <<endl;
        //cout << max_material_id << "  = most masterial fac " << fs[max_material_id].materialStocks << " material num " << endl;
        
        if (distA < distB){
            w.distToClosestFactory = distA;
            w.id_ClosestFactory = min_EOR_id;
        } else {
            w.distToClosestFactory = distB;
            w.id_ClosestFactory = max_material_id;
        }

        //cout << openingCount << endl;
        if (openingCount == 0){
            w.depression = true;
        } else {
            w.depression = false;
        }
        if (w.distToClosestFactory < w.sensitivityFactory){
            w.FactoryFound = true;
        } else {
            w.FactoryFound = false;
        }
    } // sense factory

    void seekFactory(vector<Factory>& fs, Worker& w){
        if (fs[w.id_ClosestFactory].operating()){
            Vec3f skFS(seek(fs[w.id_ClosestFactory].position, w));
            skFS *= 1.0;
            applyForce(skFS, w);
            Vec3f t = fs[w.id_ClosestFactory].position;
            facingToward(t, w);
        }
    }// seek factory

    void seekCapitalist(vector<Capitalist>& capitalist, Worker& w){
        if (!capitalist[w.id_ClosestFactory].bankrupted()){
            Vec3f skCP(seek(capitalist[w.id_ClosestFactory].pose().pos(),w));
            skCP *= 1.0;
            applyForce(skCP, w);
            Vec3f t = capitalist[w.id_ClosestFactory].pose().pos();
            facingToward(t, w);
        } else {
            inherentDesire(w.desireLevel, MetroRadius, FactoryRadius, w.desireChangeRate, w);
            facingToward(w.movingTarget, w);
        }
    } // seek capitalist

    void findPoems(Worker& w){
        //30% probability
        if (rnd::prob(0.0001)) {
            w.poetryHoldings += 1;
        };
    } // find poems

    //base behavior//

    void update(Worker& w){
        w.velocity += w.acceleration;
        if (w.velocity.mag() > w.maxspeed){
            w.velocity.normalize(w.maxspeed);
        }
        w.pose().pos() += w.velocity;
        w.acceleration *= 0; //zeros acceleration
        
    } // worker -> update

    Vec3f seek(Vec3f target, Worker& w){
        Vec3f desired = target - w.pose().pos();
        desired.normalized();
        desired *= w.maxspeed;
        Vec3f steer = desired - w.velocity;
        if (steer.mag() > w.maxforce){
            steer.normalize(w.maxforce);
        }
        return steer;
    } // worker->seek

    void applyForce(Vec3f force, Worker& w){
      Vec3f f = force / w.mass;
      w.acceleration += f;
      if (w.acceleration.mag() > w.maxAcceleration){
        w.acceleration.normalize(w.maxAcceleration);
      }
    } //worker -> applyForce

    void facingToward(Vec3f& target, Worker& w){
        //change facing direction based on target
        Vec3f src = Vec3f(w.pose().quat().toVectorZ()).normalize();
        Vec3f dst = Vec3f(target - w.pose().pos()).normalize();
        Quatd rot = Quatd::getRotationTo(src,dst);
        w.pose().quat() = rot * w.pose().quat();
    } //worker -> facingToward

    void inherentDesire(float desireLevel, float innerRadius, float outerRadius, int changeRate, Worker& w){
        //inherent desire that changes everyday
        //let them search for something in the metropolis
        w.bioClock++;
        if (w.bioClock % changeRate == 0) {
            w.movingTarget = r();
            Vec3f temp_pos = w.movingTarget;
            w.movingTarget = w.movingTarget * (outerRadius - innerRadius) + temp_pos.normalize(innerRadius);
        }
        if (w.bioClock > 1440 ){
            w.bioClock = 0;
        }
        Vec3f skTarget(seek(w.movingTarget, w));
        skTarget *= desireLevel;
        applyForce(skTarget, w);

        Vec3f ar(arrive(w.movingTarget, w));
        ar *= 0.3;
        applyForce(ar, w);
    } // worker -> inherentDesire

    Vec3f arrive(Vec3f& target, Worker& w){
        Vec3f desired = target - w.pose().pos();
        float d = desired.mag();
        desired.normalize();
        if (d < w.target_senseRadius){
            float m = MapValue(d, 0, w.target_senseRadius, w.minspeed, w.maxspeed);
            desired *= m;
        } else {
            desired *= w.maxspeed;
        }
        Vec3f steer = desired - w.velocity;
        if (steer.mag() > w.maxforce){
            steer.normalize(w.maxforce);
        }
        return steer;
    } // worker -> arrive

    void borderDetect(Worker& w){
        Vec3f origin(0,0,0);
        Vec3f distance = w.pose().pos() - origin;
        if (distance.mag() > boundary_radius) {
            Vec3f desired = origin - w.pose().pos();
            Vec3f steer = desired - w.velocity;
            if (steer.mag() > w.maxforce) {
                steer.normalize(w.maxforce);
            }
            applyForce(steer, w);
        } else {
            applyForce(Vec3f(0,0,0), w);
        }
    } // worker -> borderDetect


    void visualize(vector<Factory>& fs){
        if (drawingLinks){
            for (int i = workers.size() - 1; i >= 0; i--){
                if (workers[i].FactoryFound && !workers[i].bankrupted()){
                    lines[i].vertices()[0] = workers[i].pose().pos();
                    lines[i].vertices()[1] = fs[workers[i].id_ClosestFactory].position;
                } else {
                    lines[i].vertices()[0] = Vec3f(0,0,0);
                    lines[i].vertices()[1] = Vec3f(0,0,0);
                    // lines[i].vertices()[0] = ms[i].pose().pos();
                    // lines[i].vertices()[1] = nrps[ms[i].id_ClosestNRP].position;
                }
            }
        } else {
            for (int i = workers.size() - 1; i >= 0; i--){
                lines[i].vertices()[0] = Vec3f(0,0,0);
                lines[i].vertices()[1] = Vec3f(0,0,0);
            }
        }
    }
    void draw(Graphics& g){
        for (int i = workers.size() - 1; i >=0; i --){
            Worker& w = workers[i];
            w.draw(g);
            g.color(0.4,0.65,1);
            g.draw(lines[i]);
        }
    }
};

struct Miner_Group{
    vector<Miner> ms;
    int initial_num;

    //visualize relations
    vector<Line> lines;
    bool drawingLinks;

    float resourceUnitPrice = 280;
    int richCapitalistID = 0;

    Miner_Group(){
        initial_num = 100;
        ms.resize(initial_num);
        lines.resize(ms.size());
        drawingLinks = true;
        cout << "Miner Group joined" << endl;
    }
    Miner operator[] (const int index) const{
        return ms[index];
    }
    void run(vector<Natural_Resource_Point>& nrps, vector<Miner>& others, vector<Capitalist>& capitalists){
        //cout << "Miner Group running fast" << endl;
        //sense rich capitalist
        senseRichCapitalist(capitalists);

        for (int i = ms.size() - 1; i >=0; i --){
            Miner& m = ms[i];
            if (!m.bankrupted()){
                //m.run(nrps, others, capitalists);
                compute(nrps, others, capitalists, m);
            }
        }
        //drawing links
        visualize(nrps);
    }

    void compute(vector<Natural_Resource_Point>& nrps, vector<Miner>& others, vector<Capitalist>& capitalists, Miner& m){
        if (m.resourceHoldings < m.maxLoad){
            m.fullpack = false;
            //resource mining
            m.patienceTimer += 1;
            if (m.patienceTimer == m.patienceLimit){
                if (m.numNeighbors > m.friendliness){
                senseFruitfulPoints(nrps, m);
                } else {
                senseResourcePoints(nrps, m);
                }
                m.patienceTimer = 0;
            }
            
            if (m.resourcePointFound == true){
                m.separateForce = 1.2;
                if (m.distToClosestNRP > m.sensitivityResource){
                    seekResourcePoint(nrps, m);
                } else if (m.distToClosestNRP < m.sensitivityResource && m.distToClosestResource >= 0) {
                    collectResource(nrps, m);
                    if (m.distToClosestResource < m.pickingRange){
                        m.collectTimer ++;
                        if (m.collectTimer % (int)floorf(60.0 / m.collectRate) == 0){
                            m.resourceHoldings += 1;
                            //cout << collectTimer << endl;
                        }
                        if (m.collectTimer >= (int)floorf(60.0 / m.collectRate) * 12 - 1){
                            m.collectTimer = 0;
                        }
                    }
                }
            } else {
                m.separateForce = 0.3;
                inherentDesire(m.desireLevel, FactoryRadius, NaturalRadius, m.desireChangeRate, m);
                facingToward(m.movingTarget, m);
            } 
        } else if (m.resourceHoldings >= m.maxLoad) {
            m.fullpack = true;
            //find capitalist for a trade
            m.resourcePointFound = false;
            senseCapitalists(capitalists, m);
            if (m.capitalistNearby){
                if (m.distToClosestCapitalist > m.businessDistance){
                    seekCapitalist(capitalists, m);
                } else if (m.distToClosestCapitalist <= m.businessDistance && m.distToClosestCapitalist >= 0){
                    exchangeResource(capitalists, m);
                    m.exchanging = true;
                    m.tradeTimer ++;
                }
            } else {
                inherentDesire(m.desireLevel, FactoryRadius, NaturalRadius, m.desireChangeRate, m);
                facingToward(m.movingTarget, m);
            }
            if (m.tradeTimer == m.unloadTimeCost){
                //cout << "transaction finished" << endl;
                m.capitalHoldings += resourceUnitPrice * m.resourceHoldings;
                m.resourceHoldings = 0;
                m.exchanging = false;
                m.tradeTimer = 0;
            }

        }
        
        // cout << resourcePointFound << "found??" << endl;
        // cout << distToClosestNRP << " dist to closeset nrp"<< endl;
        // cout<< id_ClosestNRP << "  NRP" << endl;
        // cout << id_ClosestResource << " resource" << endl;

        //default behaviors
        Vec3f sep(separate(others, m));
        sep *= m.separateForce;
        applyForce(sep, m);     

        borderDetect(m);
        update(m);
        moneyConsumption(m);
    }

    void calculateResourceUnitPrice(vector<Factory>& factories){
        //miner group pull the latest resource price from the factory
        resourceUnitPrice = factories[0].resourceUnitPrice;
    }


    void visualize(vector<Natural_Resource_Point>& nrps){
        if (drawingLinks){
            for (int i = ms.size() - 1; i >= 0; i--){
                if (ms[i].resourcePointFound && !ms[i].bankrupted()){
                    lines[i].vertices()[0] = ms[i].pose().pos();
                    lines[i].vertices()[1] = nrps[ms[i].id_ClosestNRP].resources[ms[i].id_ClosestResource].position;
                } else {
                    lines[i].vertices()[0] = Vec3f(0,0,0);
                    lines[i].vertices()[1] = Vec3f(0,0,0);
                    // lines[i].vertices()[0] = ms[i].pose().pos();
                    // lines[i].vertices()[1] = nrps[ms[i].id_ClosestNRP].position;
                }
            }
        } else {
            for (int i = ms.size() - 1; i >= 0; i--){
                lines[i].vertices()[0] = Vec3f(0,0,0);
                lines[i].vertices()[1] = Vec3f(0,0,0);
            }
        }
    }

    //compute behaviors
    void senseResourcePoints(vector<Natural_Resource_Point>& nrps, Miner& m){
        float min = 999;
        int min_id = 0;
        for (size_t i = 0; i < nrps.size(); i++){
            if (!nrps[i].drained()){
                Vec3f dist_difference = m.pose().pos() - nrps[i].position;
                float dist = dist_difference.mag();
                if (dist < min){
                    min = dist;
                    min_id = i;
                        //update universal variable for other functions to use
                    m.distToClosestNRP = dist;
                    m.id_ClosestNRP = min_id;
                }
            }
        }
        if (m.distToClosestNRP < m.sensitivityNRP){
            m.resourcePointFound = true;
        } else {
            m.resourcePointFound = false;
        }
    } //sense resource points

    void senseFruitfulPoints(vector<Natural_Resource_Point>& nrps, Miner& m){
        float maxFruitfulness = 0;
        int max_id = 0;     
        for (size_t i = 0; i < nrps.size(); i++){
            if (!nrps[i].drained()){
                Vec3f dist_difference = m.pose().pos() - nrps[i].position;
                float dist = dist_difference.mag();
                if (dist < m.sensitivityNRP){
                    if (nrps[i].fruitfulness > maxFruitfulness){
                        maxFruitfulness = nrps[i].fruitfulness;
                        max_id = i;
                        
                        //update universal variable for other functions to use
                        m.distToClosestNRP = dist;
                        m.id_ClosestNRP = max_id;
                    }
                }
            }
        }
        if (m.distToClosestNRP < m.sensitivityNRP){
            m.resourcePointFound = true;
        } else {
            m.resourcePointFound = false;
        }
    }//sense fruitful resource
    void seekResourcePoint(vector<Natural_Resource_Point>& nrps, Miner& m){
        if (!nrps[m.id_ClosestNRP].drained()){
            Vec3f skNRP(seek(nrps[m.id_ClosestNRP].position, m));
            skNRP *= m.searchResourceForce;
            applyForce(skNRP, m);
            facingToward(nrps[m.id_ClosestNRP].position, m);
        } 
    }//seek resource point
    void exchangeResource(vector<Capitalist>& capitalists, Miner& m){
        Vec3f t = capitalists[m.id_ClosestCapitalist].pose().pos();
        Vec3f arCP(arrive(t, m));
        arCP *= 1.0;
        applyForce(arCP, m);
    } // exchange resource
    void collectResource(vector<Natural_Resource_Point>& nrps, Miner& m){
        float min = 9999;
        int min_id = 0;
        if (!nrps[m.id_ClosestNRP].drained()){
            for (int i = nrps[m.id_ClosestNRP].resources.size() - 1; i >= 0; i--){
                if (!nrps[m.id_ClosestNRP].resources[i].isPicked){
                    Vec3f dist_difference = m.pose().pos() - nrps[m.id_ClosestNRP].resources[i].position;
                    double dist = dist_difference.mag();
                    if (dist < min){
                        min = dist;
                        min_id = i;
                        m.id_ClosestResource = min_id;
                        m.distToClosestResource = dist;
                    }
                }
            }
            Vec3f collectNR(seek(nrps[m.id_ClosestNRP].resources[min_id].position, m));
            collectNR *= m.collectResourceForce;
            applyForce(collectNR, m);
            facingToward(nrps[m.id_ClosestNRP].resources[min_id].position, m);
        }
        findPoems(m); 
    } //collect resource
    void findPoems(Miner& m){
        //30% probability
        if (rnd::prob(0.0001)) {
            m.poetryHoldings += 1;
        };
    }//find poems

    void senseRichCapitalist(vector<Capitalist>& capitalists){
        float max_capitals = 0;
        int max_rich_id = 0;
        for (size_t i = 0; i < capitalists.size(); i++){
            if (!capitalists[i].bankrupted()){
                if (capitalists[i].capitalHoldings > max_capitals){
                    max_capitals = capitalists[i].capitalHoldings;
                    max_rich_id = i;
                }
            }
        }
        richCapitalistID = max_rich_id;
    } //sense rich capitalist
    
    void senseCapitalists(vector<Capitalist>& capitalists, Miner& m){
        float min_resources = 999999;
        int min_resource_id = 0;
        for (size_t i = 0; i < capitalists.size(); i++){
            if (!capitalists[i].bankrupted()){
                //find the one needs resource
                if (capitalists[i].totalResourceHoldings < min_resources){
                    min_resources = capitalists[i].totalResourceHoldings;
                    min_resource_id = i;
                }
            }
        }
        Vec3f dist_difference = m.pose().pos() - capitalists[min_resource_id].pose().pos();
        float dist_resource = dist_difference.mag();
        Vec3f dist_difference_2 = m.pose().pos() - capitalists[richCapitalistID].pose().pos();
        float dist_rich = dist_difference_2.mag();
        
        if (dist_resource > dist_rich){
            m.distToClosestCapitalist = dist_resource;
            m.id_ClosestCapitalist = min_resource_id;
        } else if (dist_resource <= dist_rich){
            m.distToClosestCapitalist = dist_rich;
            m.id_ClosestCapitalist = richCapitalistID;
        }

        if (m.distToClosestCapitalist < m.sensitivityCapitalist){
            m.capitalistNearby = true;
        } else {
            m.capitalistNearby = false;
        }
    }//sense capitalists
    void seekCapitalist(vector<Capitalist>& capitalists, Miner& m){
        if (!capitalists[m.id_ClosestCapitalist].bankrupted()){
            Vec3f skCP(seek(capitalists[m.id_ClosestCapitalist].pose().pos(), m));
            skCP *= 1.0;
            applyForce(skCP, m);
            Vec3f t = capitalists[m.id_ClosestCapitalist].pose().pos();
            facingToward(t, m);
        }
    }//seek capitalist

    void moneyConsumption(Miner& m){
        m.moneyTimer ++;
        if (m.moneyTimer == 0){
            m.lastSavings = m.capitalHoldings;
            m.monthlyTotal = 0;
        }
        if (m.moneyTimer % 60 == 0){
            m.currentSavings = m.capitalHoldings;
            m.todayIncome = m.currentSavings - m.lastSavings;
            m.monthlyTotal += m.todayIncome;
            m.lastSavings = m.currentSavings;
        }
        if (m.moneyTimer > 60 * 15){
            m.monthlyIncome = m.monthlyTotal;
            m.dailyIncome = m.monthlyIncome / 30;
            m.moneyTimer = 0;
        }
        if (m.monthlyIncome > 5000 && m.monthlyIncome <= 8000){
            m.incomeTax = m.dailyIncome * 0.008;
        } else if (m.monthlyIncome > 8000 && m.monthlyIncome <= 15000){
            m.incomeTax = m.dailyIncome * 0.012;
        } else if (m.monthlyIncome > 15000 ){
            m.incomeTax = m.dailyIncome * 0.018;
        } else if (m.monthlyIncome <= 5000){
            m.incomeTax = 0;
            m.povertyWelfare = - m.livingCost * 0.3;
        }

        m.capitalHoldings -= m.livingCost + m.incomeTax + m.povertyWelfare;
        if (m.capitalHoldings <= -1000){
            m.capitalHoldings = -1000;
        } else if (m.capitalHoldings >= 9999999){
            m.capitalHoldings = 9999999;
        }
    }//money consumption

    Vec3f separate(vector<Miner>& others, Miner& me){
        Vec3f sum;
        int count = 0;
        int neighborCount = 0;
        for (Miner m : others){
            Vec3f difference = me.pose().pos() - m.pose().pos();
            float d = difference.mag();
            if ((d > 0) && (d < me.desiredseparation)){
                Vec3f diff = difference.normalize();
                sum += diff;
                count ++;
            }
            if ((d >0 ) && (d < me.neightSenseRange)){
                neighborCount ++;
            }
        }
        me.numNeighbors = neighborCount;
        //cout << numNeighbors << " num neighbors" << endl;
        if (count > 0){
            sum /= count;
            sum.mag(me.maxspeed);
            Vec3f steer = sum - me.velocity;
            if (steer.mag() > me.maxforce){
                steer.normalize(me.maxforce);
            }
            return steer;
        } else {
            return Vec3f(0,0,0);
        }
    }//

    //base behavior//

    void update(Miner& m){
        m.velocity += m.acceleration;
        if (m.velocity.mag() > m.maxspeed){
            m.velocity.normalize(m.maxspeed);
        }
        m.pose().pos() += m.velocity;
        m.acceleration *= 0; //zeros acceleration
        
    } // Miner -> update

    Vec3f seek(Vec3f target, Miner& m){
        Vec3f desired = target - m.pose().pos();
        desired.normalized();
        desired *= m.maxspeed;
        Vec3f steer = desired - m.velocity;
        if (steer.mag() > m.maxforce){
            steer.normalize(m.maxforce);
        }
        return steer;
    } // Miner->seek

    void applyForce(Vec3f force, Miner& m){
      Vec3f f = force / m.mass;
      m.acceleration += f;
      if (m.acceleration.mag() > m.maxAcceleration){
        m.acceleration.normalize(m.maxAcceleration);
      }
    } //Miner -> applyForce

    void facingToward(Vec3f target, Miner& m){
        //change facing direction based on target
        Vec3f dst = Vec3f(target - m.pose().pos());

        //if too close, zero the distance
        //to make it not drifting back and forth
        if (abs(dst) < 0.05){
            dst = Vec3f(0,0,0);
        }

        m.srcRot = Vec3f(m.pose().quat().toVectorZ()).normalize();
        m.dstRot = dst.normalize();
        //m.dstRot = Vec3f(target - m.pose().pos()).normalize();
        m.cRot = Quatd::getRotationTo(m.srcRot,m.dstRot);
        m.pose().quat() = m.cRot * m.pose().quat();
    } //Miner -> facingToward

    void inherentDesire(float desireLevel, float innerRadius, float outerRadius, int changeRate, Miner& m){
        //inherent desire that changes everyday
        //let them search for something in the metropolis
        m.bioClock++;
        if (m.bioClock % changeRate == 0) {
            m.movingTarget = r();
            m.temp_pos = m.movingTarget;
            m.movingTarget = m.movingTarget * (outerRadius - innerRadius) + m.temp_pos.normalize(innerRadius);
        }
        if (m.bioClock > 1440 ){
            m.bioClock = 0;
        }
        Vec3f skTarget(seek(m.movingTarget, m));
        skTarget *= desireLevel;
        applyForce(skTarget, m);

        Vec3f ar(arrive(m.movingTarget, m));
        ar *= 0.3;
        applyForce(ar, m);
    } // Miner -> inherentDesire

    Vec3f arrive(Vec3f& target, Miner& me){
        Vec3f desired = target - me.pose().pos();
        float d = desired.mag();
        desired.normalize();
        if (d < me.target_senseRadius){
            float m = MapValue(d, 0, me.target_senseRadius, me.minspeed, me.maxspeed);
            desired *= m;
        } else {
            desired *= me.maxspeed;
        }
        Vec3f steer = desired - me.velocity;
        if (steer.mag() > me.maxforce){
            steer.normalize(me.maxforce);
        }
        return steer;
    } // Miner-> arrive

    void borderDetect(Miner& m){
        Vec3f origin(0,0,0);
        Vec3f distance = m.pose().pos() - origin;
        if (distance.mag() > boundary_radius) {
            Vec3f desired = origin - m.pose().pos();
            Vec3f steer = desired - m.velocity;
            if (steer.mag() > m.maxforce) {
                steer.normalize(m.maxforce);
            }
            applyForce(steer, m);
        } else {
            applyForce(Vec3f(0,0,0), m);
        }
    } // Miner -> borderDetect

    
    void draw(Graphics& g){
        for (int i = ms.size() - 1; i >=0; i --){
            Miner& m = ms[i];
            m.draw(g);
            g.color(0.6,1,0.6);
            g.draw(lines[i]);
        }
    }
};

#endif
