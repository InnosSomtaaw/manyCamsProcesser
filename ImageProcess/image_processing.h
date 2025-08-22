#ifndef IMAGE_PROCESSING_CLASS_H
#define IMAGE_PROCESSING_CLASS_H

#include <QObject>
#include <QMutex>
#include <QQueue>
#include <QFileDialog>
#include <QImage>
#include <qdatetime.h>
#include <QRunnable>
#include <QThread>
#include <QThreadPool>
#include <QElapsedTimer>
#include <QImageReader>

#include <opencv2/opencv.hpp>
#include <opencv2/core/ocl.hpp>

QT_BEGIN_NAMESPACE
using namespace cv;
using namespace std;
QT_END_NAMESPACE
enum WorkConditionsEnum
{
    ObjDet = 0,
    SemSeg,
    Anomaly
};

class ImageWriter : public QRunnable
{
public:
    //计时器
    QElapsedTimer usrtimer;
    QImage qimg;
    QString headname;
    QMutex iwtMutex;
    //保存方式：0-png;1-bmp;2-jpg;
    int method=0;
    void run() override;
    //自动删除30天以前保存图片文件夹
    void autoDeleteFolder(int numBk=30);
};

//图像处理类
class Image_Processing_Class : public QObject, public QRunnable
{
    Q_OBJECT
public:
    Image_Processing_Class();
    ~Image_Processing_Class();

    Mat img_input1,img_output1,img_input2,img_output2,img_output3;
    vector<Mat> img_inputs, img_outputs;
    vector<bool> inputFlags;
    bool hasInited,cam1Refreshed,cam2Refreshed,onGPU;
    int save_count,max_save_count,onceRunTime;
    bool mainwindowIsNext,mainwindowIsStopProcess,isSavingImage;

    WorkConditionsEnum workCond;
    QMutex ipcMutex;
    atomic<bool> isDetecting,busyProcessing;
    int procerIdx=-1;

    //选取文件夹内的所有图片地址
    QList<QFileInfo> *img_filenames;

    QQueue<Point> *basePTs;
    QQueue<bool> *islifting;
    vector<vector<Point> > ptsVec;

    //计时器
    QElapsedTimer usrtimer;

    //处理图片组
    virtual bool processFilePic();
    //初始化输入输出图像
    void initImgInOut();
    //重置参数
    void resetPar();

protected:
    void run() override;

signals:
    //刷新主窗体显示后图片信号
    void outputImgProcessedRequest(int);
    //刷新主窗体多画面显示信号
    void outputMulImgsRequest();
    //请求主窗体按钮状态信号
    void mainwindowStatusRequest(int);

public slots:
    //开始图片组处理槽
    void startPicsProcess();
    //开始单相机连续处理槽
    void start1CamProcess(QImage receivedImg);
    //开始多相机连续处理槽
    void startMulCamProcess(QImage recvImg, int i);
    //修改参数槽
    void changeProcPara(QString qstr,int wc);
    //
    virtual void iniImgProcessor();
    //开始单次处理槽
    virtual void startProcessOnce();

private:
    void resizeCompare();

};

#endif // IMAGE_PROCESSING_CLASS_H
