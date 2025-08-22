#include "trtinferencer.h"

trtInferencer::trtInferencer()
{
    hasInited=false;
    yuvInput=false;
    busyProcessing.store(false);
    this->setAutoDelete(false);
}

trtInferencer::~trtInferencer()
{
    cudaStreamDestroy(stream);
    for(size_t i=0;i<mDeviceBindings.size();i++)
        cudaFree(mDeviceBindings[i]);

    delete parser;
    delete network;
    delete config;
    delete builder;
}

void trtInferencer::iniImgProcessor()
{
    QFileInfo modelFi(prog_file);
    if(!modelFi.isFile())
        return;

    cout<<prog_file.toStdString()<<endl;
    builder=createInferBuilder(logger);
    const auto explicitBatch = 1U << static_cast<uint32_t>(NetworkDefinitionCreationFlag::kEXPLICIT_BATCH);
    network=builder->createNetworkV2(explicitBatch);
    parser = createParser(*network, logger);

    QByteArray qba = prog_file.toLocal8Bit();
    char* modelFile=qba.data();
    parser->parseFromFile(modelFile,
                          static_cast<int32_t>(ILogger::Severity::kWARNING));
    for (int32_t i = 0; i < parser->getNbErrors(); ++i)
        cout << parser->getError(i)->desc() << endl;

    input_nums=network->getNbInputs();
    output_nums=network->getNbOutputs();
    input_dims.resize(input_nums);
    output_dims.resize(output_nums);
    input_names.resize(input_nums);
    output_names.resize(output_nums);
    for(size_t i=0;i<input_nums;i++)
    {
        input_dims[i]=network->getInput(i)->getDimensions();
        input_names[i]=network->getInput(i)->getName();
    }
    for(size_t i=0;i<output_nums;i++)
    {
        output_dims[i]=network->getOutput(i)->getDimensions();
        output_names[i]=network->getOutput(i)->getName();
    }

    IOptimizationProfile* profile = builder->createOptimizationProfile();
    if(input_dims[0].d[0]<0)
    {
        input_dims[0].d[0]=1;
        input_dims[0].d[1]=3;
        input_dims[0].d[2]=640;
        input_dims[0].d[3]=640;
        //这里的尺寸更具你自己的输入修改（最小尺寸）
        profile->setDimensions(input_names[0].data(), OptProfileSelector::kMIN,
                Dims4(input_dims[0].d[0],input_dims[0].d[1],input_dims[0].d[2],input_dims[0].d[3]));
        //这里的尺寸更具你自己的输入修改（默认尺寸）
        profile->setDimensions(input_names[0].data(), OptProfileSelector::kOPT,
                Dims4(input_dims[0].d[0],input_dims[0].d[1],input_dims[0].d[2],input_dims[0].d[3]));
        //这里的尺寸更具你自己的输入修改（最大尺寸）
        profile->setDimensions(input_names[0].data(), OptProfileSelector::kMAX,
                Dims4(input_dims[0].d[0],input_dims[0].d[1],input_dims[0].d[2],input_dims[0].d[3]));
    }

    if(output_dims[0].d[0]<0)
    {
        output_dims[0].d[0]=1;
        output_dims[0].d[1]=1;
        output_dims[0].d[2]=640;
        output_dims[0].d[3]=640;
        //这里的尺寸更具你自己的输入修改（最小尺寸）
        profile->setDimensions(output_names[0].data(), OptProfileSelector::kMIN,
                Dims4(output_dims[0].d[0],output_dims[0].d[1],output_dims[0].d[2],output_dims[0].d[3]));
        //这里的尺寸更具你自己的输入修改（默认尺寸）
        profile->setDimensions(output_names[0].data(), OptProfileSelector::kOPT,
                Dims4(output_dims[0].d[0],output_dims[0].d[1],output_dims[0].d[2],output_dims[0].d[3]));
        //这里的尺寸更具你自己的输入修改（最大尺寸）
        profile->setDimensions(output_names[0].data(), OptProfileSelector::kMAX,
                Dims4(output_dims[0].d[0],output_dims[0].d[1],output_dims[0].d[2],output_dims[0].d[3]));
    }

    QString eng_file=modelFi.path()+"/"+modelFi.baseName()+".engine";
    modelFi=QFileInfo(eng_file);
    if(modelFi.isFile())
    {
        char *trtModelStream{nullptr};
        size_t size{0};
        // 读取文件
        ifstream file(eng_file.toStdString(), ios::binary);
        if (file.good()) {
            file.seekg(0, file.end);
            size = file.tellg();
            file.seekg(0, file.beg);
            trtModelStream = new char[size];
            assert(trtModelStream);
            file.read(trtModelStream, size);
            file.close();
        }
        cout << "engine init finished" << endl;
        runtime = createInferRuntime(logger);
        assert(runtime != nullptr);
        engine = runtime->deserializeCudaEngine(trtModelStream, size);
        assert(engine != nullptr);
        context = engine->createExecutionContext();
        assert(context != nullptr);
        delete[] trtModelStream;
    }
    else
    {
        config = builder->createBuilderConfig();
        config->setFlag(BuilderFlag::kOBEY_PRECISION_CONSTRAINTS);
        config->addOptimizationProfile(profile);	//添加进 IBuilderConfig

        serializedModel = builder->buildSerializedNetwork(*network, *config);
        runtime = createInferRuntime(logger);
        assert(runtime != nullptr);
        engine = runtime->deserializeCudaEngine(serializedModel->data(), serializedModel->size());
        assert(engine != nullptr);
        context = engine->createExecutionContext();

        qba=eng_file.toLocal8Bit();
        FILE* fp = fopen(qba.data(), "w");
        fwrite(serializedModel->data(), 1, serializedModel->size(),fp);
        fclose(fp);
    }

    input_dims[0].d[0]=1;
    output_dims[0].d[0]=1;

    cet=cudaStreamCreate(&stream);
    context->setOptimizationProfileAsync(0, stream);

    mDeviceBindings.resize(input_nums+output_nums);
    for (int i = 0; i < input_nums; i++)
    {
        byteSize=1;
        for(size_t j=0;j<input_dims[i].nbDims;j++)
            byteSize*=input_dims[i].d[j];
        cet=cudaMalloc(&mDeviceBindings[i], byteSize* sizeof(float));
        context->setTensorAddress(input_names[i].c_str(),mDeviceBindings[i]);
    }
    for (int i = input_nums; i < input_nums+output_nums; i++)
    {
        byteSize=1;
        for(size_t j=0;j<output_dims[i-input_nums].nbDims;j++)
            byteSize*=output_dims[i-input_nums].d[j];
        cet=cudaMalloc(&mDeviceBindings[i], byteSize* sizeof(float));
        context->setTensorAddress(output_names[i-input_nums].c_str(),mDeviceBindings[i]);
    }

    switch (workCond) {
    case ObjDet:{
        y8pp=new yoloPostProcess;
        std::array<int,3> szBlob={output_dims[0].d[0],output_dims[0].d[1],
                                  output_dims[0].d[2]};
        img_output2=Mat(3,szBlob.data(),CV_32F);
        y8pp->INPUT_HEIGHT=input_dims[0].d[2];
        y8pp->INPUT_WIDTH=input_dims[0].d[3];
        break;}
    case SemSeg:{
        img_output2=Mat(output_dims[0].d[1],output_dims[0].d[2],CV_32S);
        break;}
    case Anomaly:
    default:{
        std::array<int,4> szBlob={output_dims[0].d[0],output_dims[0].d[1],
                                  output_dims[0].d[2],output_dims[0].d[3]};
        img_output2=Mat(4,szBlob.data(),CV_32F);
        break;}
    }

    hasInited=true;
    this->setAutoDelete(false);
}

void trtInferencer::preprocess()
{
    ///Convert yuv to rgb
    if(yuvInput.load())
        cvtColor(img_input2,img_input1,COLOR_YUV2RGB_NV12);

    switch (workCond) {
    case ObjDet:
        y8pp->letterboxInfo=y8pp->letterbox(
                    img_input1,img_input2,Size(input_dims[0].d[3],input_dims[0].d[2]),
                Scalar(114, 114, 114),false);
        dnn::blobFromImage(img_input2,img_output1, 1.0 / 255.0);
        break;
    case SemSeg:
    case Anomaly:
    default:
        dnn::blobFromImage(img_input1,img_output1,1.0f/255.0f,
                           Size(input_dims[0].d[3],input_dims[0].d[2]),
                Scalar(127.5, 127.5, 127.5));
        break;
    }
    // Create GPU buffers on device
    byteSize=1;
    for(size_t j=0;j<input_dims[0].nbDims;j++)
        byteSize*=input_dims[0].d[j];
    cet=cudaMemcpyAsync(mDeviceBindings[0], (float*)img_output1.data, byteSize*sizeof(float),
            cudaMemcpyKind::cudaMemcpyHostToDevice,stream);//异步推理
//    cet=cudaMemcpy(mDeviceBindings[0],(float*)img_output1.data,byteSize*sizeof(float),
//            cudaMemcpyKind::cudaMemcpyHostToDevice);//同步推理
}

void trtInferencer::postprocess()
{
//    nvinfer1::DataType odt=engine->getTensorDataType(output_names[0].c_str());
    byteSize=1;
    for(size_t j=0;j<output_dims[0].nbDims;j++)
        byteSize*=output_dims[0].d[j];

    cet=cudaMemcpyAsync(img_output2.data,mDeviceBindings[1], byteSize*sizeof(float),
            cudaMemcpyKind::cudaMemcpyDeviceToHost,stream);//异步推理
    cudaStreamSynchronize(stream);//异步推理
    //        cet=cudaMemcpy(img_output2.data,mDeviceBindings[1], byteSize*sizeof(float),
    //                cudaMemcpyKind::cudaMemcpyDeviceToHost);//同步推理

    switch (workCond) {
    case ObjDet:{
        // 后处理
        cvtColor(img_input1,img_output3,COLOR_RGB2BGR);
        img_output2=img_output2.reshape(1,output_dims[0].d[1]);
        transpose(img_output2,img_output2);
        y8pp->postprocess(img_output2);
        for (size_t idx = 0; idx < y8pp->posRects.size(); ++idx)
            y8pp->drawPred(y8pp->ids[idx],y8pp->confds[idx],
                           y8pp->posRects[idx].x,y8pp->posRects[idx].y,
                           y8pp->posRects[idx].x + y8pp->posRects[idx].width,
                           y8pp->posRects[idx].y + y8pp->posRects[idx].height,
                           img_output3);
        break;}
    case SemSeg:{
        img_output2.convertTo(img_input2,CV_8U,10);
        applyColorMap(img_input2,img_output3,COLORMAP_VIRIDIS);
        break;}
    case Anomaly:{
        dnn::imagesFromBlob(img_output2,img_outputs);
        img_outputs[0].convertTo(img_input2,CV_8U,255);
        resize(img_input2,img_input2,img_input1.size(),0,0,InterpolationFlags::INTER_NEAREST);
        applyColorMap(img_input2,img_output3,COLORMAP_VIRIDIS);
        break;}
    default:{
        dnn::imagesFromBlob(img_output2,img_outputs);
        break;}
    }
}

void trtInferencer::proQImgOnce(QImage qimg)
{
    if(busyProcessing.exchange(true))
        return;

//    if (!ipcMutex.tryLock())
//        return;

//    img_input1=Mat(qimg.height(), qimg.width(), CV_8UC3,
//                   qimg.bits(), qimg.bytesPerLine());
    yuvInput.store(true);
    img_input2=Mat(qimg.height(), qimg.width(), CV_8U,
                   qimg.bits(), qimg.bytesPerLine());

//    ipcMutex.unlock();
    QThreadPool::globalInstance()->start(this);
}

void trtInferencer::startProcessOnce()
{
    if(img_input1.empty())
        return;

//    img_input1.copyTo(img_input2);
    run();
//    this->setAutoDelete(false);
//    QThreadPool::globalInstance()->start(this);
}

void trtInferencer::run()
{    
    usrtimer.start();
//    if (!ipcMutex.tryLock())
//        return;
//    isProcessing=true;
    preprocess();

//    cout<< "pre-process time is " <<usrtimer.elapsed()<< " ms"<<endl;
    //运行
    context->enqueueV3(stream); //异步推理
//    context->executeV2(mDeviceBindings.data()); //同步推理
//    cout<< "predict time is " <<usrtimer.elapsed()<< " ms"<<endl;

    postprocess();
//    ipcMutex.unlock();
    busyProcessing.store(false);

    onceRunTime=usrtimer.elapsed();
//    cout<< "post-process time is " <<onceRunTime<< " ms"<<endl;
    emit outputImgProcessedRequest(procerIdx);
}

void Logger::log(Severity severity, const char *msg) noexcept
{
    // suppress info-level messages
    if (severity <= Severity::kWARNING)
        cout << msg << endl;
}
