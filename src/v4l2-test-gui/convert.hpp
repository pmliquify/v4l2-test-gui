#pragma once

#include "image.hpp"
#include <opencv2/opencv.hpp>
#include <QtGui>

cv::Mat convert(const Image &image, bool raw);
cv::Mat convert_bayer10p_to_rgb(const uint8_t *data, int width, int height, int blackcols);
QImage cvMatToQImage(const cv::Mat &mat);
