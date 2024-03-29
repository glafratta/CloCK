#ifndef GENERAL_H
#define GENERAL_H

#include <vector>
#include <map>
#include <set>
#include <utility>                   // for std::pair
#include <algorithm>                 // for std::for_each
#include <boost/graph/graph_traits.hpp>
#include <boost/graph/adjacency_list.hpp>
#include </usr/include/boost/container/map.hpp>
#include <cmath>
#include "disturbance.h"


struct Edge{
	Direction direction;
};

struct Node{
	Disturbance disturbance; //error signal
	b2Transform endPose = b2Transform(b2Vec2(0.0, 0.0), b2Rot(0)); 
	simResult::resultType outcome;
	std::vector <Direction> options;
	int nodesInSameSpot =0;
	int totDs=0; //error signal
	bool filled =0;
	float cost=0; //self-error
	float error=0; //error with respect to control goal
	int step=0;
	
	void fill(simResult);

	float evaluationFunction(float weight=0.02);
};

struct TaskSummary{
	Disturbance disturbance;
	Direction direction = DEFAULT;
	int step=0;

	TaskSummary()=default;

	TaskSummary(Disturbance d, Direction dir, float s): disturbance(d), direction(dir), step(s){	}
};

typedef b2Transform Transform;
bool operator!=(Transform const &, Transform const &);
bool operator==(Transform const &, Transform const &);

typedef boost::adjacency_list<boost::vecS, boost::vecS, boost::bidirectionalS, Node, Edge> CollisionGraph;
typedef boost::graph_traits<CollisionGraph>::vertex_iterator vertexIterator; 
typedef boost::graph_traits<CollisionGraph>::vertex_descriptor vertexDescriptor;
typedef boost::graph_traits<CollisionGraph>::edge_descriptor edgeDescriptor;
typedef boost::graph_traits<CollisionGraph>::edge_iterator edgeIterator;
typedef std::vector <TaskSummary> Sequence;


struct Point{
	private:
	bool valid=0;
	public:
	float x=0;
	float y=0;
	float r=0;
	float phi=0;

	Point(){}

	Point(float _x, float _y): x(_x), y(_y){
		r= sqrt(x*x+y*y);
		phi = atan2(y,x);;
	}

	Point(b2Vec2 v): x(v.x), y(v.y){
		r= sqrt(x*x+y*y);
		phi = atan2(y,x);
	}

	Point(float _x, float _y, float _r, float _phi): x(_x), y(_y), r(_r), phi(_phi){
	}

	void operator=(const Point &p){
		x = p.x;
		y= p.y;
		r= p.r;
		phi = p.phi;
	}

	bool operator==(Point &p){
		return (x == p.x && y == p.y);
	}

	bool operator!=(Point &p){
		if (*this == p){
			return false;
		}
		else if (!(*this ==p)){
			return true;
		}
	}

	Point operator+(const Point &p){
		Point result;
		result.x = x + p.x;
		result.y = y+ p.y;
		result.phi = phi +p.phi;
		result.r = r+p.r;
		return result;
	}


	bool isInRadius(b2Vec2 center, float radius = 0.05){ //check if this point is within a certain radius from another given point (center)
		bool result = false;
		if (center.IsValid() & this->getb2Vec2().IsValid()){
			std::pair <float, float> xBounds(center.x+radius, center.x-radius);
			std::pair <float, float> yBounds(center.y+radius, center.y-radius);		
			float xLow = std::min(xBounds.first, xBounds.second);
			float xHigh = std::max(xBounds.first, xBounds.second);
			float yLow = std::min(yBounds.first, yBounds.second);
			float yHigh = std::max(yBounds.first, yBounds.second);
			result = this->x <= xHigh && this->x >=xLow-radius && this->y <= yHigh && this->y >=yLow;
		}
		return result;
	}

	b2Vec2 getb2Vec2(){
		return b2Vec2(x,y);
	}

	void polarInit(float radius, float angle){
		r= radius;
		phi = angle;
		x = radius *cos(angle);
		y = radius *sin(angle);
	}


};

struct comparator{
    bool operator() ( Point a, Point b ){ //
        return a.y <=b.y;
	}
}; 

typedef Point P;
bool operator<(P const &, P const &);

bool operator>(P const &, P const &);

bool operator==(P const &, P const &);

typedef std::set<Point> CoordinateContainer;

typedef std::pair <bool, int> Quadrant; //a .2x.2 box used to check whether general area has been visited

#endif