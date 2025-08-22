#ifndef INTEGRATEDDISPLAY_H
#define INTEGRATEDDISPLAY_H

#include <QTimer>

#include "ImageProcess/image_processing.h"

class IntegratedDisplay : public Image_Processing_Class
{
    Q_OBJECT
public:
    IntegratedDisplay();

    QTimer* displayTimer;
    int disItv;
    atomic<bool> shouldSave;

    //图像保存类
    std::vector<ImageWriter*> iwts;

public slots:
    //视频流N获取槽
    void slotGetOneFrameN(QImage img,int camidx);
    //处理结果N获取槽
    void slotGetOneResultN(int residx);
    //定时显示槽
    void slotledTimeOut();

    void iniImgProcessor() override;
};

#endif // INTEGRATEDDISPLAY_H
