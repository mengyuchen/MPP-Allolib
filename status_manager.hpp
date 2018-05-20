#ifndef INCLUDE_STATUS_MANAGER_HPP
#define INCLUDE_STATUS_MANAGER_HPP

#include "al/core/app/al_App.hpp"
#include "agent_managers.hpp"
#include "location_managers.hpp"

using namespace al;
using namespace std;


struct MarketManager{
    //price
    float resourceUnitPrice;
    float laborUnitPrice;

    //population
    float numCapitalists;
    float numWorkers;
    float numMiners;
    float liveCapitalists;
    float liveWorkers;
    float liveMiners;
    float liveFactories;
    float jobHuntingWorkers;
    float poorCapitalists;
    float poorWorkers;
    float poorMiners;
    float richWorkers;
    float richMiners;
    float richCapitalists;

    //stats
    float averageCollectRate;
    float averageMaxLoad;
    float averageMinerSpeed;
    float averageCapitalistWealth;
    float averageFactoryGrossProfits;
    float averageWorkerWealth;
    float averageMinerWealth;
    float averageFactoryCapitalReserve;
    float averageWorkersPayCheck;
    float averageFactoryMaterialStocks;
    float averageCapitalistResource;
    float averageMinerPoetryLevel;
    float averageWorkerPoetryLevel;
    float averageCapitalistPoetryLevel;
    float MinerCapitalistRatio;
    float WorkerCapitalistRatio;

    //inner class stratification
    float minerWealthLine;
    float minerPovertyLine;
    float workerWealthLine;
    float workerPovertyLine;
    float capitalistWealthLine;
    float capitalistPovertyLine;

    //invisible hand
    float resourcePopulationFactor;
    float laborPolulationFactor;
    float minerPovertyRate;
    float minerWealthRate;
    float workerPovertyRate;
    float workerWealthRate;

    //misc
    float statusTimer;

    MarketManager(){
        statusTimer = 0;
        numCapitalists = 1;
        numWorkers = 1;
        numMiners = 1;
        liveCapitalists = 1;
        liveWorkers = 1;
        liveMiners = 1;
        liveFactories = 1;
        jobHuntingWorkers = 1;

        //miners stats
        averageCollectRate = 0.5;
        averageMaxLoad = 12;
        averageMinerSpeed = 0.3;

        //market
        resourceUnitPrice = 150;
        laborUnitPrice = 250;
        resourcePopulationFactor = 1.0;
        laborPolulationFactor = 1.0;

        //inclass stratification
        minerPovertyLine = 1000;
        minerWealthLine = 8000;
        workerPovertyLine = 2000;
        workerWealthLine = 30000;
        capitalistPovertyLine = 15000;
        capitalistWealthLine = 500000;

    }
    void statsInit(Capitalist_Entity& capitalists, Worker_Union& workers, Miner_Group& miners){
        numCapitalists = capitalists.cs.size();
        liveCapitalists = numCapitalists;
        numWorkers = workers.workers.size();
        liveWorkers = numWorkers;
        jobHuntingWorkers = workers.workers.size();
        numMiners = miners.ms.size();
        liveMiners = numMiners;

        //calculate Miner stats
        float sum1 = 0;
        float sum2 = 0;
        float sum3 = 0;
        for (size_t i = 0; i < miners.ms.size(); i++){
            sum1 += miners.ms[i].collectRate;
            sum2 += miners.ms[i].maxLoad;
            sum3 += miners.ms[i].maxspeed;
        }
        averageCollectRate = sum1 / miners.ms.size();
        averageMaxLoad = sum2 / miners.ms.size();
        averageMinerSpeed = sum3 / miners.ms.size();
        cout << averageCollectRate << "avg collectrate" << endl;
        cout << averageMaxLoad << "avg maxload "<< endl;
        cout << averageMinerSpeed << "avg miner max speed " << endl;

        //initialize price
        resourceUnitPrice = (1 / averageCollectRate * 60 + NaturalRadius / averageMaxLoad / averageMinerSpeed * 2) * ((float)numMiners * 2 / (1 + (float)liveMiners));
        //time is money, frames needed for each resource + travel distance / velocity divided by each resource, plus some extra time
        //the more miners, the cheaper resource, and vice versa
        //resource per second * frameRate = minimum Money consumption rate each frame
        laborUnitPrice = 0.5 * resourceUnitPrice + resourceUnitPrice * ( ((float)numWorkers - (float)jobHuntingWorkers) / (float)liveWorkers);
        

    }
    void populationMonitor(Capitalist_Entity& capitalists, Worker_Union& workers, Miner_Group& miners, vector<Factory>& factories){
        liveCapitalists = 0;
        liveWorkers = 0;
        liveMiners = 0;
        liveFactories = 0;
        poorCapitalists = 0;
        poorMiners = 0;
        poorWorkers = 0;
        richCapitalists = 0;
        richMiners = 0;
        richWorkers = 0;
        jobHuntingWorkers = 0;
        for (size_t i = 0; i < capitalists.cs.size(); i ++){
            if (!capitalists.cs[i].bankrupted()){
                liveCapitalists += 1;
                if (capitalists.cs[i].capitalHoldings < capitalistPovertyLine){
                    poorCapitalists += 1; 
                } else if (capitalists.cs[i].capitalHoldings > capitalistWealthLine){
                    richCapitalists += 1;
                }
            }
        }
        for (size_t i = 0; i < workers.workers.size(); i ++){
            if (!workers.workers[i].bankrupted()){
                liveWorkers += 1;
                if (workers.workers[i].jobHunting == true){
                    jobHuntingWorkers += 1;
                }
                if (workers.workers[i].capitalHoldings < workerPovertyLine){
                    poorWorkers += 1;
                } else if (workers.workers[i].capitalHoldings > workerWealthLine){
                    richWorkers += 1;
                }
            }
        }
        WorkerCapitalistRatio = liveWorkers / liveCapitalists;
        for (size_t i = 0; i < miners.ms.size(); i++){
            if (!miners.ms[i].bankrupted()){
                liveMiners += 1;
                if (miners.ms[i].capitalHoldings < minerPovertyLine){
                    poorMiners += 1; 
                } else if (miners.ms[i].capitalHoldings > minerWealthLine){
                    richMiners += 1;
                }
            }
        }
        MinerCapitalistRatio = liveMiners / liveCapitalists;

        for (size_t i = 0; i < factories.size(); i ++){
            if (factories[i].operating()){
                liveFactories += 1;
            }
        }
        //cout << liveCapitalists << " cp " << liveWorkers << " ws " << liveMiners << " ms " << endl;
    }
    void capitalMonitor(Capitalist_Entity& capitalists, Worker_Union& workers, Miner_Group& miners, vector<Factory>& factories){
        float sum1 = 0;
        float sum2 = 0;
        float sum3 = 0;
        float sum4 = 0;
        float sum5 = 0;
        float sum6 = 0;
        float sum7 = 0;
        float sum8 = 0;
        float sum9 = 0;
        float sum10 = 0;
        float sum11 = 0;
        for (size_t i = 0; i < capitalists.cs.size(); i ++){
            sum1 += capitalists.cs[i].capitalHoldings;
            sum6 += capitalists.cs[i].workersPayCheck;
            sum8 += capitalists.cs[i].resourceHoldings;
            sum11 += capitalists.cs[i].poetryHoldings;
        }
        averageCapitalistWealth = sum1 / liveCapitalists;
        averageWorkersPayCheck = sum6 / liveCapitalists;
        averageCapitalistResource = sum8 / liveCapitalists;
        averageCapitalistPoetryLevel = sum11 / liveCapitalists;

        for (size_t i = 0; i < workers.workers.size(); i ++){
            sum2 += workers.workers[i].capitalHoldings;
            sum10 += workers.workers[i].poetryHoldings;

        }
        averageWorkerWealth = sum2 / liveWorkers;
        averageWorkerPoetryLevel = sum10 / liveWorkers;

        for (size_t i = 0; i < miners.ms.size(); i++){
            sum3 += miners.ms[i].capitalHoldings;
            sum9 += miners.ms[i].poetryHoldings;
        }
        averageMinerWealth = sum3 / liveMiners;
        averageMinerPoetryLevel = sum9 / liveMiners;

        for (size_t i = 0; i < factories.size(); i ++){
            sum4 += factories[i].grossProfits;
            sum5 += factories[i].capitalReserve;
            sum7 += factories[i].materialStocks;
        }
        averageFactoryGrossProfits = sum4 / factories.size();
        averageFactoryCapitalReserve = sum5 / factories.size();
        averageFactoryMaterialStocks = sum7 / factories.size();


        // cout << averageCapitalistWealth << " = avg cp wealth  " << averageWorkerWealth << "  = avg wk wealth " << endl;
        // cout << averageMinerWealth << " = avg ms wealth  " << averageFactoryGrossProfits << " =  avg gross profits" << endl;
        // cout << averageFactoryCapitalReserve << " = avg capital reserve in fs " << averageWorkersPayCheck << " = avg paycheck" << endl; 
        // cout << resourceUnitPrice << " = resource unit price  "<< laborUnitPrice << " = labor unit price" <<endl;
        // cout << averageFactoryMaterialStocks << " = avg material stock " << averageCapitalistResource << "  = avg captialits resource holds"<<endl;
        // cout << liveFactories << " = live Factories operating" << endl;
        // cout << richCapitalists << " rich capitalists || " << poorCapitalists << " poor capitalist, out of  " << liveCapitalists << " live capitalists" << endl;
        // cout << richMiners << " rich Miners || " << poorMiners << " poor miners, out of " << liveMiners << " live miners" << endl;
        // cout << richWorkers << " rich workers || " << poorWorkers << " poor Workers, out of " << liveWorkers << " live workers, also " << jobHuntingWorkers << " need a job " <<endl;
        // cout << averageMinerPoetryLevel << "= avg Miner Poety " << averageWorkerPoetryLevel << " = avg Worker Poetry " << averageCapitalistPoetryLevel << " = avg Capitalist poetry" << endl;
    }
    void updatePrice(Capitalist_Entity& capitalists, Worker_Union& workers, Miner_Group& miners){
        minerPovertyRate = poorMiners / liveMiners;
        minerWealthRate = richMiners / liveMiners;
        if (minerPovertyRate >= 0.1 && minerPovertyRate < 0.3){
            resourcePopulationFactor = 1.2 + poorMiners / liveMiners;
        } else if (minerPovertyRate >= 0.3 && minerPovertyRate < 0.5){
            resourcePopulationFactor = 1.5 + poorMiners / liveMiners;
        } else if (minerPovertyRate >= 0.5){
            resourcePopulationFactor = 1.8 + poorMiners / liveMiners;
        } else if (minerPovertyRate < 0.1){
            resourcePopulationFactor = 1.0 + poorMiners / liveMiners;;
        }
        workerPovertyRate = poorWorkers / liveWorkers;
        workerWealthRate = richWorkers / liveWorkers;

        //poverty prevention mechanism
        if (workerPovertyRate >= 0.1 && workerPovertyRate < 0.3){
            laborPolulationFactor = 1.0 + poorWorkers / liveWorkers;
        } else if (workerPovertyRate >= 0.3 && workerPovertyRate < 0.5){
            laborPolulationFactor = 1.5 + poorWorkers / liveWorkers;
        } else if (workerPovertyRate >= 0.5){
            laborPolulationFactor = 1.8 + poorWorkers / liveWorkers;
        } else if (workerPovertyRate < 0.1){
            laborPolulationFactor = 1.0 + poorWorkers / liveWorkers;
        }

        //over-rich prevention mechanism is paying more tax, so far

        resourceUnitPrice = (1 / averageCollectRate * 60 + NaturalRadius * 4 / averageMaxLoad / averageMinerSpeed) * ( 1 + (numMiners / (1 + liveMiners)) / 10 ) * resourcePopulationFactor;
        laborUnitPrice = (0.5 * resourceUnitPrice + resourceUnitPrice * ( (liveWorkers - jobHuntingWorkers) /  (1 + liveWorkers) ) ) * laborPolulationFactor;
        
    }
    void monitorResourceStatus(vector<Natural_Resource_Point> nrps){

    }
    
};


#endif
