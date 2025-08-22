#ifndef HIKCAMERA_H
#define HIKCAMERA_H

#include "general_camera.h"
#include <HCNetSDK.h>
#include <array>

class hikCamera : public General_Camera
{
    Q_OBJECT
public:
    hikCamera();

    std::array<QString,5> hikLogin;
    void startCamera() override;
    void getOneFrame() override;

private:
    LONG lUserID;
    LONG lRealPlayHandle;

protected:
    void run() override;
};

#endif // HIKCAMERA_H
