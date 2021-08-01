#include "calibrate.hpp"
#include <iostream>

namespace lasermouse{
Calibrater::Calibrater(int height, int width, int chessboard_cols, int chessboard_rows):
    height_(height), width_(width), chessboard_cols_(chessboard_cols), chessboard_rows_(chessboard_rows) {}

/* description: 根据当前显示器的大小产生一个棋盘图片
 * */
cv::Mat Calibrater::createChessboard() {
    int n_pix = MIN(height_, width_) / 10;  // 一个块的像素数
    int pad_h = height_ - n_pix * (chessboard_rows_+ 1);    
    int pad_w = width_ - n_pix * (chessboard_cols_+ 1);

    int row_start = pad_h / 2, row_end = height_ - (pad_h - pad_h / 2); 
    int col_start = pad_w / 2, col_end = width_ - (pad_w - pad_w / 2);

    // 在中心位置创建棋盘图片
    cv::Mat result = cv::Mat(height_, width_, CV_8UC1, cv::Scalar::all(255));
    for (int i = row_start; i < row_end; i++) {
        for (int j = col_start; j < col_end; j++) {
            if ((((i - row_start) / n_pix) & 0x01) == 0) {  // 如果是偶数行
                if ((((j - col_start) / n_pix) & 0x01) != 0) {
                    result.at<uchar>(i, j) = 0;
                }
            } else {    // 如果是奇数行
                if ((((j - col_start) / n_pix) & 0x01) == 0) {
                    result.at<uchar>(i, j) = 0;
                }
            }
        }
    }

    return result;
}

/* description: 标定当前屏幕与相机的位置
* param 
*     cap: 传入当前相机的视频流
* return: 标定成功返回true，失败返回false
* */
bool Calibrater::calibrate(cv::VideoCapture &cap) {
    if (!cap.isOpened()) {
        std::cerr << "cap open failed" << std::endl;
        return false;
    }
    // 全屏显示图片
    cv::Mat chessboard = createChessboard();

    cv::namedWindow("calibrate", cv::WINDOW_NORMAL);
    cv::setWindowProperty("calibrate", cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);
    cv::imshow("calibrate", chessboard);
    cv::waitKey(100);

    // 开始标定
    cv::Mat frame;
    std::vector<cv::Point2d> corners_show, corners_detect;
    int times = 30;
    while (cap.isOpened()) {
        if (times-- == 0)
            break;
        if (cap.read(frame) == false)
            break;
        
        // 检测角点并匹配
        cv::findChessboardCorners(chessboard, cv::Size(chessboard_cols_, chessboard_rows_), corners_show);
        if (cv::findChessboardCorners(frame, cv::Size(chessboard_cols_, chessboard_rows_), corners_detect)) {
            H_ = cv::findHomography(corners_detect, corners_show);
            break;
        }
    }

    if (H_.empty() && times <= 0) {
        std::cerr << "calibrate failed, timeout!" << std::endl;
        return false;
    }
    cv::destroyWindow("calibrate");
    return true;
}

/* description: 将摄像头拍摄到的图片中的坐标，变换到在显示器中的坐标
* param
*    coords: 输入要进行转换的坐标 
* return: 变换后的坐标
* */
std::vector<std::vector<int>> Calibrater::coordsTransform(const std::vector<std::vector<int>> &coords) {
    std::vector<std::vector<int>> result; 

    // 将输入转换成Mat数组
    cv::Mat X = cv::Mat(3, coords.size(), CV_32FC1, cv::Scalar::all(0)); 
    for (size_t i = 0; i < coords.size(); i++) {
        for (size_t j= 0; j < coords[i].size(); j++) {
            X.at<float>(i, j) = coords[i][j];
        }
    }
    
    // 坐标变换
    cv::Mat Y = H_ * X;

    // 将输出转换成vector
    for (int i = 0; i < Y.cols; i++) {
        std::vector<int> temp;
        temp.push_back(int(Y.at<float>(0, i) / Y.at<float>(2, i)));
        temp.push_back(int(Y.at<float>(1, i) / Y.at<float>(2, i)));
        result.push_back(temp);
    }

    return result;
}

}
