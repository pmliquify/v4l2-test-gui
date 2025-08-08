#pragma once

#include <QtCore>
#include <QtGui>
#include <memory>

enum class FunctionType {
    Histogram
    // Future functions can be added here (e.g., Statistics, Measurement, etc.)
};

class Function
{
public:
    Function(FunctionType type);
    virtual ~Function() = default;
    
    // Pure virtual methods that must be implemented by derived classes
    virtual void calculate(const QImage &image, const QRect &roi) = 0;
    virtual void draw(QPainter &painter) const = 0;
    virtual bool containsPoint(const QPoint &point) const = 0;
    virtual QJsonObject toJson() const = 0;
    virtual void fromJson(const QJsonObject &json) = 0;
    virtual std::unique_ptr<Function> clone() const = 0;
    
    // Common properties
    FunctionType type() const;
    QPoint position() const;
    bool isVisible() const;
    
    void setPosition(const QPoint &position);
    void setVisible(bool visible);
    
    // Operators
    virtual bool operator==(const Function &other) const;
    virtual bool operator!=(const Function &other) const;

protected:
    FunctionType m_type;
    QPoint m_position;
    bool m_visible;
};

// Factory function to create functions from JSON
std::unique_ptr<Function> createFunctionFromJson(const QJsonObject &json);
