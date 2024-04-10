#ifndef WORLDBUILDER_H
#define WORLDBUILDER_H
#include "sensor.h"
//#include "box2d/box2d.h"

// class BoxFeatures{
//     private:#include "general.h"

//     float halfWindowWidth =.1;
//     public:
    

// };



class WorldBuilder{
    int bodies=0;
    public:
    int iteration=0;
    bool debug =0;
    char bodyFile[100];
    float simulationStep=BOX2DRANGE;
    //int buildType=0;
    std::pair <CoordinateContainer, bool> salientPoints(b2Transform, CoordinateContainer, std::pair <Pointf, Pointf>); //gets points from the raw data that are relevant to the task based on bounding boxes
                                                                                                                                        //std::pair<points, obstaclestillthere>
    void makeBody(b2World&, BodyFeatures);

    std::vector <BodyFeatures> processData(CoordinateContainer);

    bool checkDisturbance(Pointf, bool&,Task * curr =NULL, float range=0.025);

    std::pair<bool, b2Vec2> buildWorld(b2World&,CoordinateContainer, b2Transform, Direction,  Disturbance disturbance=Disturbance());

    std::pair <Pointf, Pointf> bounds(Direction, b2Transform t, float boxLength, Task* curr=NULL); //returns bottom and top of bounding box

    cv::Rect2f getRect(std::vector <Pointf>);//gets bounding box of points

    b2Vec2 averagePoint(CoordinateContainer, Disturbance &, float rad = 0.025); //finds centroid of a poitn cluster, return position vec difference

    int getBodies(){
        return bodies;
    }

    void resetBodies(){
        bodies =0;
    }

    bool occluded(Disturbance);
};
#endif