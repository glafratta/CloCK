#pragma once
#include "configurator.h"
#include <chrono>
#include <iostream>

void Configurator::NewScan(std::vector <Point> & data){ 
	//PREPARE VECTORS TO RECEIVE DATA
	iteration++; //iteration set in getVelocity
	//std::vector <cv::Point2f> current;
	// std::vector <cv::Point2f> previous = current;
	std::vector <Point> previous;
	for (Point p:current){
		previous.push_back(p);
	}
	current.clear();
	for (Point p:data){
		current.push_back(p);
	}
	//current = data;
	//printf("current length: %i, previous length%i\n", current.size(), previous.size());

	//BENCHMARK + FIND TRUE SAMPLING RATE
	auto now =std::chrono::high_resolution_clock::now();
	std::chrono::duration<float, std::milli>diff= now - previousTimeScan; //in seconds
	timeElapsed=float(diff.count())/1000; //express in seconds
	//printf("time elapsed between newscans = %4f ms\n", timeElapsed);
	totalTime += timeElapsed; //for debugging
	previousTimeScan=now; //update the time of sampling

	//timeElapsed = samplingRate;
	//DISCARD BAD SCANS
	if ( timeElapsed< .15){
		//timeElapsed= samplingRate;
		//return;
	}
	else if (timeElapsed >.25){
		//return;
	}
	
	
	//LOAD LIDAR DATA

	// char filePath[256];
	// sprintf(filePath, "%s%s%04i.dat", folder, readMap,iteration);
	// //printf("%s\t", filePath);
	// std::ifstream file(filePath);

	// float x, y;
	// while (file>>x>>y){ 
	// 	if (sqrt(x*x+y*y)<1.5){
	// 		current.push_back(cv::Point2f(x, y));
	// 	}
		
	// }
	// file.close();


	//CREATE BOX2D ENVIRONMENT
	b2World world = b2World({0.0f,0.0f});
	char name[256];

	
	//CALCULATE VELOCITY 
	
	Configurator::getVelocityResult affineTransResult= GetRealVelocity(current, previous);
	b2Vec2 estimatedVelocity;

	if (affineTransResult.valid){
		estimatedVelocity = affineTransResult.vector;
	}
	else{
		estimatedVelocity = {0.0f, 0.0f};
		//printf("invalid affine trans result\n");
	}
	//printf("estimatedVelocity = %f, %f\n", estimatedVelocity.x, estimatedVelocity.y);

	//MAKE NOTE OF WHAT STATE WE'RE IN BEFORE RECHECKING FOR COLLISIONS
	bool wasAvoiding = 0;
	bool isSameState = 1;
	//UPDATE ABSOLUTE POSITION (SLAM-ISH for checking accuracy of velocity measurements)

	absPosition.x += estimatedVelocity.x*timeElapsed;
	absPosition.y += estimatedVelocity.y*timeElapsed;
	//dumpPath = fopen("/tmp/dumpPath.txt", "a+");
	//fprintf(dumpPath, "%f\t%f\n", absPosition.x, absPosition.y);
	//fclose(dumpPath);

	//IF WE  ALREADY ARE IN AN OBSTACLE-AVOIDING STATE, ROUGHLY ESTIMATE WHERE THE OBSTACLE IS NOW
	if (plan.size()>0 && plan[0].obstacle.isValid()){
		wasAvoiding =1; //remember that the robot was avoiding an obstacle
		//printf("old untracked obstacle position: %f\t%f\n", plan[0].obstacle.getPosition().x, plan[0].obstacle.getPosition().y);
		plan[0].trackObject(plan[0].obstacle, timeElapsed, estimatedVelocity, {0.0f, 0.0f}); //robot default position is 0,0
		if (plan[0].obstacle.getAngle(estimatedVelocity) >= M_PI_2){ 		//if obstacle (pos) and robot (vel) are perpendicular
			plan[0].obstacle.invalidate();
			plan.erase(plan.begin());						//no need to be in obstacle avoiding state
		//	printf("erased plan\n");
		}
		else{
			State::Object temp = plan[0].obstacle;			//otherwise update current state with new obstacle position
			plan[0]= State(temp);
		}

	}

	//MAKE BOX2D BODIES 

	bool isObstacleStillThere=0;
	//for (int i=0; i<current.size(); i++){ //makes obstacles and checks for duplicates
	for (Point &p:current){
		//if ((current[i].x != current[i-1].x || current[i].y!=current[i-1].y)&& current[i].x>=0 && current[i].r<1){ // robot only sees obstacles ahead of it //DEPRECATED
		//if (current[i]!=current[i-1]&& current[i].x>=0 && current[i].r<=1){
		if (p != *(&p-1)&& p.x >=0 && p.r <.5){
			b2Body * body;
			b2BodyDef bodyDef;
			b2FixtureDef fixtureDef;
			bodyDef.type = b2_dynamicBody;
			b2PolygonShape fixture; //giving the point the shape of a box
			fixtureDef.shape = &fixture;
			fixture.SetAsBox(.001f, .001f); 

		//CHECK IF BODIES ARE OBSERVED IN THE GENERAL AREA WHERE THE OBSTACLE SHOULD BE 
			if (plan.size()>0 && plan[0].obstacle.isValid()){
				//if (current[i].x > plan[0].obstacle.getPosition().x-0.05 && current[i].x < plan[0].obstacle.getPosition().x+0.05 && current[i].y > plan[0].obstacle.getPosition().y-0.05 && current[i].y < plan[0].obstacle.getPosition().y+0.05){
				if (p.isInSquare(plan[0].obstacle.getPosition())){
					isObstacleStillThere =1;
				}
			}
			//if (current[i].x == 0 && current[i].y ==0){
			
				//printf("obstacle at 0,0, skipping\n");
			//}
			//else{
			if (p.x !=0 && p.y!=0){
				// bodyDef.position.Set(current[i].x, current[i].y); 
				bodyDef.position.Set(p.x, p.y); 
				body = world.CreateBody(&bodyDef);
				body->CreateFixture(&fixtureDef);
			}

		}
	}
	//printf("bodies = %i\n", world.GetBodyCount());

	if (!isObstacleStillThere && plan.size()>0){
		plan.erase(plan.begin());
	}

	//current.clear(); //commented because now we're keeping previous in memory

	//CHECK IF WITH THE CURRENT STATE THE ROBOT WILL CRASH
	//printf("plan size %i\n", plan.size());
	isSameState = wasAvoiding == plan.size()>0;
	State * state;
	State::simResult result;
	if (plan.size() ==0){
		//printf("state is desired state\n");
		desiredState.setRecordedVelocity(estimatedVelocity);
		state = &desiredState;		 
		//printf("set state\n");
		result =desiredState.willCollide(world, iteration);
		//printf("done willcollide\n");
		//setNameBuffer(desiredState.planFile);
		// if (result.resultCode == State::simResult::crashed){
		// 	plan.push_back(State(result.collision)); //if state has one or more obstacles it is "avoid" state
		// 	printf("collided, new state has trajectory omega= %f,  linear speed: %f\n", plan[0].getAction().getOmega(), plan[0].getAction().getLinearSpeed() );
		}
	else if (plan.size()>0){ //the program enters this loop both if a disturbance has just been detected and if it was previously detected
		plan[0].setRecordedVelocity(estimatedVelocity);
			//check if the trajectory being followed will bump the robot into something
		result =plan[0].willCollide(world, iteration);
		//setNameBuffer(plan[0].planFile);
		state = & plan[0];
	}		
	if (result.resultCode == State::simResult::crashed){
		//printf("crashed\n");
		//CHECK IF THE OBSTACLE DETECTED IS THE SAME OBSTACLE THAT IS ALREADY BEING AVOIDED
		if (plan.size()>0){
			if (result.collision.getPosition().x > plan[0].obstacle.getPosition().x-0.05 && result.collision.getPosition().x < plan[0].obstacle.getPosition().x+0.05 && result.collision.getPosition().y > plan[0].obstacle.getPosition().y-0.05 && result.collision.getPosition().y < plan[0].obstacle.getPosition().y+0.05){
				printf("same obstacle, plan still executing\n");
			}
		}
		else{ 
			plan.push_back(State(result.collision)); //ADD TO THE QUEUE OF OBSTACLES TO AVOID
			printf("created new trajectory omega= %f, linear speed: %f\n", plan[0].getAction().getOmega(), plan[0].getAction().getLinearSpeed() );

		}			
	}
	else {//printf("not crashed\n");
	}
	//IF THE STATE DIDN'T CHANGE, CORRECT ANY INACCURACIES IN 
	//printf("was avoiding? %i\tis same state? %i\n", wasAvoiding, isSameState);
	if (isSameState){
		//printf("state is the same\n");
		if (state->controller()==State::controlResult::DONE){
			plan.erase(plan.begin());	
			printf("obstacle successfully avoided\n");					//no need to be in obstacle avoiding state
		}
	}
	else {
		//printf("state has changed\n");
	}

	printf("plan size (end of newscan) = %i, iteration %i\n", plan.size(), iteration);


	}


	


	

Configurator::getVelocityResult Configurator::GetRealVelocity(std::vector <Point> &_current, std::vector <Point> &_previous){	 //does not modify current vector, creates copy	
		getVelocityResult result;
		// std::vector <cv::Point2f> previousTmp;
		// FILE * currentSmooth;
		// char smoothName[250];
		// char prevPath[256];
		// sprintf(prevPath, "%s/%s%04d.dat", folder, readMap, iteration-1);
		// if (iteration >1){
		// 	std::ifstream prev(prevPath);
		// 	float x,y;
		// 	while (prev>>x>>y){
		// 		b2Vec2 v(x, y);
		// 		if (v.Length()<1.5){
		// 			previousTmp.push_back(cv::Point2f(x,y));
		// 		}
		// 	}
		// 	prev.close();
		// }


		int diff = _current.size()-_previous.size(); //if +ve,current is bigger, if -ve, previous is bigger
        //adjust for discrepancies in vector size		//int diff = currSize-prevSize;
		std::vector <cv::Point2f> currentTmp, previousTmp;
		//MAKE OPENCV VECTORS
		for (Point p: _current){
			currentTmp.push_back(cv::Point2f(p.x, p.y));
		}
		for (Point p: _previous){
			previousTmp.push_back(cv::Point2f(p.x, p.y));
		}

		if(diff>0){ //(current.size()>previous.size()){
			if (previousTmp.empty()){
				previousTmp = currentTmp;
				}
			else{
				for (int i=0; i<abs(diff); i++){
					previousTmp.push_back(previousTmp[0]); //before it was [-1]
				if (previousTmp[-1].x == 0 && previousTmp[-1].y ==0){
					printf("can't get previous data\n");
				}

			}
			}
		}
	
		else if (diff<0){//(current.size()<previous.size()){
			if (currentTmp.empty()){
				for (cv::Point2f p:previousTmp){
					//current.push_back(cv::Point2f());
					printf("no data\n");
					return result;
				} 
				//current = previousTmp;
				}
			else{
				for (int i=0; i<abs(diff); i++){
			currentTmp.push_back(currentTmp[0]);
				if (currentTmp[-1].x == 0 && currentTmp[-1].y ==0){
				}

				}
		}
		//printf("p>curr\n");
		}

	//use partial affine transformation to estimate displacement
	cv::Mat transformMatrix = cv::estimateAffinePartial2D(previousTmp, currentTmp, cv::noArray(), cv::LMEDS);
	float theta;
		State * state = getCurrentState();
		//printf("matrix is empty? %i\n", transformMatrix.empty());
		if (!transformMatrix.empty()){
			b2Vec2 tmp;
			tmp.x= -(transformMatrix.at<double>(0,2))/timeElapsed;
			tmp.y = -(transformMatrix.at<double>(1,2))/timeElapsed;
			//printf("velocity result %f %f\n", tmp.x, tmp.y);
			float tmpAngle = atan(tmp.y/tmp.x); //atan2 gives results between pi and -pi, atan gives pi/2 to -pi/2
			if (tmp.y ==0 && tmp.x ==0){
				tmpAngle =0;
			}
			float tmpPi = tmpAngle/M_PI;
			//dumpDeltaV = fopen("/tmp/deltaV.txt", "a");
			//fprintf(dumpDeltaV, "og velocity x= %f, y=%f, length = %f\n", tmp.x, tmp.y, tmp.Length()); //technically
			//fprintf(dumpDeltaV,"tmpAngle = %f pi\n", tmpPi);
			if (tmp.Length()>maxAbsSpeed){
				affineTransError += tmp.Length()-maxAbsSpeed;
				tmp.x = state->getAction().getLinearSpeed() *cos(tmpAngle);
				tmp.y = state->getAction().getLinearSpeed() *sin(tmpAngle);
				//printf("getting velocity from proprioception\n");
			//fprintf(dumpDeltaV, "changed velocity to x= %f, y=%f, length = %f\n", tmp.x, tmp.y, tmp.Length()); //technically

			}
			//fclose(dumpDeltaV);
			return getVelocityResult(tmp);
		}
		else if (transformMatrix.empty()){ //if the plan is empty look at the default wheel speed
			b2Vec2 estimatedVel;
			theta = state->getAction().getOmega()* timeElapsed;
			estimatedVel ={state->getAction().getLinearSpeed()*cos(theta),state->getAction().getLinearSpeed()*sin(theta)};
			result = getVelocityResult(estimatedVel);
			//printf("getting velocity from current state\n");
			return result;
		}
		else{
			printf("could not find velocity\n");
			return result;
		}
	}
	//}



// b2Vec2 Configurator::estimateDisplacementFromWheels(){ //actually estimates displacement
// 	//we 
//     b2Vec2 vel;
// 	float realL = maxAbsSpeed*leftWheelSpeed;
// 	float realR = maxAbsSpeed*rightWheelSpeed;
//     	//find angle theta in the pose:
// 	float theta = (realL-realR)*timeElapsed/desiredState.getAction().getDistanceWheels(); //rad/s, final angle at end of 1s
// 	//find absolute speed
// 	float V =(realL+realR)/2; //distance covered
// 	if (realR-realL == 0){
// 		vel.x = realL;
// 		vel.y = 0;
// 		}
// 	else {
// 		vel.x = (desiredState.getAction().getDistanceWheels()/2)* sin(desiredState.getAction().getDistanceWheels()*timeElapsed/(realR-realL));
// 		vel.y = -(desiredState.getAction().getDistanceWheels()/2)* cos(desiredState.getAction().getDistanceWheels()*timeElapsed/(realR-realL));

// 	}
// 	return vel;
// }

