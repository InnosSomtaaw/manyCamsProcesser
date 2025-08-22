#include "videoplayer.h"


VideoPlayer::VideoPlayer()
{
  isCapturing = 0; hasFinished = false;hasStarted=false;
}

VideoPlayer::~VideoPlayer()
{
    av_free(buf);
    av_free(pAVframe);
    av_free(pAVframeRGB);
    avcodec_close(pAVctx);
    avformat_close_input(&pFormatCtx);
    isCapturing=0;
    hasFinished=true;
}

void VideoPlayer::startCamera()
{
//    AVFormatContext *pFormatCtx;
//    AVCodecContext *pAVctx;
//    const AVCodec *pCodec;
//    AVFrame *pAVframe, *pAVframeRGB;
//    AVPacket *packet;
//    uint8_t *buf;

//    static struct SwsContext *img_convert_ctx;

//    int streamIndex, i, numBytes;
//    int ret, got_picture;

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

    ///查找解码器
    pAVctx = avcodec_alloc_context3(NULL);
    avcodec_parameters_to_context(pAVctx, pFormatCtx->streams[streamIndex]->codecpar);
    pCodec = avcodec_find_decoder(pAVctx->codec_id);

    ///2017.8.9---lizhen
    pAVctx->bit_rate =0;   //初始化为0
    pAVctx->time_base.num=1;  //下面两行：一秒钟25帧
    pAVctx->time_base.den=12;
//    pAVctx->frame_number=1;  //每包一个视频帧

    if (pCodec == NULL) {
        printf("Codec not found.\n");
        return;
    }

    ///打开解码器
    if (avcodec_open2(pAVctx, pCodec, NULL) < 0) {
        printf("Could not open codec.\n");
        return;
    }

    pAVframe = av_frame_alloc();
    pAVframeRGB = av_frame_alloc();

    buf = (unsigned char *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_RGB24, pAVctx->width, pAVctx->height, 1));
    av_image_fill_arrays(pAVframeRGB->data, pAVframeRGB->linesize, buf, AV_PIX_FMT_RGB24, pAVctx->width, pAVctx->height, 1);

    ///这里我们改成了 将解码后的YUV数据转换成RGB24
    img_convert_ctx = sws_getContext(pAVctx->width, pAVctx->height,
            pAVctx->pix_fmt, pAVctx->width, pAVctx->height,
            AV_PIX_FMT_RGB24, SWS_BICUBIC, NULL, NULL, NULL);

    int y_size = pAVctx->width * pAVctx->height;
    packet = (AVPacket *) malloc(sizeof(AVPacket)); //分配一个packet
    av_new_packet(packet, y_size); //分配packet的数据

    hasStarted=true;
    getOneFrame();
}

void VideoPlayer::getOneFrame()
{
    this->setAutoDelete(false);
    QThreadPool::globalInstance()->start(this);
}

void VideoPlayer::run()
{
    hasFinished = false;
    while (1)
    {
        if (av_read_frame(pFormatCtx, packet) < 0 || isCapturing==0)
        {
            hasFinished = true;
            printf("isCapturing is false...\n");
            av_packet_unref(packet);//释放资源,否则内存会一直上升
            break; //这里认为视频读取完了
        }

        if (packet->stream_index == streamIndex) {
            got_picture=avcodec_send_packet(pAVctx, packet);
            ret=avcodec_receive_frame(pAVctx, pAVframe);

            if (ret < 0) {
                printf("decode error.\n");
                QThread::msleep(40);
                continue;
            }
            if (!got_picture)
            {
                sws_scale(img_convert_ctx,
                        (uint8_t const * const *) pAVframe->data,
                        pAVframe->linesize, 0, pAVctx->height, pAVframeRGB->data,
                        pAVframeRGB->linesize);
                //把这个RGB数据 用QImage加载
                QImage tmpImg((uchar *)buf,pAVctx->width,pAVctx->height,QImage::Format_RGB888);
                QImage image = tmpImg.copy(); //把图像复制一份 传递给界面显示
                emit sigGetOneFrame(image,camidx);  //发送信号
            }
        }
        av_packet_unref(packet);//释放资源,否则内存会一直上升
        QThread::msleep(froceLag);
    }
}
