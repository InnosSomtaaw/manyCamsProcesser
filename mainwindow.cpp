#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    //显示窗体初始化
    initImageBoxes();
    //初始化程序工况
    initWorkCondition();
    //文本输出初始化
    initTextBrowsers();
    //初始化参数设置
    initBaseSettings();
    //初始化网络相机设置
    initCamSettings();
    //图像处理类初始化
    initImageProcess();
    //数字相机类初始化
    initVideoPlayers();
}

MainWindow::~MainWindow()
{
    for(int i=0;i<cams.size();i++)
        cams[i]->isCapturing=0;

    delete ui;
}

void MainWindow::initImageBoxes()
{
    imgboxes.append(ui->centralwidget->findChildren<PictureView*>());
    for(int i=0;i<imgboxes.size();i++)
    {
        imgboxes[i]->setScene(&scenes[i]);
        scenes[i].addItem(&pixmapShows[i]);
        connect(imgboxes[i],&PictureView::loadImgRequest,
                this,&MainWindow::on_imageboxOpenImage);
        connect(imgboxes[i],&PictureView::saveImgRequest,
                this,&MainWindow::on_imageboxSaveImage);
    }
}

void MainWindow::initWorkCondition()
{
    iniRW = new QSettings("LastSettings.ini",QSettings::IniFormat);
    iniRW->setIniCodec("UTF-8");

    workCond=WorkConditionsEnum(iniRW->value("WorkCondition/WorkCondition").toInt());
    froceLag=iniRW->value("CameraSettings/FroceLag").toInt();
    proj_path=iniRW->value("TensorRTInference/ModelPath").toString();
    IpRemote=iniRW->value("Communication/IpRemote").toString();
    portLocal=iniRW->value("Communication/LocalPort").toInt();
    portRemote=iniRW->value("Communication/RemotePort").toInt();
    autoStart=iniRW->value("WorkCondition/autoStart").toBool();
    displayInterval=iniRW->value("WorkCondition/displayInterval").toInt();

    for(int i=1;i<MAX_CAMS_NUM+1;i++)
    {
        QString urlName,urlVal;
        if(i<10)
            urlName="Urls/url00"+QString::number(i);
        else if(i>9 && i<100)
            urlName="Urls/url0"+QString::number(i);
        else
            urlName="Urls/url"+QString::number(i);
        urlVal=iniRW->value(urlName).toString();
        if(urlVal=="")
            break;
        urls.push_back(urlVal);
    }

//    for(int i=1;i<MAX_CAMS_NUM+1;i++)
//    {
//        QString urlName;
//        if(i<10)
//            urlName="Urls/url00"+QString::number(i);
//        else if(i>9 && i<100)
//            urlName="Urls/url0"+QString::number(i);
//        else
//            urlName="Urls/url"+QString::number(i);
//        QString val="";
//        iniRW->setValue(urlName,val);
//    }

}

void MainWindow::initTextBrowsers()
{
    txtbrowsers=findChildren<QTextBrowser*>();

    for(int i=0;i<txtbrowsers.size();i++)
    {
        txtbrowsers[i]->setStyleSheet(
                    "background:transparent;border-width:0;border-style:outset");
        strOutputs[i]="place holder "+QString::number(i)+" ...\n";
        txtbrowsers[i]->setText(strOutputs[i]);
    }
}

void MainWindow::initBaseSettings()
{
    isDetecting=false;
    QThreadPool::globalInstance()->setMaxThreadCount(MAX_THREADS_NUM);
}

void MainWindow::initCamSettings()
{
    timedDisplayer=new IntegratedDisplay;
    timedDisplayer->img_inputs.resize(urls.size());
    timedDisplayer->img_outputs.resize(urls.size());
    timedDisplayer->disItv=displayInterval;
    timedDisplayer->iniImgProcessor();
    displayThd=new QThread;
    timedDisplayer->moveToThread(displayThd);
    displayThd->start();
    connect(timedDisplayer,&IntegratedDisplay::outputMulImgsRequest,
            this,&MainWindow::slotGetOneFrameN);
}

void MainWindow::initImageProcess()
{
    //trtInferencer类初始化
    imgProcessors.resize(urls.size());
    imgTrts.resize(urls.size());
    imgProThds.resize(urls.size());
    for(int i=0;i<imgTrts.size();i++)
    {
        imgTrts[i]=new trtInferencer;

        imgTrts[i]->prog_file=proj_path;
        imgTrts[i]->workCond=workCond;
        imgTrts[i]->procerIdx=i;
        imgTrts[i]->iniImgProcessor();

        imgProThds[i]=new QThread;
        imgTrts[i]->moveToThread(imgProThds[i]);
        imgProThds[i]->start();

        imgProcessors[i]=imgTrts[i];
        imgProcessors[i]->ipcMutex.unlock();

        if(imgTrts[i]->hasInited)
            ui->buttonOpenAIProject->setEnabled(false);

        connect(imgTrts[i],&trtInferencer::outputImgProcessedRequest,
                timedDisplayer,&IntegratedDisplay::slotGetOneResultN);
    }
}

void MainWindow::initVideoPlayers()
{
    cams.resize(urls.size());
    camProThds.resize(urls.size());
    for(int i=0;i<cams.size();i++)
    {
        cams[i]=new hwDecoder;
        cams[i]->camURL=urls[i];
        cams[i]->froceLag=froceLag;
        if(urls[i].contains("mp4"))
            cams[i]->froceLag+=40;
        cams[i]->camidx=i;
        camProThds[i]=new QThread;
        cams[i]->moveToThread(camProThds[i]);
        camProThds[i]->start();

        connect(this,&MainWindow::startCamNRequest,
                cams[i],&hwDecoder::startCamera);
        connect(cams[i],&hwDecoder::sigProcessOnce,
                imgTrts[i],&trtInferencer::proQImgOnce);
        connect(cams[i],&hwDecoder::sigGetOneFrame,
                timedDisplayer,&IntegratedDisplay::slotGetOneFrameN);
    }
}


void MainWindow::on_buttonOpenAIProject_clicked()
{
    proj_path=QFileDialog::getOpenFileName(this,tr("Open AI Model: "),"./model/",
                                             tr("Model File(*.pdmodel *.onnx)"));
    ui->buttonReset->click();
    iniRW = new QSettings("LastSettings.ini",QSettings::IniFormat);
    iniRW->setValue("TensorRTInference/ModelPath",proj_path);
}

void MainWindow::on_imageboxOpenImage()
{
    QString fileName = QFileDialog::getOpenFileName(
                this,tr("Open Image"),".",tr("Image File(*.png *.jpg *.jpeg *.bmp)"));

    if (fileName.isEmpty())
        return;

    QImageReader qir(fileName);
    QImage img=qir.read();

    img=img.convertToFormat(QImage::Format_RGB888);
    Mat srcImage(img.height(), img.width(), CV_8UC3,img.bits(), img.bytesPerLine());

    QObject* ptrSender=sender();
    PictureView* pvSdr=(PictureView*)ptrSender;
    int sdrId=0;
    for(int i=0;i<imgboxes.size();i++)
        if(imgboxes[i]==pvSdr)
            sdrId=i;
    cvtColor(srcImage,img_inputs[sdrId],COLOR_BGR2RGB);
    //窗体N显示
    pixmapShows[sdrId].setPixmap(QPixmap::fromImage(img));
    imgboxes[sdrId]->Adapte();

    srcImage.release();
}

void MainWindow::on_imageboxSaveImage()
{
    ImageWriter *iw=new ImageWriter;
    //保存方式：0-png;1-bmp;2-jpg;
    iw->method=2;
    iw->headname="_mannul_";

    QObject* ptrSender=sender();
    PictureView* pvSdr=(PictureView*)ptrSender;
    int sdrId=0;
    for(int i=0;i<imgboxes.size();i++)
        if(imgboxes[i]==pvSdr)
            sdrId=i;
    iw->qimg=pixmapShows[sdrId].pixmap().toImage();

    QThreadPool::globalInstance()->start(iw);
}

void MainWindow::on_buttonProcess_clicked()
{
    if(imgProcessors[0]->img_filenames->size()>0)
        return;
    for(int i=0;i<min(imgboxes.size(),(int)urls.size());i++)
    {
//        cout<<i<<":"<<cams[i]->isCapturing.load()<<endl;
        if(cams[i]->isCapturing.load()>0)
            continue;
        QImage qimg=pixmapShows[i].pixmap().toImage();
        qimg.convertTo(QImage::Format_RGB888);
        if(!qimg.isNull())
        {
            imgTrts[i]->img_input1=Mat(qimg.height(), qimg.width(), CV_8UC3,
                           qimg.bits(), qimg.bytesPerLine());
            imgTrts[i]->yuvInput.store(false);
            imgTrts[i]->run();
            qimg = QImage(imgTrts[i]->img_output3.data,
                          imgTrts[i]->img_output3.cols,imgTrts[i]->img_output3.rows,
                          imgTrts[i]->img_output3.cols*imgTrts[i]->img_output3.channels(),
                          QImage::Format_BGR888);
            strOutputs[i]="No#"+QString::number(i+1)+" camera process consumed: "
                    +QString::number(imgProcessors[i]->onceRunTime)+"ms.\n";
            pixmapShows[i].setPixmap(QPixmap::fromImage(qimg));
            txtbrowsers[i]->setText(strOutputs[i]);
            strOutputs[i].clear();
            if(i==min(imgboxes.size(),(int)urls.size())-1)
                return;
        }
        else
            return;
    }

    if(ui->buttonProcess->text()=="Process" /*&& isDetecting==false*/)
    {
        isDetecting=true;
        ui->buttonProcess->setText("StopProcess");
        for(int i=0;i<imgProcessors.size();i++)
        {
            cams[i]->isCapturing=2;
            imgProcessors[i]->isDetecting.store(isDetecting);
//            imgProcessors[i]->ipcMutex.unlock();
        }
    }
    else if (ui->buttonProcess->text() == "StopProcess" /*&& isDetecting == true*/)
    {
        isDetecting=false;
        ui->buttonProcess->setText("Process");
        for(int i=0;i<imgProcessors.size();i++)
        {
            cams[i]->isCapturing=1;
            imgProcessors[i]->isDetecting.store(isDetecting);
//            imgProcessors[i]->ipcMutex.unlock();
        }
    }
    timedDisplayer->isDetecting.store(isDetecting);
}

void MainWindow::on_buttonReset_clicked()
{
//    ui->buttonOpenImageList->setText("OpenImageList");
    printf("reset clicked!");
    ui->buttonOpenImgList->setText("OpenImgList");
    isDetecting = false;
    ui->buttonProcess->setText("Process");
    ui->buttonStartCapture->setText("StartCapture");
    for(int i=0;i<imgProcessors.size();i++)
    {
        cams.at(i)->isCapturing=0;
        imgProcessors[i]->resetPar();
    }
    ui->buttonOpenAIProject->setEnabled(true);
}

void MainWindow::on_buttonStartCapture_clicked()
{
    if(ui->buttonStartCapture->text()=="StartCapture")
    {
        ui->buttonStartCapture->setText("StopCapture");

        for(int i=0;i<cams.size();i++)
            cams[i]->isCapturing=1;

        if(!cams[0]->hasStarted)
            emit startCamNRequest();
        else
            for(int i=0;i<cams.size();i++)
                QThreadPool::globalInstance()->start(cams[i]);
        for(int i=0;i<cams.size();i++)
            camProThds[i]->quit();

        timedDisplayer->displayTimer->start();
    }
    else if(ui->buttonStartCapture->text()=="StopCapture")
    {
        for(int i=0;i<cams.size();i++)
            cams[i]->isCapturing=0;
        ui->buttonStartCapture->setText("StartCapture");
        timedDisplayer->displayTimer->stop();
    }
}

void MainWindow::slotGetOneFrameN()
{
    long gap2recv=timer.elapsed();
    timer.start();

    for(int i=0;i<imgboxes.size();i++)
    {
        //窗体N显示
        QImage disImage;
        QString tempStr;
        int disGroup=ui->condComboBox->currentIndex();
        int j=disGroup*imgboxes.size()+i;
        if(j>timedDisplayer->img_inputs.size()-1)
            break;

        if(isDetecting)
        {
            disImage = QImage(timedDisplayer->img_outputs[j].data,
                    timedDisplayer->img_outputs[j].cols,timedDisplayer->img_outputs[j].rows,
                    timedDisplayer->img_outputs[j].cols*timedDisplayer->img_outputs[j].channels(),
                    QImage::Format_BGR888);
            tempStr="No#"+QString::number(j+1)+" camera refresh time(processed): "
                    +QString::number(gap2recv)+"ms. In which process consumed: "
                    +QString::number(imgProcessors[i]->onceRunTime)+"ms.\n";
        }
        else
        {
            disImage = QImage(timedDisplayer->img_inputs[j].data,
                              timedDisplayer->img_inputs[j].cols,timedDisplayer->img_inputs[j].rows,
                              timedDisplayer->img_inputs[j].cols*timedDisplayer->img_inputs[j].channels(),
                              QImage::Format_RGB888);
            tempStr="No#"+QString::number(j+1)+" camera refresh time: "
                    +QString::number(gap2recv)+" ms.\n";
        }
        pixmapShows[i].setPixmap(QPixmap::fromImage(disImage));
        strOutputs[i]+=tempStr;
        double fps=1000/double(gap2recv);
        tempStr="No#"+QString::number(j+1)+" camera FPS: "
                +QString::number(fps,'f',1)+".\n";
        strOutputs[i]+=tempStr;
        txtbrowsers[i]->setText(strOutputs[i]);
        strOutputs[i].clear();
        tempStr.clear();
    }
//    cout<<"Main thread: "<<QThread::currentThreadId()<<endl;
}

void MainWindow::on_buttonOpenImgList_clicked()
{
    if(ui->buttonOpenImgList->text()=="OpenImgList")
    {
        QString filename = QFileDialog::getExistingDirectory();
        QDir *dir=new QDir(filename);
        QStringList filter;
        filter<<"*.png"<<"*.jpg"<<"*.jpeg"<<"*.bmp";
        imgProcessors[0]->img_filenames =new QList<QFileInfo>(dir->entryInfoList(filter));
        imgProcessors[0]->processFilePic();
        if(imgProcessors[0]->img_filenames->count()>0)
            ui->buttonOpenImgList->setText("Next");
    }
    else if(ui->buttonOpenImgList->text()=="Next")
    {
        if (!imgProcessors[0]->processFilePic())
            ui->buttonOpenImgList->setText("OpenImgList");
    }
}



void MainWindow::on_checkBoxSaving_toggled(bool checked)
{
    timedDisplayer->shouldSave.store(checked);
}

