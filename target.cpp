#include "custom.h"

void forget(Configurator *c){}

Disturbance set_target(int run, b2Transform start){
	Disturbance result=Disturbance(PURSUE, start.p, start.q.GetAngle());
	return result;
}

int main(int argc, char** argv) {
	A1Lidar lidar;
	AlphaBot motors;
	Disturbance target(2, b2Vec2(BOX2DRANGE, 0));
    Task controlGoal(target, DEFAULT);
	ConfiguratorInterface configuratorInterface;
    Configurator configurator(controlGoal);
	configurator.planning =1;
	if (argc>1){
		configurator.debugOn= atoi(argv[1]);
		configuratorInterface.debugOn = atoi(argv[1]);
	}
	if (argc>2){
		configurator.simulationStep = atof(argv[2]);
	}
	LidarInterface dataInterface(&configuratorInterface);
	configurator.registerInterface(&configuratorInterface);
	MotorCallback cb(&configurator);
	lidar.registerInterface(&dataInterface);
	CameraCallback cameraCB(&cb);
	Libcam2OpenCV camera;
    camera.registerCallback(&cameraCB);
    Libcam2OpenCVSettings settings;
    settings.framerate = FPS;
	motors.registerStepCallback(&cb);
	configurator.start();
	lidar.start();
    camera.start(settings);
	motors.start();
	getchar();
	configurator.stop();
	motors.stop();
	lidar.stop();
	camera.stop();
}
	
	
