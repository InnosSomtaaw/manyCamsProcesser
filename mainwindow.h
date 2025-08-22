#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileDialog>
#include <QtWidgets>
#include <QtXml/QDomDocument>
#include <QTextStream>

#include "Camera/videoplayer.h"
#include "Camera/hwdecoder.h"
#include "ImageProcess/image_processing.h"
#include "ImageProcess/trtinferencer.h"
#include "ImageProcess/integrateddisplay.h"
#include "pictureview.h"

#define MAX_CAMS_NUM 270
#define MAX_THREADS_NUM 24

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    bool isDetecting,autoStart;
    QString IpRemote;
    int portLocal,portRemote;

private:
        //显示窗体初始化
        void initImageBoxes();
        //初始化程序工况
        void initWorkCondition();
        //文本输出初始化
        void initTextBrowsers();
        //初始化参数设置
        void initBaseSettings();
        //图像处理类初始化
        void initImageProcess();
        //初始化网络相机设置
        void initCamSettings();
        //数字相机类初始化
        void initVideoPlayers();

        Ui::MainWindow *ui;
        std::array<Mat,9> img_inputs,img_outputs;

        //网络相机参数
        std::vector<QString> urls;
        //AI工程相关文件路径
        QString proj_path;
        //定时显示间隔
        int displayInterval;

        WorkConditionsEnum workCond;
        QSettings *iniRW;

        QElapsedTimer timer;
        int froceLag=0;
//        //数字相机类（ffmpeg读取）
//        std::array<VideoPlayer*,4> cams;
        //数字相机类（cuda读取）
        std::vector<hwDecoder*> cams;
        //数字相机线程
        std::vector<QThread*> camProThds;

        //图像处理类
        std::vector<Image_Processing_Class*> imgProcessors;
        std::vector<trtInferencer*> imgTrts;
        //图像处理线程
        std::vector<QThread*> imgProThds;

        //集成显示类
        IntegratedDisplay* timedDisplayer;
        //集成显示线程
        QThread* displayThd;

        std::array<QGraphicsScene,9> scenes;
        std::array<QGraphicsPixmapItem,9> pixmapShows;
        std::array<QString,9> strOutputs;

        QList<PictureView*> imgboxes;
        QList<QTextBrowser*> txtbrowsers;
        QList<QFileInfo> *img_filenames;

private slots:
        //窗体打开图片
        void on_imageboxOpenImage();
        //窗体保存图片
        void on_imageboxSaveImage();
        //选择AI工程和节点按钮槽
        void on_buttonOpenAIProject_clicked();
        //重置按钮槽
        void on_buttonReset_clicked();
        //实时处理按钮槽
        void on_buttonProcess_clicked();
        //开始连续采集按钮槽
        void on_buttonStartCapture_clicked();
        //打开图片序列按钮槽
        void on_buttonOpenImgList_clicked();

        //视频流N获取槽
        void slotGetOneFrameN();

        void on_checkBoxSaving_toggled(bool checked);

signals:
        //开始数字相机信号
        void startCamNRequest();
        //开始单次处理信号
        void procCam1Request();
        void procCam2Request();
        void procCam3Request();
        void procCam4Request();

};
#endif // MAINWINDOW_H
