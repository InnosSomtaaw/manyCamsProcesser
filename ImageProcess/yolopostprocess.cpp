#include "yolopostprocess.h"

yoloPostProcess::yoloPostProcess()
{

}

/**
 * @brief 对图像进行letterbox预处理，保持宽高比并填充
 *
 * @param src 输入图像
 * @param dst 输出图像
 * @param out_size 目标尺寸 (width, height)
 * @param color 填充颜色，默认为(114, 114, 114)
 * @param auto 自动调整尺寸，默认为true
 * @param scale_fill 是否拉伸填充，默认为false
 * @param scale_up 是否允许放大，默认为true
 * @return std::vector<float> 缩放比例和偏移量 [w_scale, h_scale, x_offset, y_offset]
 */
std::vector<float> yoloPostProcess::letterbox(const cv::Mat& src, cv::Mat& dst,
                            const cv::Size& out_size,const cv::Scalar& color,
                            bool auto_flag,bool scale_fill,bool scale_up)
{
    // 源图像尺寸
    int src_h = src.rows;
    int src_w = src.cols;
    // 目标尺寸
    int dst_w = out_size.width;
    int dst_h = out_size.height;

    // 计算缩放比例
    float scale = std::min(static_cast<float>(dst_h) / src_h,
                          static_cast<float>(dst_w) / src_w);

    // 如果不允许放大，则缩放比例不超过1.0
    if (!scale_up) {
        scale = std::min(scale, 1.0f);
    }

    // 计算缩放后的图像尺寸
    int new_unpad_w = static_cast<int>(std::round(src_w * scale));
    int new_unpad_h = static_cast<int>(std::round(src_h * scale));

    // 计算填充量
    int pad_w = dst_w - new_unpad_w;
    int pad_h = dst_h - new_unpad_h;

    // 如果auto模式开启，则只在需要时填充
    if (auto_flag) {
        pad_w = pad_w % 32;  // 通常选择32的倍数，适应模型下采样
        pad_h = pad_h % 32;
    }

    // 如果scale_fill开启，则直接拉伸图像，不保持比例
    if (scale_fill) {
        new_unpad_w = dst_w;
        new_unpad_h = dst_h;
        pad_w = 0;
        pad_h = 0;
    }

    // 计算上下左右填充量，使图像居中
    pad_w /= 2;
    pad_h /= 2;

    // 调整图像大小
    cv::Mat resized;
    if (src_w != new_unpad_w || src_h != new_unpad_h) {
        cv::resize(src, resized, cv::Size(new_unpad_w, new_unpad_h), 0, 0, cv::INTER_LINEAR);
    } else {
        resized = src.clone();
    }

    // 计算填充边界
    int top = static_cast<int>(std::round(pad_h - 0.1));
    int bottom = static_cast<int>(std::round(pad_h + 0.1));
    int left = static_cast<int>(std::round(pad_w - 0.1));
    int right = static_cast<int>(std::round(pad_w + 0.1));

    // 填充图像
    cv::copyMakeBorder(resized, dst, top, bottom, left, right, cv::BORDER_CONSTANT, color);

    // 返回缩放比例和偏移量，用于后续坐标转换
    return {static_cast<float>(new_unpad_w) / src_w,
            static_cast<float>(new_unpad_h) / src_h,
            static_cast<float>(left),
            static_cast<float>(top)};
}

void yoloPostProcess::postprocess(Mat &out)
{
    ids.clear();
    confds.clear();
    posRects.clear();
    // Network produces output blob with a shape NxC where N is a number of
    // detected objects and C is a number of classes + 4 where the first 4
    // numbers are [center_x, center_y, width, height]
    float* data = (float*)out.data;
    for (int i = 0; i < out.rows; ++i, data += out.cols)
    {
        Mat scores = out.row(i).colRange(4, out.cols);
        Point classIdPoint;
        double confidence;
        minMaxLoc(scores, 0, &confidence, 0, &classIdPoint);
        if (confidence > SCORE_THRESHOLD)
        {
            int left = int((data[0] - 0.5 * data[2]-letterboxInfo[2])/letterboxInfo[0]);
            int top = int((data[1] - 0.5 * data[3]-letterboxInfo[3])/letterboxInfo[1]);
            int width = int(data[2]/letterboxInfo[0]);
            int height = int(data[3]/letterboxInfo[1]);
            ids.push_back(classIdPoint.x);
            confds.push_back((float)confidence);
            posRects.push_back(Rect(left, top, width, height));
        }
    }
    // NMS
    std::map<int, std::vector<size_t> > class2indices;
    for (size_t i = 0; i < ids.size(); i++)
        if (confds[i] >= SCORE_THRESHOLD)
            class2indices[ids[i]].push_back(i);
    std::vector<Rect> nmsBoxes;
    std::vector<float> nmsConfidences;
    std::vector<int> nmsClassIds;
    for (std::map<int, std::vector<size_t> >::iterator it = class2indices.begin();
         it != class2indices.end(); ++it)
    {
        std::vector<Rect> localBoxes;
        std::vector<float> localConfidences;
        std::vector<size_t> classIndices = it->second;
        for (size_t i = 0; i < classIndices.size(); i++)
        {
            localBoxes.push_back(posRects[classIndices[i]]);
            localConfidences.push_back(confds[classIndices[i]]);
        }
        std::vector<int> nmsIndices;
        dnn::NMSBoxes(localBoxes, localConfidences, SCORE_THRESHOLD, NMS_THRESHOLD, nmsIndices);
        for (size_t i = 0; i < nmsIndices.size(); i++)
        {
            size_t idx = nmsIndices[i];
            nmsBoxes.push_back(localBoxes[idx]);
            nmsConfidences.push_back(localConfidences[idx]);
            nmsClassIds.push_back(it->first);
        }
    }
    posRects = nmsBoxes;
    ids = nmsClassIds;
    confds = nmsConfidences;
}

void yoloPostProcess::drawPred(int classId, float conf, int left, int top, int right, int bottom, Mat& frame)
{
    rectangle(frame, Point(left, top), Point(right, bottom), Scalar(0, 255, 0),4);

    std::string label = format("%.2f", conf);
    // COCO数据集
    if (!classes.empty())
    {
        CV_Assert(classId < (int)classes.size());
        label = classes[classId] + ": " + label;
    }

    int baseLine;
    Size labelSize = getTextSize(label, FONT_HERSHEY_SIMPLEX, 0.5, 1, &baseLine);

    top = max(top, labelSize.height);
    rectangle(frame, Point(left, top - labelSize.height),
              Point(left + labelSize.width, top + baseLine), Scalar::all(255), FILLED);
    putText(frame, label, Point(left, top), FONT_HERSHEY_SIMPLEX, 0.5, Scalar(),2);
}


