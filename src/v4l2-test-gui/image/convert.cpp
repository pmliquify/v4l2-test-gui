#include "convert.hpp"
#include <linux/videodev2.h>


void unpack10ToRAW8(cv::Mat &unpackedRAW8, const Image &image)
{
    const uint8_t *packedData = (uint8_t *)image.plane(0).data();
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
    const uint8_t *packedData = (uint8_t *)image.plane(0).data();
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

void cvtColorYUV2BGR_NV12_BT601(const cv::Mat& imageY, const cv::Mat& imageUV, cv::Mat &imageBGR) 
{
    // qDebug() << "Using OpenCV NV12 BT.601 conversion";

    cv::cvtColorTwoPlane(imageY, imageUV, imageBGR, cv::COLOR_YUV2BGR_NV12);
}

void cvtColorYUV2BGR_NV12_BT601_Custom(const cv::Mat& imageY, const cv::Mat& imageUV, cv::Mat &imageBGR) 
{
    qDebug() << "Using custom NV12 BT.601 conversion";

    int height = imageY.rows;
    int width = imageY.cols;

    // Erstellen der BGR-Ausgabematrix
    imageBGR.create(height, width, CV_8UC3);

    // Iteration über jeden Pixel des Y-Kanals
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Lesen der Y-Werte
            uchar Y_val = imageY.at<uchar>(y, x);

            // Lesen der U- und V-Werte aus der UV-Ebene
            // Die UV-Ebene ist halbiert und interleaved (U, V, U, V...)
            // Die UV-Koordinaten müssen den Y-Koordinaten entsprechen
            int uv_row = y / 2;
            int uv_col_pair = x / 2;
            
            // Die UV-Ebene enthält verschachtelte Werte: U V U V ...
            uchar U_val = imageUV.at<cv::Vec2b>(uv_row, uv_col_pair)[0];
            uchar V_val = imageUV.at<cv::Vec2b>(uv_row, uv_col_pair)[1];

            // Anwenden der BT.601-Formeln (mit limited range offset)
            float C = static_cast<float>(Y_val) - 16.0f;
            float D = static_cast<float>(U_val) - 128.0f;
            float E = static_cast<float>(V_val) - 128.0f;

            // BT.601-Konvertierungsmatrix
            float R = 1.164f * C + 1.596f * E;
            float G = 1.164f * C - 0.392f * D - 0.813f * E;
            float B = 1.164f * C + 2.017f * D;

            // Sicherstellen, dass die Werte im gültigen Bereich 0-255 liegen
            imageBGR.at<cv::Vec3b>(y, x)[0] = cv::saturate_cast<uchar>(B);
            imageBGR.at<cv::Vec3b>(y, x)[1] = cv::saturate_cast<uchar>(G);
            imageBGR.at<cv::Vec3b>(y, x)[2] = cv::saturate_cast<uchar>(R);
        }
    }
}

void cvtColorYUV2BGR_NV12_BT709_Custom(const cv::Mat& imageY, const cv::Mat& imageUV, cv::Mat &imageBGR) 
{
    qDebug() << "Using custom NV12 BT.709 conversion";

    int height = imageY.rows;
    int width = imageY.cols;

    // Erstellen der BGR-Ausgabematrix
    imageBGR.create(height, width, CV_8UC3);

    // Iteration über jeden Pixel des Y-Kanals
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // Lesen der Y-Werte
            uchar Y_val = imageY.at<uchar>(y, x);

            // Lesen der U- und V-Werte aus der UV-Ebene
            // Die UV-Ebene ist halbiert und interleaved (U, V, U, V...)
            // Die UV-Koordinaten müssen den Y-Koordinaten entsprechen
            int uv_row = y / 2;
            int uv_col = x / 2;

            uchar U_val = imageUV.at<cv::Vec2b>(uv_row, uv_col)[0];
            uchar V_val = imageUV.at<cv::Vec2b>(uv_row, uv_col)[1];

            // Anwenden der BT.709-Formeln
            // Offset für U/V wird subtrahiert (128)
            float Y_norm = static_cast<float>(Y_val);
            float U_norm = static_cast<float>(U_val) - 128.0f;
            float V_norm = static_cast<float>(V_val) - 128.0f;

            // BT.709-Konvertierungsmatrix
            float R = Y_norm + 1.5748f * V_norm;
            float G = Y_norm - 0.1873f * U_norm - 0.4681f * V_norm;
            float B = Y_norm + 1.8556f * U_norm;

            // Sicherstellen, dass die Werte im gültigen Bereich 0-255 liegen
            imageBGR.at<cv::Vec3b>(y, x)[0] = cv::saturate_cast<uchar>(B);
            imageBGR.at<cv::Vec3b>(y, x)[1] = cv::saturate_cast<uchar>(G);
            imageBGR.at<cv::Vec3b>(y, x)[2] = cv::saturate_cast<uchar>(R);
        }
    }
}

cv::Mat convert(const Image &image, int strideOffset, bool raw)
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
    case V4L2_PIX_FMT_NV12:     break;
    default: 
        qDebug() << "Unsupported pixel format!";
        return cv::Mat();
    }

    if (image.pixelformat() == V4L2_PIX_FMT_NV12) {
        cv::Mat imageY (image.height(),   image.width(), CV_8UC1, (char *)image.plane(0).data());
        cv::Mat imageUV(image.height()/2, image.width()/2, CV_8UC2, (char *)image.plane(1).data());
        cv::Mat imageBGR;
        cvtColorYUV2BGR_NV12_BT601(imageY, imageUV, imageBGR);
        // cvtColorYUV2BGR_NV12_BT601_Custom(imageY, imageUV, imageBGR);
        // cvtColorYUV2BGR_NV12_BT709_Custom(imageY, imageUV, imageBGR);
        return imageBGR;
    }

    cv::Mat imageRAW8(image.height(), image.width() + strideOffset, type, cv::Scalar(200, 0, 0));
    if (packed10bit) {
        unpack10ToRAW8(imageRAW8, image);

    } else if (packed12bit) {
        unpack12ToRAW8(imageRAW8, image);

    } else {
        int imageSize = image.bytesPerLine() * image.height();
        if (image.size() < imageSize) {
                imageSize = image.size();
        }
        memcpy(imageRAW8.data, (char *)image.plane(0).data(), imageSize);

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
            cvtColor(imageRAW8, imageResult, cv::COLOR_GRAY2BGR);
    }

    return imageResult;
}
