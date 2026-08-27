// Copyright (c) 2026 Peter Martienssen
// SPDX-License-Identifier: MIT

#include "histogram.hpp"
#include "functionplugin.hpp"
#include <opencv2/opencv.hpp>


// ===============================================================================================
// HistogramPlugin implementation

class HistogramPlugin : public QObject, public FunctionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID FunctionPlugin_iid)
    Q_INTERFACES(FunctionPlugin)
    
public:
    QString id() const override { return "histogram"; }
    QString displayName() const override { return "Histogram"; }
    
    std::unique_ptr<Function> createInstance() const override
    {
        return std::make_unique<Histogram>();
    }
};


// ===============================================================================================
// HistogramAdapter implementation

class HistogramAdapter : public PropertyAdapter 
{
public:
    HistogramAdapter(Histogram *histogram) : m_histogram(histogram) {}

    QString sectionName() const override { return "Histogram"; }

    QList<PropertyInfo> availableProperties() const override
    {
        return {
            {"ShowRGBChannels", QMetaType::Bool, 0, 1, 1, ""}
        };
    }

    QVariant getValue(const QString &name) const override
    {
        if (name == "ShowRGBChannels") {
            return m_histogram->showRGBChannels();
        }
        return QVariant();
    }

    void setValue(const QString &name, const QVariant &value) override
    {
        if (name == "ShowRGBChannels") {
            m_histogram->setShowRGBChannels(value.toBool());
        }
    }

private:
    Histogram* m_histogram;
};


// ===============================================================================================
// Histogram implementation

Histogram::Histogram() :
    Function("histogram"),
    m_propertyAdapter(new HistogramAdapter(this)),
    m_showRGBChannels(true),
    m_redHist(HISTOGRAM_BINS, 0),
    m_greenHist(HISTOGRAM_BINS, 0),
    m_blueHist(HISTOGRAM_BINS, 0),
    m_luminanceHist(HISTOGRAM_BINS, 0)
{
}

Histogram::Histogram(const QPoint &position) :
    Function("histogram"),
    m_propertyAdapter(new HistogramAdapter(this)),
    m_showRGBChannels(true),
    m_redHist(HISTOGRAM_BINS, 0),
    m_greenHist(HISTOGRAM_BINS, 0),
    m_blueHist(HISTOGRAM_BINS, 0),
    m_luminanceHist(HISTOGRAM_BINS, 0)
{
    setPosition(position);
}

Histogram::~Histogram()
{
    delete m_propertyAdapter;
}

PropertyAdapter* Histogram::propertyAdapter() const
{
    return m_propertyAdapter;
}

bool Histogram::showRGBChannels() const
{
    return m_showRGBChannels;
}

void Histogram::setShowRGBChannels(bool show)
{
    m_showRGBChannels = show;
}

void Histogram::calculate(const cv::Mat &image, const cv::Rect &roi)
{
    if (image.empty() || roi.empty()) {
        m_redHist.fill(0);
        m_greenHist.fill(0);
        m_blueHist.fill(0);
        m_luminanceHist.fill(0);
        return;
    }
    
    
    // Ensure ROI is within image bounds
    cv::Rect cvRoi= roi;
    cvRoi &= cv::Rect(0, 0, image.cols, image.rows);
    
    if (cvRoi.width <= 0 || cvRoi.height <= 0) {
        m_redHist.fill(0);
        m_greenHist.fill(0);
        m_blueHist.fill(0);
        m_luminanceHist.fill(0);
        return;
    }
    
    cv::Mat roiImage = image(cvRoi);
    
    if (m_showRGBChannels) {
        // Calculate RGB histograms separately
        std::vector<cv::Mat> channels;
        cv::split(roiImage, channels);
        
        int histSize = HISTOGRAM_BINS;
        float range[] = {0, 256};
        const float* histRange = {range};
        
        cv::Mat blueHist, greenHist, redHist;
        cv::calcHist(&channels[0], 1, 0, cv::Mat(), blueHist, 1, &histSize, &histRange);
        cv::calcHist(&channels[1], 1, 0, cv::Mat(), greenHist, 1, &histSize, &histRange);
        cv::calcHist(&channels[2], 1, 0, cv::Mat(), redHist, 1, &histSize, &histRange);

        m_redHist.resize(HISTOGRAM_BINS);
        m_greenHist.resize(HISTOGRAM_BINS);
        m_blueHist.resize(HISTOGRAM_BINS);
        
        for (int i = 0; i < HISTOGRAM_BINS; ++i) {
            m_redHist[i] = static_cast<int>(redHist.at<float>(i));
            m_greenHist[i] = static_cast<int>(greenHist.at<float>(i));
            m_blueHist[i] = static_cast<int>(blueHist.at<float>(i));
        }
    } else {
        // Calculate luminance histogram
        cv::Mat grayImage;
        cv::cvtColor(roiImage, grayImage, cv::COLOR_BGR2GRAY);
        
        int histSize = HISTOGRAM_BINS;
        float range[] = {0, 256};
        const float* histRange = {range};
        
        cv::Mat luminanceHist;
        cv::calcHist(&grayImage, 1, 0, cv::Mat(), luminanceHist, 1, &histSize, &histRange);
        
        m_luminanceHist.resize(HISTOGRAM_BINS);
        for (int i = 0; i < HISTOGRAM_BINS; ++i) {
            m_luminanceHist[i] = static_cast<int>(luminanceHist.at<float>(i));
        }
    }
}

void Histogram::draw(QPainter &painter, double scaleFactor, const QPoint &imageOffset) const
{
    // Save painter state to avoid interference with other drawing operations
    painter.save();
    
    // Draw black semi-transparent background
    QRect histogramRect(position(), QSize(HISTOGRAM_WIDTH, HISTOGRAM_HEIGHT));
    painter.fillRect(histogramRect, QColor(0, 0, 0, 100));

    if (m_showRGBChannels) {
        // Draw RGB histogram channels
        drawHistogramChannel(painter, m_blueHist, QColor(0, 0, 255), 0);
        drawHistogramChannel(painter, m_greenHist, QColor(0, 255, 0), 0);
        drawHistogramChannel(painter, m_redHist, QColor(255, 0, 0), 0);
        
    } else {
        // Draw luminance histogram
        drawHistogramChannel(painter, m_luminanceHist, QColor(200, 200, 200), 0);
    }

    // Draw title with RGB mean values
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    painter.drawText(position() + QPoint(5, 12), getMeanValueString());
    
    // Draw frame
    painter.setPen(Qt::white);
    painter.drawRect(histogramRect);

    // Draw X-axis with labels (only 0 and 1.0)
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 10, QFont::Bold));
    
    QRect histRect(position(), QSize(HISTOGRAM_WIDTH, HISTOGRAM_HEIGHT));
    int xAxisY = histRect.bottom() + 12;
    
    painter.drawText(histRect.left() + 2, xAxisY, "0.0");
    painter.drawText(histRect.right() - 15, xAxisY, "1.0");
    
    // Restore painter state
    painter.restore();
}

bool Histogram::containsPoint(const QPoint &point) const
{
    QRect histogramRect(position(), QSize(HISTOGRAM_WIDTH, HISTOGRAM_HEIGHT + 35)); // +35 for title and x-axis
    return histogramRect.contains(point);
}

double Histogram::getLuminanceMean() const
{
    return calculateMean(m_luminanceHist);
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
    if (m_showRGBChannels) {
        double redMean = getRedMean() / 255.0;
        double greenMean = getGreenMean() / 255.0;
        double blueMean = getBlueMean() / 255.0;
        
        return QString("RGB Histo (R: %1, G: %2, B: %3)")
            .arg(QString::number(redMean, 'f', 3))
            .arg(QString::number(greenMean, 'f', 3))
            .arg(QString::number(blueMean, 'f', 3));
    
    } else {
        double luminanceMean = getLuminanceMean() / 255.0;
        return QString("Luminance Histo (%1)")
            .arg(QString::number(luminanceMean, 'f', 3));
    }
}

QJsonObject Histogram::toJson() const
{
    QJsonObject json;
    json["type"] = "histogram";
    json["position_x"] = position().x();
    json["position_y"] = position().y();
    json["showRGBChannels"] = m_showRGBChannels;
    return json;
}

void Histogram::fromJson(const QJsonObject &json)
{
    int x = json["position_x"].toInt(10);
    int y = json["position_y"].toInt(10);
    setPosition(QPoint(x, y));
    m_showRGBChannels = json["showRGBChannels"].toBool(false);
}

std::unique_ptr<Function> Histogram::clone() const
{
    auto copy = std::make_unique<Histogram>();
    copy->setPosition(position());
    copy->m_redHist = m_redHist;
    copy->m_greenHist = m_greenHist;
    copy->m_blueHist = m_blueHist;
    copy->m_luminanceHist = m_luminanceHist;
    copy->m_showRGBChannels = m_showRGBChannels;
    return copy;
}

bool Histogram::operator==(const Histogram &other) const
{
    return Function::operator==(other) && 
           m_redHist == other.m_redHist &&
           m_greenHist == other.m_greenHist &&
           m_blueHist == other.m_blueHist &&
           m_luminanceHist == other.m_luminanceHist &&
           m_showRGBChannels == other.m_showRGBChannels;
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
    painter.setBrush(Qt::NoBrush);
    
    QRect histRect(position(), QSize(HISTOGRAM_WIDTH, HISTOGRAM_HEIGHT));
    
    for (int i = 0; i < hist.size(); ++i) {
        int barHeight = static_cast<int>((static_cast<double>(hist[i]) / maxValue) * HISTOGRAM_HEIGHT);
        if (barHeight > 0) {
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

// Include MOC file for plugin metadata
#include "histogram.moc"
