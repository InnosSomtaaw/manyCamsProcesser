#ifndef VIDEOPLAYER_H
#define VIDEOPLAYER_H

#include <stdio.h>
#include "Camera/general_camera.h"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/pixfmt.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}


class VideoPlayer : public General_Camera
{
    Q_OBJECT

public:
    VideoPlayer();
    ~VideoPlayer();

    void startCamera() override;
    void getOneFrame() override;

protected:
    void run() override;

public:
    bool mainwindowIsStopProcess;

private:
    AVFormatContext *pFormatCtx;
    AVCodecContext *pAVctx;
    const AVCodec *pCodec;
    AVFrame *pAVframe, *pAVframeRGB;
    AVPacket *packet;
    uint8_t *buf;

    struct SwsContext *img_convert_ctx;

    int streamIndex, i, numBytes;
    int ret, got_picture;
};

#endif // VIDEOPLAYER_H
