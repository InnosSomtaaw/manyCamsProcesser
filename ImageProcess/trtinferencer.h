#ifndef TRTINFERENCER_H
#define TRTINFERENCER_H

#include "ImageProcess/image_processing.h"
#include "ImageProcess/yolopostprocess.h"

#include <fstream>
#include <NvInfer.h>
#include <NvOnnxParser.h>

QT_BEGIN_NAMESPACE
using namespace nvinfer1;
using namespace nvonnxparser;
QT_END_NAMESPACE

class Logger : public ILogger
{
    void log(Severity severity, const char* msg) noexcept override;
};

class trtInferencer : public Image_Processing_Class
{
    Q_OBJECT
public:
    trtInferencer();
    ~trtInferencer();

    QString prog_file;
    WorkConditionsEnum workCond;
    yoloPostProcess* y8pp;
    atomic<bool> yuvInput;

public slots:
    void iniImgProcessor() override;
    void startProcessOnce() override;

    void proQImgOnce(QImage qimg);
    void run() override;

private:
    Logger logger;
    IBuilder* builder;
    IParser* parser;
    INetworkDefinition* network;
    IBuilderConfig* config;
    IHostMemory* serializedModel;
    IRuntime* runtime;
    ICudaEngine* engine;
    IExecutionContext *context;

    vector<vector<float>> input_datas,output_datas;
    vector<Dims> input_dims,output_dims;
    vector<string> input_names,output_names;
    size_t input_nums,output_nums;
    vector<Mat> outImgs;
    cudaError_t cet;
    cudaStream_t stream;
    vector<void*> mDeviceBindings;

    size_t byteSize;
    vector<Vec3b> colors;

    void preprocess();
    void postprocess();
};

#endif // TRTINFERENCER_H
