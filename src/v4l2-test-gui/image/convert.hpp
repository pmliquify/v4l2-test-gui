// Copyright (c) 2026 Peter Martienssen
// SPDX-License-Identifier: MIT

#pragma once

#include "image.hpp"
#include <opencv2/opencv.hpp>
#include <QtGui>

cv::Mat convert(const Image &image,  int strideOffset, bool raw);
QImage cvMatToQImage(const cv::Mat &mat);
