#include "integrateddisplay.h"

IntegratedDisplay::IntegratedDisplay()
{
    displayTimer=new QTimer();
    disItv=40;
    shouldSave=false;
}

void IntegratedDisplay::iniImgProcessor()
{
    displayTimer->setInterval(disItv);
    connect(displayTimer, &QTimer::timeout, this,
            &IntegratedDisplay::slotledTimeOut);

    iwts.resize(img_inputs.size());
    for(int i=0;i<iwts.size();i++)
    {
        iwts[i]=new ImageWriter;
        //保存方式：0-png;1-bmp;2-jpg;
        iwts[i]->method=2;
        iwts[i]->setAutoDelete(false);
    }
}

void IntegratedDisplay::slotGetOneFrameN(QImage img, int camidx)
{
    if(camidx<img_inputs.size())
        img_inputs[camidx]=Mat(img.height(), img.width(), CV_8UC3,
                               img.bits(), img.bytesPerLine()).clone();
}

void IntegratedDisplay::slotGetOneResultN(int residx)
{
    QObject* ptrSender=sender();

    Image_Processing_Class* ipcSender=(Image_Processing_Class*)ptrSender;
    if(residx<img_inputs.size())
        img_outputs[residx]=ipcSender->img_output3;
}

void IntegratedDisplay::slotledTimeOut()
{
//    cout<<"Current Timer thread: "<<QThread::currentThreadId()<<endl;
    emit outputMulImgsRequest();

    if(!shouldSave.load())
        return;

    for(int i=0;i<iwts.size();i++)
        if(iwts[i]->iwtMutex.tryLock())
        {
            iwts[i]->headname="_ch"+QString::number(i)+"_";
            QImage saveQimg;
            if(isDetecting.load())
                saveQimg = QImage(img_outputs[i].data,
                                  img_outputs[i].cols,img_outputs[i].rows,
                                  img_outputs[i].cols*img_outputs[i].channels(),
                                  QImage::Format_BGR888);
            else
                saveQimg = QImage(img_inputs[i].data,
                                  img_inputs[i].cols,img_inputs[i].rows,
                                  img_inputs[i].cols*img_inputs[i].channels(),
                                  QImage::Format_RGB888);
            iwts[i]->qimg=saveQimg;
            QThreadPool::globalInstance()->start(iwts[i]);
            iwts[i]->iwtMutex.unlock();
        }
}
