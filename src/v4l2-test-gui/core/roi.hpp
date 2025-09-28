#pragma once

#include <QtCore>
#include <opencv2/opencv.hpp>
#include <memory>
#include <vector>
#include "propertyhost.hpp"
#include "function.hpp"

class RoiAdapter;

class Roi : public PropertyHost
{
public:
    Roi();
    Roi(const QRect &roiRect, int roiId);
    Roi(const cv::Rect &roiRect, int roiId);
    
    ~Roi();
    PropertyAdapter* propertyAdapter() const override;
    
    // Getters
    QRect qRect() const;
    cv::Rect rect() const;
    int id() const;
    int area() const;  // For sorting by size
    
    // Setters
    void setRoi(const QRect &roiRect);
    void setRoi(const cv::Rect &roiRect);
    void setId(int roiId);
    
    // Function management
    void addFunction(std::unique_ptr<Function> function);
    void removeFunction(const QString &typeId);
    bool hasFunction(const QString &typeId) const;
    Function* getFunction(const QString &typeId);
    const Function* getFunction(const QString &typeId) const;
    const std::vector<std::unique_ptr<Function>>& getFunctions() const;
       
    // Operations
    void calculateFunctions(const cv::Mat &image, const cv::Rect &roi);
    void drawFunctions(QPainter &painter, double scaleFactor, const QPoint &imageOffset) const;
    Function* findFunctionAt(const QPoint &point);
    const Function* findFunctionAt(const QPoint &point) const;
    
    // Serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);
    
    // Operators
    bool operator==(const Roi &other) const;
    bool operator!=(const Roi &other) const;

private:
    RoiAdapter* m_propertyAdapter;                        // Property adapter for this ROI
    cv::Rect m_roi;                                       // ROI in image coordinates (OpenCV format)
    int m_id;                                             // Unique identifier
    std::vector<std::unique_ptr<Function>> m_functions;   // List of functions
    
    // Helper functions for conversion
    static cv::Rect qRectToCvRect(const QRect &qRect);
    static QRect cvRectToQRect(const cv::Rect &cvRect);
};
