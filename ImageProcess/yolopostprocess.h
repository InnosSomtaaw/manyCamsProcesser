#ifndef YOLOPOSTPROCESS_H
#define YOLOPOSTPROCESS_H

#include <QObject>
#include <opencv2/opencv.hpp>
#include <vector>
#include <algorithm>
#include <iostream>
#include <string>

QT_BEGIN_NAMESPACE
using namespace cv;
using namespace std;
QT_END_NAMESPACE

class yoloPostProcess : public QObject
{
    Q_OBJECT
public:
    // YOLOv8n模型输入尺寸
    int INPUT_WIDTH = 640;
    int INPUT_HEIGHT = 640;

    // 置信度和NMS阈值
    float SCORE_THRESHOLD = 0.5f;
    float NMS_THRESHOLD = 0.4f;

    vector<float> letterboxInfo;

    // COCO数据集类别名称
    vector<string> classes = {
        "person", "bicycle", "car", "motorcycle", "airplane", "bus", "train", "truck", "boat",
        "traffic light", "fire hydrant", "stop sign", "parking meter", "bench", "bird", "cat",
        "dog", "horse", "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack",
        "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis", "snowboard", "sports ball",
        "kite", "baseball bat", "baseball glove", "skateboard", "surfboard", "tennis racket",
        "bottle", "wine glass", "cup", "fork", "knife", "spoon", "bowl", "banana", "apple",
        "sandwich", "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake", "chair",
        "couch", "potted plant", "bed", "dining table", "toilet", "tv", "laptop", "mouse", "remote",
        "keyboard", "cell phone", "microwave", "oven", "toaster", "sink", "refrigerator", "book",
        "clock", "vase", "scissors", "teddy bear", "hair drier", "toothbrush"
    };

    vector<int> ids;
    vector<float> confds;
    vector<Rect> posRects;

    yoloPostProcess();

    void drawPred(int classId, float conf, int left, int top, int right, int bottom, Mat &frame);
    void postprocess(Mat &out);
    std::vector<float> letterbox(const cv::Mat &src, cv::Mat &dst,
                                 const cv::Size &out_size,
                                 const cv::Scalar &color = cv::Scalar(114, 114, 114),
                                 bool auto_flag = true, bool scale_fill = false,
                                 bool scale_up = true);
signals:

};

#endif // YOLOPOSTPROCESS_H
