#include "task.h"
#include "sensor.h"
#include "libcam2opencv.h"
#include "alphabot.h"
#include "Iir.h"
#include <string>

class MotorCallback :public AlphaBot::StepCallback { //every 100ms the callback updates the plan
    unsigned int m_step=0;
    char a='0';
    int n_l=0, n_r=0, n_s=0;
public:
Task t=Task(STOP);


MotorCallback(){}

void step( AlphaBot &motors){
    m_step--;
    if (m_step==0){
        setA();
    }
    motors.setRightWheelSpeed(t.getAction().getRWheelSpeed()); //temporary fix because motors on despacito are the wrong way around
    motors.setLeftWheelSpeed(t.getAction().getLWheelSpeed());
    printf("char =%c\n", a);
}

void setA(char _a='0'){
    a=_a;
  //  printf("size of dn=%i\n", sizeof(dumpname));
    //memset(dumpname, 0, sizeof(dumpname));
    //printf("cleared dumpname, size =%i\n", sizeof(dumpname));
    //dumpname =char[50];
    //dumpname.clear();
    if (a== 'l'){
        t=Task(LEFT);
        m_step=15;
        n_l++;
    }
    else if (a=='r'){
        t=Task(RIGHT);
        m_step=15;
        n_r++;
    }
    else if (a=='s'){
        t=Task(DEFAULT);
        m_step=100;
        n_s++;
    }
    else{
        t=Task(STOP);
    }
  // 
//     snprintf(dumpname, "%c_%i.txt", getID(), getCount());
//    dumpname =std::string(tmp);
//     FILE * dump=fopen(dumpname, "w+");
//     fclose(dump);
}

int getCount(){
    if (a=='s'){
        return n_s;
    }
    else if (a=='l'){
        return n_l;
    }
    else if (a=='r'){
        return n_r;
    }
    else if (a=='0'){
        return 0;
    }

}

char* getID(){
    return &a;
}
};

struct CameraCallback: Libcam2OpenCV::Callback {
    char dumpname[8];
    struct{
        int order=3;
        int DC=0; //HZ
        int cutoff_frequency=4; //HZ
        int band_width=0.5;
    }filter_parameters;
    Iir::Butterworth::Lowpass<filter_parameters.order>low_pass;
    Iir::Butterworth::BandStop<filter_parameters.order>band_stop;

    CameraCallback(MotorCallback * _cb):cb(_cb){
        low_pass.setup(FPS, filter_parameters.cutoff_frequency);
        band_stop.setup(FPS, filter_parameters.DC, filter_parameters.band_width);
    }


	void hasFrame(const cv::Mat &frame, const libcamera::ControlList &) {
		printf("has frame\n");
        b2Vec2 optic_flow=imgProc.avgOpticFlow(frame);
        b2Vec2 optic_flow_filtered=optic_flow;
        printf("optic flow = %f, %f\n", optic_flow.x, optic_flow.y);
        optic_flow_filtered.x=float(low_pass.filter(optic_flow.x));
        optic_flow_filtered.x=float(band_stop.filter(optic_flow_filtered.x));
		//cb->t.correct.update(optic_flow.x); //for now just going straight
        FILE * dump=fopen(dumpname, "a+");

        fprintf(dump, "%f\t%f\t%f\t%f\t%f\t%f\n", 
            optic_flow.x, optic_flow.y, optic_flow_filtered.x, optic_flow_filtered.y, 
                cb->t.correct.);
        fclose(dump);
    }
private:
ImgProc imgProc;
MotorCallback *cb=NULL;
};

int main(int argc, char** argv) {
    char a=0;
    if (argc>1){
        a=*argv[1];
    }
	AlphaBot motors;	
    MotorCallback cb;
    cb.setA(a);
    CameraCallback cameraCB(&cb);
    sprintf(cameraCB.dumpname, "avg%s_%i_iir.txt", cb.getID(), cb.getCount());
    FILE * dump=fopen(cameraCB.dumpname, "w+");
    fclose(dump);
    Libcam2OpenCV camera;
    camera.registerCallback(&cameraCB);
    Libcam2OpenCVSettings settings;
    settings.framerate = 30;
	motors.registerStepCallback(&cb);
	//motors.start();
    camera.start(settings);
	// do {
    //    // if (getchar()){
    //         //char a=getchar();
            
    //    // } 
	// } while(true);
    getchar();
	//motors.stop();
    camera.stop();

}
	
	
