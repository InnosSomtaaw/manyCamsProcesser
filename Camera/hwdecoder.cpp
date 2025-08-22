#include "hwdecoder.h"

hwDecoder::hwDecoder()
{
    isCapturing.store(0); hasFinished = false;hasStarted=false;
}

hwDecoder::~hwDecoder()
{
    av_free(pAVframe);
    av_free(pAVframeRGB);
    avcodec_close(pAVctx);
    avformat_close_input(&pFormatCtx);
    yuvMat.release();
    rgbMat.release();
}

AVPixelFormat hwDecoder::GetHwFormat(AVCodecContext * ctx, const AVPixelFormat * pix_fmts)
{
    const enum AVPixelFormat *p;
    hwDecoder *pThis = (hwDecoder *)ctx->opaque;
    for (p = pix_fmts; *p != -1; p++) {
        if (*p == pThis->pAVStreamInfo->pix_fmt) {
            return *p;
        }
    }

    fprintf(stderr, "Failed to get HW surface format.\n");
    return AV_PIX_FMT_NONE;
}

void hwDecoder::startCamera()
{
//    QThread::msleep(200);
    //Allocate an AVFormatContext.
    pFormatCtx = avformat_alloc_context();

    ///2017.8.5---lizhen
    AVDictionary *avdic=NULL;
    char option_key[]="rtsp_transport";
    char option_value[]="tcp";
    av_dict_set(&avdic,option_key,option_value,0);
    char option_key2[]="max_delay";
    char option_value2[]="100";
    av_dict_set(&avdic,option_key2,option_value2,0);
    ///rtsp地址，可根据实际情况修改
//    char url[]="rtsp://admin:Lead123456@192.168.137.98:554/h265/ch1/main/av_stream";
    QByteArray qba = camURL.toLocal8Bit();
    char *url=qba.data();

    if (avformat_open_input(&pFormatCtx, url, NULL, &avdic) != 0) {
        printf("can't open the file. \n");
        return;
    }
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        printf("Could't find stream infomation.\n");
        return;
    }

    streamIndex = -1;
    ///循环查找视频中包含的流信息，直到找到视频类型的流
    ///便将其记录下来 保存到streamIndex变量中
    ///这里我们现在只处理视频流  音频流先不管他
    for (i = 0; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            streamIndex = i;
        }
    }
    ///如果streamIndex为-1 说明没有找到视频流
    if (streamIndex == -1) {
        printf("Didn't find a video stream.\n");
        return;
    }
    ret = av_find_best_stream(pFormatCtx, AVMEDIA_TYPE_VIDEO, -1, -1, &pCodec, 0);
    if (ret < 0)
        return;
    pAVStreamInfo = new AVCodecHWConfig;
    ///查找解码器
    for (int i = 0;; i++)
    {
        const AVCodecHWConfig *config = avcodec_get_hw_config(pCodec, i);
        if (!config)
        {
            // 没找到cuda解码器，不能使用;
            cout<<"No CUDA decoder found!"<<endl;
            return;
        }
        if (config->device_type == AV_HWDEVICE_TYPE_CUDA) {
            // 找到了cuda解码器，记录对应的AVPixelFormat,后面get_format需要使用;
            pAVStreamInfo->pix_fmt=config->pix_fmt;
            pAVStreamInfo->device_type=AV_HWDEVICE_TYPE_CUDA;
            break;
        }
    }
    pAVctx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(pAVctx, pFormatCtx->streams[streamIndex]->codecpar);
    pAVctx->opaque = this;
    pAVctx->get_format = hwDecoder::GetHwFormat;

//    pAVctx->bit_rate = 6000000;
//    pAVctx->time_base.num=1;  //下面两行：一秒钟25帧
//    pAVctx->time_base.den=12;

//    if (pAVbufRef) {
//        av_buffer_unref(&pAVbufRef);
//        pAVbufRef = NULL;
//    }
    if (av_hwdevice_ctx_create(&pAVbufRef, pAVStreamInfo->device_type,
                               NULL, NULL, 0) < 0) {
        fprintf(stderr, "Failed to create specified HW device.\n");
        // 创建硬解设备context失败，不能使用;
        return;
    }
    pAVctx->hw_device_ctx = av_buffer_ref(pAVbufRef);

    ///打开解码器
    if (avcodec_open2(pAVctx, pCodec, NULL) < 0) {
        printf("Could not open codec.\n");
        return;
    }

    pAVframe = av_frame_alloc();
    pAVframeRGB = av_frame_alloc();
    yuvMat=Mat(pAVctx->height*3/2,pAVctx->width,CV_8U);

    int y_size = pAVctx->width * pAVctx->height;
    packet = (AVPacket *) malloc(sizeof(AVPacket)); //分配一个packet
    av_new_packet(packet, y_size); //分配packet的数据

    hasStarted=true;
    getOneFrame();
}

void hwDecoder::getOneFrame()
{
    this->setAutoDelete(false);
    QThreadPool::globalInstance()->start(this);
}

void hwDecoder::run()
{
    hasFinished = false;
    while (1)
    {
        if (isCapturing==0)
        {
            hasFinished = true;
            printf("isCapturing is false...\n");
            av_packet_unref(packet);//释放资源,否则内存会一直上升
            break; //这里认为视频读取完了
        }

        ret=av_read_frame(pFormatCtx, packet);
        ret=avcodec_send_packet(pAVctx, packet);
        // 每解出来一帧，丢到队列中;
        ret=avcodec_receive_frame(pAVctx, pAVframe);
        if (ret < 0) {
//            cout<<"cam "<<camidx<<" decode error."<<endl;
            av_packet_unref(packet);//释放资源,否则内存会一直上升
            QThread::msleep(froceLag);
            continue;
        }

        if (pAVStreamInfo->pix_fmt != AV_PIX_FMT_NONE &&
            pAVStreamInfo->device_type != AV_HWDEVICE_TYPE_NONE)
        {
            ret = av_hwframe_transfer_data(pAVframeRGB, pAVframe, 0);
            if (ret < 0) {
                av_frame_unref(pAVframeRGB);
                return;
            }
            av_frame_copy_props(pAVframeRGB, pAVframe);
            /// pAVframeRGB中的data，就是硬解完成后的视频帧数据

            memcpy(yuvMat.data,pAVframeRGB->data[0],pAVctx->width*pAVctx->height);
            memcpy(yuvMat.data + pAVctx->width*pAVctx->height, pAVframeRGB->data[1],
                    pAVctx->width*pAVctx->height / 2);

            if(isCapturing==1)
            {
                ///Convert yuv to rgb
                cvtColor(yuvMat,rgbMat,COLOR_YUV2RGB_NV12);
                QImage tmpImg(rgbMat.data,rgbMat.cols,rgbMat.rows,
                              rgbMat.cols*rgbMat.channels(),QImage::Format_RGB888);
                QImage image = tmpImg.copy(); //把图像复制一份 传递给界面显示
                emit sigGetOneFrame(image,camidx);  //发送信号
            }
            else if(isCapturing>1)
            {
                QImage tmpImg(yuvMat.data,yuvMat.cols,yuvMat.rows,
                              yuvMat.cols*yuvMat.channels(),QImage::Format_Grayscale8);
                QImage image = tmpImg.copy();
                emit sigProcessOnce(image);
            }
        }
        av_packet_unref(packet);//释放资源,否则内存会一直上升
        QThread::msleep(froceLag);
    }
}
