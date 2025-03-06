#include "convert.hpp"
#include <linux/videodev2.h>


cv::Mat convert(const Image &image, bool raw)
{
        int type = CV_8UC1;
        bool debayer = false;
        int divider = 1;
        int code = 0;

        switch (image.pixelformat()) {
        case V4L2_PIX_FMT_GREY:    type =  CV_8UC1; debayer = false; divider =    1; break;
        case V4L2_PIX_FMT_SRGGB8:  type =  CV_8UC1; debayer = true;  divider =    1; code =  cv::COLOR_BayerBG2RGB; break;
        case V4L2_PIX_FMT_SGBRG8:  type =  CV_8UC1; debayer = true;  divider =    1; code =  cv::COLOR_BayerGB2RGB; break;
        case V4L2_PIX_FMT_SGRBG8:  type =  CV_8UC1; debayer = true;  divider =    1; code =  cv::COLOR_BayerGR2RGB; break;
        case V4L2_PIX_FMT_SBGGR8:  type =  CV_8UC1; debayer = true;  divider =    1; code =  cv::COLOR_BayerBG2RGB; break;
        case V4L2_PIX_FMT_Y10:     type = CV_16UC1; debayer = false; divider = 1023; break;
        case V4L2_PIX_FMT_SRGGB10: type = CV_16UC1; debayer = true;  divider = 1023; code =  cv::COLOR_BayerRG2RGB; break;
        case V4L2_PIX_FMT_SGBRG10: type = CV_16UC1; debayer = true;  divider = 1023; code =  cv::COLOR_BayerGB2RGB; break;
        case V4L2_PIX_FMT_SGRBG10: type = CV_16UC1; debayer = true;  divider = 1023; code =  cv::COLOR_BayerGR2RGB; break;
        case V4L2_PIX_FMT_SBGGR10: type = CV_16UC1; debayer = true;  divider = 1023; code =  cv::COLOR_BayerBG2RGB; break;
        case V4L2_PIX_FMT_Y12:     type = CV_16UC1; debayer = false; divider = 4095; break;
        case V4L2_PIX_FMT_SRGGB12: type = CV_16UC1; debayer = true;  divider = 4095; code =  cv::COLOR_BayerRG2RGB; break;
        case V4L2_PIX_FMT_SGBRG12: type = CV_16UC1; debayer = true;  divider = 4095; code =  cv::COLOR_BayerGB2RGB; break;
        case V4L2_PIX_FMT_SGRBG12: type = CV_16UC1; debayer = true;  divider = 4095; code =  cv::COLOR_BayerGR2RGB; break;
        case V4L2_PIX_FMT_SBGGR12: type = CV_16UC1; debayer = true;  divider = 4095; code =  cv::COLOR_BayerBG2RGB; break;
        case V4L2_PIX_FMT_YUYV:    type =  CV_8UC2; debayer = true;  divider =    1; code = cv::COLOR_YUV2BGR_YUY2; break;
        case V4L2_PIX_FMT_SRGGB10P: type = CV_16UC1; debayer = true;  divider = 1023; code =  cv::COLOR_BayerRG2RGB; break;
        }

        if(image.pixelformat() == V4L2_PIX_FMT_SRGGB10P || 
                image.pixelformat() == V4L2_PIX_FMT_SBGGR10P || 
                image.pixelformat() == V4L2_PIX_FMT_SGBRG10P) {
            return convert_bayer10p_to_rgb((uint8_t *)image.planes()[0], image.width(), image.height(),0);
        }

        cv::Mat imageRAW8(image.height(), image.width(), type, cv::Scalar(200, 0, 0));
        int imageSize = image.bytesPerLine() * image.height();
        if (image.imageSize() < imageSize) {
                imageSize = image.imageSize();
        }
        memcpy(imageRAW8.data, (char *)image.planes()[0], imageSize);

        if (divider > 1) {
                imageRAW8.convertTo(imageRAW8, CV_8UC1, 255.0/divider/(1 << image.shift()));
        }

        cv::Mat imageResult;
        if (!raw && debayer) {
                cv::Mat imageBGR(image.height(), image.width(), CV_8UC3);
                cvtColor(imageRAW8, imageBGR, code);
                imageResult = imageBGR;

        } else {
                imageResult = imageRAW8;
        }

        return imageResult;
}

cv::Mat convert_bayer10p_to_rgb(const uint8_t *data, int width, int height, int blackcols)
{
    // Create a 16-bit Mat from the 10-bit data
    cv::Mat rgb;
    cv::Mat raw(height, width, CV_8UC1);

    auto pointer = data;


    for (int i = 0; i < height; ++i)
    {
        for (int j = 0; j < width; j += 4)
        {
            const uint8_t *local_pointer = pointer + ((i * width + j) * 5) / 4 + i * blackcols;

            raw.at<uint8_t>(i, j + 0) = static_cast<uint8_t>(*local_pointer);
            raw.at<uint8_t>(i, j + 1) = static_cast<uint8_t>(*(local_pointer + 1));
            raw.at<uint8_t>(i, j + 2) = static_cast<uint8_t>(*(local_pointer + 2));
            raw.at<uint8_t>(i, j + 3) = static_cast<uint8_t>(*(local_pointer + 3));

        }
    }
    cv::cvtColor(raw, rgb, cv::COLOR_BayerBG2BGR);

    // Convert the Bayer image to RGB

    return rgb;
}

QImage cvMatToQImage(const cv::Mat &mat) {
        if(mat.type() == CV_8UC3) {
            // OpenCV speichert Farben als BGR, QImage erwartet RGB
            QImage image(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_RGB888);
            return image.rgbSwapped();
    
        } else if(mat.type() == CV_8UC1) {
            return QImage(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_Grayscale8);
        
        } else {
            qDebug() << "Unsupported cv::Mat format";
            return QImage();
        }
}