#include "convert.hpp"
#include <linux/videodev2.h>


void unpack10ToRAW8(cv::Mat &unpackedRAW8, const Image &image)
{
    const uint8_t *packedData = (uint8_t *)image.planes()[0];
    uint8_t *unpackedData = (uint8_t *)unpackedRAW8.data;
    
    for (int y = 0; y < image.height(); y++) {
        uint32_t packedX = 0 ;
        uint32_t packedY =  y * image.bytesPerLine();
        uint32_t unpackedX = 0;
        uint32_t unpackedY =  y * image.width();
        while(packedX < image.bytesPerLine() && unpackedX < image.width()) {
            uint32_t packedIndex = packedY + packedX;
            uint32_t unpackedIndex = unpackedY + unpackedX;

            *(uint32_t *)(unpackedData + unpackedIndex) 
                = *(uint32_t *)(packedData + packedIndex);

            packedX += 5;
            unpackedX += 4;
        }              
    }
}

void unpack12ToRAW8(cv::Mat &unpackedRAW8, const Image &image)
{
    const uint8_t *packedData = (uint8_t *)image.planes()[0];
    uint8_t *unpackedData = (uint8_t *)unpackedRAW8.data;
    
    for (int y = 0; y < image.height(); y++) {
        uint32_t packedX = 0 ;
        uint32_t packedY =  y * image.bytesPerLine();
        uint32_t unpackedX = 0;
        uint32_t unpackedY =  y * image.width();
        while(packedX < image.bytesPerLine() && unpackedX < image.width()) {
            uint32_t packedIndex = packedY + packedX;
            uint32_t unpackedIndex = unpackedY + unpackedX;

            *(uint16_t *)(unpackedData + unpackedIndex) 
                = *(uint16_t *)(packedData + packedIndex);

            packedX += 3;
            unpackedX += 2;
        }              
    }
}

cv::Mat convert(const Image &image, bool raw)
{
    bool packed10bit = false;
    bool packed12bit = false;
    int type = CV_8UC1;
    bool debayer = false;
    int divider = 1;
    int code = 0;

    switch (image.pixelformat()) {
    case V4L2_PIX_FMT_GREY:     type =  CV_8UC1; debayer = false; divider =    1; break;
    case V4L2_PIX_FMT_SRGGB8:   type =  CV_8UC1; debayer = true;  divider =    1; code =  cv::COLOR_BayerBG2RGB; break;
    case V4L2_PIX_FMT_SGBRG8:   type =  CV_8UC1; debayer = true;  divider =    1; code =  cv::COLOR_BayerGB2RGB; break;
    case V4L2_PIX_FMT_SGRBG8:   type =  CV_8UC1; debayer = true;  divider =    1; code =  cv::COLOR_BayerGR2RGB; break;
    case V4L2_PIX_FMT_SBGGR8:   type =  CV_8UC1; debayer = true;  divider =    1; code =  cv::COLOR_BayerBG2RGB; break;
    case V4L2_PIX_FMT_Y10:      type = CV_16UC1; debayer = false; divider = 1023; break;
    case V4L2_PIX_FMT_Y10P:     type =  CV_8UC1; debayer = false; packed10bit = true; break;
    case V4L2_PIX_FMT_SRGGB10:  type = CV_16UC1; debayer = true;  divider = 1023; code =  cv::COLOR_BayerRG2RGB; break;
    case V4L2_PIX_FMT_SGBRG10:  type = CV_16UC1; debayer = true;  divider = 1023; code =  cv::COLOR_BayerGB2RGB; break;
    case V4L2_PIX_FMT_SGRBG10:  type = CV_16UC1; debayer = true;  divider = 1023; code =  cv::COLOR_BayerGR2RGB; break;
    case V4L2_PIX_FMT_SBGGR10:  type = CV_16UC1; debayer = true;  divider = 1023; code =  cv::COLOR_BayerBG2RGB; break;
    case V4L2_PIX_FMT_SRGGB10P: type =  CV_8UC1; debayer = true;  code =  cv::COLOR_BayerRG2RGB; packed10bit = true; break;
    case V4L2_PIX_FMT_SGBRG10P: type =  CV_8UC1; debayer = true;  code =  cv::COLOR_BayerGB2RGB; packed10bit = true; break;
    case V4L2_PIX_FMT_SGRBG10P: type =  CV_8UC1; debayer = true;  code =  cv::COLOR_BayerGR2RGB; packed10bit = true; break;
    case V4L2_PIX_FMT_SBGGR10P: type =  CV_8UC1; debayer = true;  code =  cv::COLOR_BayerBG2RGB; packed10bit = true; break;
    case V4L2_PIX_FMT_Y12:      type = CV_16UC1; debayer = false; divider = 4095; break;
    case V4L2_PIX_FMT_Y12P:     type =  CV_8UC1; debayer = false; packed12bit = true; break;
    case V4L2_PIX_FMT_SRGGB12:  type = CV_16UC1; debayer = true;  divider = 4095; code =  cv::COLOR_BayerRG2RGB; break;
    case V4L2_PIX_FMT_SGBRG12:  type = CV_16UC1; debayer = true;  divider = 4095; code =  cv::COLOR_BayerGB2RGB; break;
    case V4L2_PIX_FMT_SGRBG12:  type = CV_16UC1; debayer = true;  divider = 4095; code =  cv::COLOR_BayerGR2RGB; break;
    case V4L2_PIX_FMT_SBGGR12:  type = CV_16UC1; debayer = true;  divider = 4095; code =  cv::COLOR_BayerBG2RGB; break;
    case V4L2_PIX_FMT_SRGGB12P: type =  CV_8UC1; debayer = true;  code =  cv::COLOR_BayerRG2RGB; packed12bit = true; break;
    case V4L2_PIX_FMT_SGBRG12P: type =  CV_8UC1; debayer = true;  code =  cv::COLOR_BayerGB2RGB; packed12bit = true; break;
    case V4L2_PIX_FMT_SGRBG12P: type =  CV_8UC1; debayer = true;  code =  cv::COLOR_BayerGR2RGB; packed12bit = true; break;
    case V4L2_PIX_FMT_SBGGR12P: type =  CV_8UC1; debayer = true;  code =  cv::COLOR_BayerBG2RGB; packed12bit = true; break;
    case V4L2_PIX_FMT_YUYV:     type =  CV_8UC2; debayer = true;  divider =    1; code = cv::COLOR_YUV2BGR_YUY2; break;
    default: 
        qDebug() << "Unsupported pixel format!";
        return cv::Mat();
    }

    cv::Mat imageRAW8(image.height(), image.width(), type, cv::Scalar(200, 0, 0));
    if (packed10bit) {
        unpack10ToRAW8(imageRAW8, image);

    } else if (packed12bit) {
        unpack12ToRAW8(imageRAW8, image);

    } else {
        int imageSize = image.bytesPerLine() * image.height();
        if (image.imageSize() < imageSize) {
                imageSize = image.imageSize();
        }
        memcpy(imageRAW8.data, (char *)image.planes()[0], imageSize);
    
        if (divider > 1) {
                imageRAW8.convertTo(imageRAW8, CV_8UC1, 255.0/divider/(1 << image.shift()));
        }
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


QImage cvMatToQImage(const cv::Mat &mat) 
{
    if(mat.type() == CV_8UC3) {
        // OpenCV manages colors as BGR, QImage expects RGB
        QImage image(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_RGB888);
        return image.rgbSwapped();

    } else if(mat.type() == CV_8UC1) {
        return QImage(mat.data, mat.cols, mat.rows, static_cast<int>(mat.step), QImage::Format_Grayscale8);
    
    } else {
        qDebug() << "Unsupported cv::Mat format!";
        return QImage();
    }
}