#include "general_camera.h"

General_Camera::General_Camera()
{
    isCapturing=0;
    hasFinished=false;
    hasStarted=false;
    froceLag=0;
    camidx=-1;
}

General_Camera::~General_Camera()
{
    isCapturing=0;
    hasFinished=true;
}

void General_Camera::startCamera()
{
    hasStarted=true;
}

void General_Camera::getOneFrame()
{
    isCapturing=1;
}
