#include "histogram.hpp"
#include <QPainter>
#include <QJsonObject>
#include <opencv2/opencv.hpp>

Histogram::Histogram()
    : Function(FunctionType::Histogram),
      m_redHist(HISTOGRAM_BINS, 0),
      m_greenHist(HISTOGRAM_BINS, 0),
      m_blueHist(HISTOGRAM_BINS, 0)
{
}

Histogram::Histogram(const QPoint &position)
    : Function(FunctionType::Histogram),
      m_redHist(HISTOGRAM_BINS, 0),
      m_greenHist(HISTOGRAM_BINS, 0),
      m_blueHist(HISTOGRAM_BINS, 0)
{
    setPosition(position);
}

QVector<int> Histogram::redHistogram() const
{
    return m_redHist;
}

QVector<int> Histogram::greenHistogram() const
{
    return m_greenHist;
}

QVector<int> Histogram::blueHistogram() const
{
    return m_blueHist;
}

void Histogram::setHistogramData(const QVector<int> &red, const QVector<int> &green, const QVector<int> &blue)
{
    m_redHist = red;
    m_greenHist = green;
    m_blueHist = blue;
}

void Histogram::calculate(const QImage &image, const QRect &roi)
{
    if (image.isNull() || roi.isEmpty()) {
        m_redHist.fill(0);
        m_greenHist.fill(0);
        m_blueHist.fill(0);
        return;
    }
    
    // Convert QImage to cv::Mat for histogram calculation
    cv::Mat cvImage;
    if (image.format() == QImage::Format_RGB32 || image.format() == QImage::Format_ARGB32) {
        cvImage = cv::Mat(image.height(), image.width(), CV_8UC4, (void*)image.constBits(), image.bytesPerLine());
        cv::cvtColor(cvImage, cvImage, cv::COLOR_BGRA2BGR);
    } else if (image.format() == QImage::Format_RGB888) {
        cvImage = cv::Mat(image.height(), image.width(), CV_8UC3, (void*)image.constBits(), image.bytesPerLine());
        cv::cvtColor(cvImage, cvImage, cv::COLOR_RGB2BGR);
    } else {
        QImage rgbImage = image.convertToFormat(QImage::Format_RGB888);
        cvImage = cv::Mat(rgbImage.height(), rgbImage.width(), CV_8UC3, (void*)rgbImage.constBits(), rgbImage.bytesPerLine());
        cv::cvtColor(cvImage, cvImage, cv::COLOR_RGB2BGR);
    }
    
    // Ensure ROI is within image bounds
    cv::Rect cvRoi(roi.x(), roi.y(), roi.width(), roi.height());
    cvRoi &= cv::Rect(0, 0, cvImage.cols, cvImage.rows);
    
    if (cvRoi.width <= 0 || cvRoi.height <= 0) {
        m_redHist.fill(0);
        m_greenHist.fill(0);
        m_blueHist.fill(0);
        return;
    }
    
    cv::Mat roiImage = cvImage(cvRoi);
    
    // Split channels
    std::vector<cv::Mat> channels;
    cv::split(roiImage, channels);
    
    // Calculate histograms
    int histSize = HISTOGRAM_BINS;
    float range[] = {0, 256};
    const float* histRange = {range};
    
    cv::Mat blueHist, greenHist, redHist;
    cv::calcHist(&channels[0], 1, 0, cv::Mat(), blueHist, 1, &histSize, &histRange);
    cv::calcHist(&channels[1], 1, 0, cv::Mat(), greenHist, 1, &histSize, &histRange);
    cv::calcHist(&channels[2], 1, 0, cv::Mat(), redHist, 1, &histSize, &histRange);
    
    // Convert to QVector
    m_redHist.resize(HISTOGRAM_BINS);
    m_greenHist.resize(HISTOGRAM_BINS);
    m_blueHist.resize(HISTOGRAM_BINS);
    
    for (int i = 0; i < HISTOGRAM_BINS; ++i) {
        m_redHist[i] = static_cast<int>(redHist.at<float>(i));
        m_greenHist[i] = static_cast<int>(greenHist.at<float>(i));
        m_blueHist[i] = static_cast<int>(blueHist.at<float>(i));
    }
}

void Histogram::draw(QPainter &painter) const
{
    if (!isVisible()) {
        return;
    }
    
    // Draw black semi-transparent background
    QRect histogramRect(position(), QSize(HISTOGRAM_WIDTH, HISTOGRAM_HEIGHT));
    painter.fillRect(histogramRect, QColor(0, 0, 0, 100));

    // Draw histogram channels
    drawHistogramChannel(painter, m_blueHist, QColor(0, 0, 255), 0);
    drawHistogramChannel(painter, m_greenHist, QColor(0, 255, 0), 0);
    drawHistogramChannel(painter, m_redHist, QColor(255, 0, 0), 0);
    
    // Draw title with mean values
    QString title = QString("RGB Histo %1").arg(getMeanValueString());
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(8);
    painter.setFont(font);
    painter.drawText(position() + QPoint(5, 12), title);
    
    // Draw frame
    painter.setPen(Qt::white);
    painter.drawRect(histogramRect);

    // Draw X-axis with labels (only 0 and 256)
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 7));
    
    // X-axis line
    QRect histRect(position(), QSize(HISTOGRAM_WIDTH, HISTOGRAM_HEIGHT));
    int xAxisY = histRect.bottom() + 12;
    
    // X-axis labels - only 0 and 256, positioned correctly
    painter.drawText(histRect.left() + 2, xAxisY, "0");       // Right of left edge
    painter.drawText(histRect.right() - 15, xAxisY, "256");   // Left of right edge
}

bool Histogram::containsPoint(const QPoint &point) const
{
    if (!isVisible()) {
        return false;
    }
    
    QRect histogramRect(position(), QSize(HISTOGRAM_WIDTH, HISTOGRAM_HEIGHT + 35)); // +35 for title and x-axis
    return histogramRect.contains(point);
}

double Histogram::getRedMean() const
{
    return calculateMean(m_redHist);
}

double Histogram::getGreenMean() const
{
    return calculateMean(m_greenHist);
}

double Histogram::getBlueMean() const
{
    return calculateMean(m_blueHist);
}

QString Histogram::getMeanValueString() const
{
    double redMean = getRedMean();
    double greenMean = getGreenMean();
    double blueMean = getBlueMean();
    
    return QString("(R:%1, G:%2, B:%3)")
           .arg(QString::number(redMean, 'f', 0))
           .arg(QString::number(greenMean, 'f', 0))
           .arg(QString::number(blueMean, 'f', 0));
}

QJsonObject Histogram::toJson() const
{
    QJsonObject json;
    json["type"] = "histogram";
    json["position_x"] = position().x();
    json["position_y"] = position().y();
    json["visible"] = isVisible();
    return json;
}

void Histogram::fromJson(const QJsonObject &json)
{
    int x = json["position_x"].toInt(10);
    int y = json["position_y"].toInt(10);
    setPosition(QPoint(x, y));
    setVisible(json["visible"].toBool(true));
}

std::unique_ptr<Function> Histogram::clone() const
{
    auto copy = std::make_unique<Histogram>();
    copy->setPosition(position());
    copy->setVisible(isVisible());
    copy->m_redHist = m_redHist;
    copy->m_greenHist = m_greenHist;
    copy->m_blueHist = m_blueHist;
    return copy;
}

bool Histogram::operator==(const Histogram &other) const
{
    return Function::operator==(other) && 
           m_redHist == other.m_redHist &&
           m_greenHist == other.m_greenHist &&
           m_blueHist == other.m_blueHist;
}

bool Histogram::operator!=(const Histogram &other) const
{
    return !(*this == other);
}

void Histogram::drawHistogramChannel(QPainter &painter, const QVector<int> &hist, const QColor &color, int yOffset) const
{
    if (hist.isEmpty()) {
        return;
    }
    
    // Find maximum value for scaling
    int maxValue = *std::max_element(hist.begin(), hist.end());
    if (maxValue == 0) {
        return;
    }
    
    painter.setPen(QPen(color, 1));
    painter.setBrush(Qt::NoBrush); // Kein gefüllter Brush - nur Linien
    
    QRect histRect(position(), QSize(HISTOGRAM_WIDTH, HISTOGRAM_HEIGHT));
    
    for (int i = 0; i < hist.size(); ++i) {
        int barHeight = static_cast<int>((static_cast<double>(hist[i]) / maxValue) * HISTOGRAM_HEIGHT);
        if (barHeight > 0) {
            // Zeichne nur eine vertikale Linie, kein gefülltes Rechteck
            painter.drawLine(histRect.left() + i, histRect.bottom() + yOffset,
                           histRect.left() + i, histRect.bottom() - barHeight + yOffset);
        }
    }
}

double Histogram::calculateMean(const QVector<int> &histogram) const
{
    if (histogram.isEmpty()) {
        return 0.0;
    }
    
    long long totalPixels = 0;
    long long weightedSum = 0;
    
    for (int i = 0; i < histogram.size(); ++i) {
        totalPixels += histogram[i];
        weightedSum += i * histogram[i];
    }
    
    return totalPixels > 0 ? static_cast<double>(weightedSum) / totalPixels : 0.0;
}
