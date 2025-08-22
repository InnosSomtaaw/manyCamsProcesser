#include "image_processing.h"

Image_Processing_Class::Image_Processing_Class()
{
    initImgInOut();
    hasInited=false;cam1Refreshed=false;cam2Refreshed = false;isDetecting=false;onGPU=false;
    img_filenames = new QList<QFileInfo>();
    save_count = 0,max_save_count =100,onceRunTime=0;
    mainwindowIsNext=false;mainwindowIsStopProcess=false;isSavingImage =false;
    inputFlags.insert(inputFlags.begin(),4,false);
    busyProcessing.store(false);

    basePTs = new QQueue<Point>();
    islifting = new QQueue<bool>();
}

Image_Processing_Class::~Image_Processing_Class()
{
}

void Image_Processing_Class::initImgInOut()
{
    for(int i = 0; i<4; i++)
    {
        Mat imgIn, imgOut;
        img_inputs.push_back(imgIn.clone());
        img_outputs.push_back(imgOut.clone());
        inputFlags.push_back(false);
        vector<Point> pts;
        ptsVec.push_back(pts);
    }
}

void Image_Processing_Class::resetPar()
{
    mainwindowIsNext=false;mainwindowIsStopProcess=false;isSavingImage =false;
    save_count=0;
    hasInited=false;cam1Refreshed=false;cam2Refreshed = false;
    img_filenames->clear();
    basePTs->clear();
    islifting->clear();
}

void Image_Processing_Class::run()
{

}

void Image_Processing_Class::resizeCompare()
{
    if (img_input1.empty())
        return;

    ipcMutex.lock();
    //测试线程：
    {
        img_output1 = img_input1.clone();
//        Laplacian(img_output1,img_output1,img_output1.depth());
        resize(img_output1,img_output1,Size(),0.5,0.5);

        ipcMutex.unlock();

//        if (save_count % max_save_count == 0)
//        {
//            imwrite("test1.jpg",img_output1);
//            isSavingImage = true;
//        }
//        else
//            isSavingImage = false;

//        save_count++;
//        if (save_count > max_save_count)
//            save_count = 1;

        emit outputImgProcessedRequest(procerIdx);
        return;
    }

}

void Image_Processing_Class::startProcessOnce()
{
    resizeCompare();
}

void Image_Processing_Class::startPicsProcess()
{
    emit mainwindowStatusRequest(procerIdx);
    do
    {
        bool flg = processFilePic();
        if(mainwindowIsStopProcess || !flg)
            break;
        QThread::sleep(10);
        emit mainwindowStatusRequest(procerIdx);
    }while(mainwindowIsNext);
}

void Image_Processing_Class::start1CamProcess(QImage receivedImg)
{
    if(receivedImg.isNull())
        return;
    Mat temp_img(receivedImg.height(),receivedImg.width(),
                CV_8UC4,receivedImg.bits(),
                size_t(receivedImg.bytesPerLine()));
    cvtColor(temp_img,img_input1,COLOR_BGRA2BGR);
//    cvtColor(temp_img,img_input1,CV_BGRA2BGR);
    temp_img.release();
}

void Image_Processing_Class::startMulCamProcess(QImage recvImg, int i)
{

}

void Image_Processing_Class::changeProcPara(QString qstr, int wc)
{
}

void Image_Processing_Class::iniImgProcessor()
{

}

bool Image_Processing_Class::processFilePic()
{
    if (img_filenames->count() > 0)
    {
        QImageReader qir(img_filenames->at(0).filePath());
        QImage img=qir.read();
        img=img.convertToFormat(QImage::Format_BGR888);
        img_input2=Mat(img.height(), img.width(), CV_8UC3,img.bits(), img.bytesPerLine());
//        img_input1 = imread(img_filenames->at(0).filePath().toLocal8Bit().toStdString());
        if(img_input2.empty())
            return false;
        img_input2.copyTo(img_input1);

        startProcessOnce();

        img_filenames->removeAt(0);
        return true;
    }
    else
    {
        return false;
    }
}

void ImageWriter::run()
{  
    if (!iwtMutex.tryLock())
        return;
    //    usrtimer.start();

    //按需保存图片：
    QDateTime current_date_time =QDateTime::currentDateTime();
    QString current_date =current_date_time.toString("yyyy_MM_dd");
    QString current_time =current_date_time.toString("hh_mm_ss_zzz");
    QString dir_str = "/mnt/home/zhangshaoyue/SaveFile/"+current_date+"/";
    QDir dir;
    if (!dir.exists(dir_str))
    {
        dir.mkpath(dir_str);
        autoDeleteFolder(30);
    }
    QString fn;
    switch (method) {
    case 1:
        fn=dir_str+headname+current_time+"saved.bmp";
        break;
    case 2:
        fn=dir_str+headname+current_time+"saved.jpg";
        break;
    default:
        fn=dir_str+headname+current_time+"saved.png";
        break;
    }

    qimg.save(fn);
    QThread::sleep(30);
    //    cout<<"Save time(width:"<<qimg.width()<<",height:"<<qimg.height()<<
    //          "): "<<usrtimer.elapsed()<<" ms."<<endl;
    iwtMutex.unlock();
}

void ImageWriter::autoDeleteFolder(int numBk)
{
    QDate currentDate =QDate::currentDate();
    QDir dir("/mnt/home/orangepi/SaveFile/");
    if(!dir.exists())
        return;
    dir.setFilter(QDir::Dirs);
    dir.setSorting(QDir::DirsFirst);
    QFileInfoList list = dir.entryInfoList();
    int i=0;
    bool bIsDir;
    do
    {
        QFileInfo fileInfo = list.at(i);
        if(fileInfo.fileName() == "." | fileInfo.fileName() == "..")
        {
            ++i;
            continue;
        }
        bIsDir = fileInfo.isDir();
        if(bIsDir)
        {
            QDate folderDate;
            folderDate = QDate::fromString(fileInfo.fileName(),"yyyy_MM_dd");
            qint64 pastDays = currentDate.toJulianDay()-folderDate.toJulianDay();
            if(pastDays>numBk)
            {
                dir.setPath(fileInfo.filePath());
                dir.removeRecursively();
            }
        }
        ++i;
    }while(i < list.size());
}
