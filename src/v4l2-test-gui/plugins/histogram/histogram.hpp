#pragma once

#include <memory>
#include "function.hpp"

class HistogramAdapter;

class Histogram : public Function
{
public:
    Histogram();
    Histogram(const QPoint &position);
    ~Histogram();

    QString title() const override { return "Histogram"; }
    PropertyAdapter* propertyAdapter() const override;
    
    // Function interface implementation
    void calculate(const cv::Mat &image, const cv::Rect &roi) override;
    void draw(QPainter &painter, double scaleFactor, const QPoint &imageOffset) const override;
    bool containsPoint(const QPoint &point) const override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &json) override;
    std::unique_ptr<Function> clone() const override;
    
    // Statistics
    bool showRGBChannels() const;
    void setShowRGBChannels(bool show);
    double getRedMean() const;
    double getGreenMean() const;
    double getBlueMean() const;
    double getLuminanceMean() const;
    QString getMeanValueString() const;
    
    // Operators
    bool operator==(const Histogram &other) const;
    bool operator!=(const Histogram &other) const;

private:
    HistogramAdapter* m_propertyAdapter;
    
    bool         m_showRGBChannels;
    QVector<int> m_redHist;         // Red channel histogram (256 values)
    QVector<int> m_greenHist;       // Green channel histogram (256 values)
    QVector<int> m_blueHist;        // Blue channel histogram (256 values)
    QVector<int> m_luminanceHist;   // Luminance histogram (256 values)
    
    // Constants
    static const int HISTOGRAM_WIDTH = 256;
    static const int HISTOGRAM_HEIGHT = 100;
    static const int HISTOGRAM_BINS = 256;
    
    void drawHistogramChannel(QPainter &painter, const QVector<int> &hist, const QColor &color, int yOffset) const;
    double calculateMean(const QVector<int> &histogram) const;
};
