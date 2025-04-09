#pragma once

#include "image.hpp"
#include <opencv2/opencv.hpp>
#include <QtGui>

cv::Mat convert(const Image &image, bool raw);
QImage cvMatToQImage(const cv::Mat &mat);
