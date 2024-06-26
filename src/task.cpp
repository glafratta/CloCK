#include "task.h"



simResult Task::willCollide(b2World & _world, int iteration, bool debugOn, float remaining, float simulationStep){ //CLOSED LOOP CONTROL, og return simreult
		simResult result=simResult(simResult::resultType::successful);
		Robot robot(&_world);
		Listener listener;
		_world.SetContactListener(&listener);	
		FILE * robotPath;
		if (debugOn){
			sprintf(planFile, "/tmp/robot%04i.txt", iteration);
			robotPath = fopen(planFile, "a");
		}
		float theta = start.q.GetAngle();
		b2Vec2 instVelocity = {0,0};
		b2Vec2 pointInObject;
		robot.body->SetTransform(start.p, theta);
		int step=0;
		for (step; step < (HZ*remaining); step++) {//3 second
			instVelocity.x = action.getLinearSpeed()*cos(theta);
			instVelocity.y = action.getLinearSpeed()*sin(theta);
			robot.body->SetLinearVelocity(instVelocity);
			robot.body->SetAngularVelocity(action.getOmega());
			robot.body->SetTransform(robot.body->GetPosition(), theta);
			if (debugOn){
				fprintf(robotPath, "%f\t%f\n", robot.body->GetPosition().x, robot.body->GetPosition().y); //save predictions/
			}
			if (checkEnded(robot.body->GetTransform()).ended || (start.p-robot.body->GetTransform().p).Length()>=simulationStep){
				break;
			}
			_world.Step(1.0f/HZ, POS_IT, VEL_IT); //time step 100 ms which also is alphabot callback time, possibly put it higher in the future if fast
			theta += action.getRecOmega()/HZ; //= omega *t
			if (listener.collisions.size()>0){ //
				int index = int(listener.collisions.size()/2);
				Disturbance collision = Disturbance(1, listener.collisions[index]);
				b2Vec2 distance = collision.getPosition()-robot.body->GetTransform().p;
				result = simResult(simResult::resultType::crashed, collision);
				robot.body->SetTransform(start.p, start.q.GetAngle()); //if the simulation crashes reset position for 
				collision.setOrientation(robot.body->GetTransform().q.GetAngle());
				break;
			}
		}
		result.collision.setAngle(robot.body->GetTransform());	
		b2Vec2 distance; 
		distance.x = robot.body->GetPosition().x - start.p.x;
		distance.y = robot.body->GetPosition().y - start.p.y;
		result.distanceCovered = distance.Length() ;
		result.endPose = robot.body->GetTransform();
		result.step=step;
		for (b2Body * b = _world.GetBodyList(); b!=NULL; b = b->GetNext()){
			_world.DestroyBody(b);
		}
		if (debugOn){
			fclose(robotPath);
		}
		int bodies = _world.GetBodyCount();
		return result;
	
}


void Task::trackDisturbance(Disturbance & d, float timeElapsed, b2Transform robVelocity, b2Transform pose){ //isInternal refers to whether the tracking is with respect to the global coordinate frame (i.e. in willCollide) if =1, if isIntenal =0 it means that the Disturbance is tracked with the robot in the default position (0.0)
	b2Transform shift(b2Vec2(-robVelocity.p.x*timeElapsed, -robVelocity.p.y*timeElapsed), b2Rot(-robVelocity.q.GetAngle()*timeElapsed)); //calculates shift in the time step
	b2Vec2 newPos(d.getPosition().x+shift.p.x,d.getPosition().y + shift.p.y);
	d.setPosition(newPos);
	float angle = d.getAngle(pose);
	if (d.isPartOfObject()){
		d.setOrientation(d.getOrientation() + shift.q.GetAngle());
	}
	d.setAngle(angle); //with respect to robot's velocity
}

void Task::trackDisturbance(Disturbance & d, Action a){
	float angleTurned =MOTOR_CALLBACK*a.getOmega();
	d.pose.q.Set(d.pose.q.GetAngle()-angleTurned);	
	float distanceTraversed = MOTOR_CALLBACK*a.getLinearSpeed();
	float initialL = d.pose.p.Length();
	d.pose.p.x=cos(d.pose.q.GetAngle())*initialL-cos(angleTurned)*distanceTraversed;
	d.pose.p.y = sin(d.pose.q.GetAngle())*initialL-sin(angleTurned)*distanceTraversed;
}

void Task::controller(float timeElapsed, int s){
	float tolerance = 0.01; //tolerance in radians/pi = just under 2 degrees degrees
	float timeStepError =action.getRecOmega()*timeElapsed; 
	float normAccErr = timeStepError/SAFE_ANGLE;
	if (action.getOmega()!=0|| s<1 || s%10!=0){ //only check every 2 sec
		return;
	}
	if (fabs(timeStepError)>tolerance){
		printf("error non norm = %f, error norm= %f\n",timeStepError, normAccErr);
		if (normAccErr<0){
			action.L -= normAccErr*pGain;  
		}
		else if (normAccErr>0){
			action.R -= normAccErr *pGain; 
		}
		if (action.L>1.0){
		action.L=1.0;
		}
		if (action.R>1.0){
			action.R=1;
		}
		if (action.L<(-1.0)){
			action.L=-1;
		}
		if (action.R<(-1.0)){
			action.R=-1;
		}
	}

}

Direction Task::H(Disturbance ob, Direction d, bool topDown){
	if (ob.isValid()){
        if (ob.getAffIndex()==int(InnateAffordances::AVOID)){ //REACTIVE BEHAVIOUR
            if (d == Direction::DEFAULT){ //REACTIVE BEHAVIOUR
                if (ob.getAngle(start)<0){//angle formed with robot at last safe pose
                    d= Direction::LEFT; //go left
                }
                else if (ob.getAngle(start)>0){ //angle formed with robot at last safe pose
                    d= Direction::RIGHT; //
                }   
                else{
                    int c = rand() % 2;
                    d = static_cast<Direction>(c);

                }
            }
        }
		else if (ob.getAffIndex()==int(InnateAffordances::PURSUE)){
			if (d == Direction::DEFAULT & !topDown){ //REACTIVE BEHAVIOUR
                if (ob.getAngle(start)<-.1){//angle formed with robot at last safe pose
                    d= Direction::RIGHT; //go left
                }
                else if (ob.getAngle(start)>0.1){ //angle formed with robot at last safe pose, around .1 rad tolerance
                    d= Direction::LEFT; //
                }   
            }
		}
}
    return d;
}

void Task::setEndCriteria(){
	Distance d;
	Angle a;
	b2Vec2 v;
	switch(disturbance.getAffIndex()){
		case AVOID: {
				endCriteria.distance.setValid(1);
			if (disturbance.isPartOfObject()){
				endCriteria.angle.setValid(1);
				}
			else{
				endCriteria.angle=Angle(SAFE_ANGLE);
			}
		}
		break;
		case PURSUE:{
			endCriteria.distance = Distance(0+DISTANCE_ERROR_TOLERANCE);
		}
		break;
		default:
		break;
	}
	
}

EndedResult Task::checkEnded(b2Transform robotTransform){ //self-ended
	EndedResult r;
	Angle a;
	Distance d;
	if (disturbance.isValid()){
		b2Vec2 v = disturbance.getPosition() - robotTransform.p; //distance between disturbance and robot
		d= Distance(v.Length());
		if (action.getOmega()!=0){
			float angleL = start.q.GetAngle()+M_PI_2;
			float angleR = start.q.GetAngle()-M_PI_2;
			r.ended = robotTransform.q.GetAngle()>=angleL || robotTransform.q.GetAngle()<=angleR;
		}
		else if (getAffIndex()== int(InnateAffordances::NONE)){
			r.ended = true;
		}
		else if (getAffIndex()==int(InnateAffordances::PURSUE)){
			a = Angle(disturbance.getAngle(robotTransform));
			r.ended = d<=endCriteria.distance; 
		}
	}
	else{
		float angleL = start.q.GetAngle()+M_PI_2;
		float angleR = start.q.GetAngle()-M_PI_2;
		r.ended = robotTransform.q.GetAngle()>=angleL || robotTransform.q.GetAngle()<=angleR;
	}
	if (round(robotTransform.p.Length()*100)/100>=BOX2DRANGE){ //if length reached or turn
		r.ended =true;
	}
	r.errorFloat = endCriteria.getStandardError(a,d);
	return r;

}

EndedResult Task::checkEnded(Node n){ //check error of node compared to the present Task
	EndedResult r;
	Angle a;
	Distance d;
	r = checkEnded(n.endPose);
	r.errorFloat+= endCriteria.getStandardError(a,d, n);
	return r;
}

float Task::findOrientation(b2Vec2 v1, b2Vec2 v2){
	float slope = (v2.y- v1.y)/(v2.x - v1.x);
	return atan(slope);
}

