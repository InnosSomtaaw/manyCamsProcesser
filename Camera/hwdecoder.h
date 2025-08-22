#ifndef HWDECODER_H
#define HWDECODER_H

#include "general_camera.h"

#include <opencv2/opencv.hpp>

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

QT_BEGIN_NAMESPACE
using namespace cv;
QT_END_NAMESPACE

class hwDecoder : public General_Camera
{
    Q_OBJECT
public:
    hwDecoder();
    ~hwDecoder();

    void startCamera() override;
    void getOneFrame() override;

protected:
    void run() override;

private:
    AVFormatContext *pFormatCtx;
    AVCodecContext *pAVctx;
    AVCodecHWConfig *pAVStreamInfo;
    AVBufferRef *pAVbufRef;
    const AVCodec *pCodec;
    AVFrame *pAVframe, *pAVframeRGB;
    AVPacket *packet;

    int streamIndex, i, numBytes;
    int ret;

    Mat yuvMat,rgbMat;

    static AVPixelFormat GetHwFormat(AVCodecContext *ctx, const AVPixelFormat *pix_fmts);
};

#endif // HWDECODER_H
