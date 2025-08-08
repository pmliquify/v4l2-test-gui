#pragma once

#include <QtCore>
#include <QtGui>
#include <QVector>
#include <memory>
#include "function.hpp"

class Histogram : public Function
{
public:
    Histogram();
    Histogram(const QPoint &position);
    
    // Function interface implementation
    void calculate(const QImage &image, const QRect &roi) override;
    void draw(QPainter &painter) const override;
    bool containsPoint(const QPoint &point) const override;
    QJsonObject toJson() const override;
    void fromJson(const QJsonObject &json) override;
    std::unique_ptr<Function> clone() const override;
    
    // Histogram-specific methods
    QVector<int> redHistogram() const;
    QVector<int> greenHistogram() const;
    QVector<int> blueHistogram() const;
    void setHistogramData(const QVector<int> &red, const QVector<int> &green, const QVector<int> &blue);
    
    // Statistics
    double getRedMean() const;
    double getGreenMean() const;
    double getBlueMean() const;
    QString getMeanValueString() const;
    
    // Operators
    bool operator==(const Histogram &other) const;
    bool operator!=(const Histogram &other) const;

private:
    QVector<int> m_redHist;      // Red channel histogram (256 values)
    QVector<int> m_greenHist;    // Green channel histogram (256 values)
    QVector<int> m_blueHist;     // Blue channel histogram (256 values)
    
    // Constants
    static const int HISTOGRAM_WIDTH = 256;
    static const int HISTOGRAM_HEIGHT = 100;
    static const int HISTOGRAM_BINS = 256;
    
    void drawHistogramChannel(QPainter &painter, const QVector<int> &hist, const QColor &color, int yOffset) const;
    double calculateMean(const QVector<int> &histogram) const;
};
