#pragma once

#include <QtCore>
#include <memory>
#include <vector>
#include "function.hpp"

class Histogram; // Forward declaration

class Roi
{
public:
    Roi();
    Roi(const QRect &roiRect, int roiId);
    
    // Copy constructor and assignment operator
    Roi(const Roi &other);
    Roi& operator=(const Roi &other);
    
    // Move constructor and assignment operator
    Roi(Roi&&) = default;
    Roi& operator=(Roi&&) = default;
    
    // Getters
    QRect rect() const;
    int id() const;
    
    // Setters
    void setRoi(const QRect &roiRect);
    void setId(int roiId);
    
    // Function management
    void addFunction(std::unique_ptr<Function> function);
    void removeFunction(FunctionType type);
    bool hasFunction(FunctionType type) const;
    Function* getFunction(FunctionType type);
    const Function* getFunction(FunctionType type) const;
    const std::vector<std::unique_ptr<Function>>& getFunctions() const;
    
    // Convenience methods for histogram (backward compatibility)
    bool hasHistogram() const;
    Histogram* getHistogram();
    const Histogram* getHistogram() const;
    void enableHistogram(const QPoint &position = QPoint(10, 10));
    void disableHistogram();
    QPoint histogramPos() const;
    void setHistogramPos(const QPoint &position);
    
    // Operations
    void calculateFunctions(const QImage &image, const QRect &roi);
    void drawFunctions(QPainter &painter) const;
    Function* findFunctionAt(const QPoint &point);
    const Function* findFunctionAt(const QPoint &point) const;
    
    // Serialization
    QJsonObject toJson() const;
    void fromJson(const QJsonObject &json);
    
    // Operators
    bool operator==(const Roi &other) const;
    bool operator!=(const Roi &other) const;

private:
    QRect m_roi;                                          // ROI in image coordinates
    int m_id;                                             // Unique identifier
    std::vector<std::unique_ptr<Function>> m_functions;   // List of functions
};
