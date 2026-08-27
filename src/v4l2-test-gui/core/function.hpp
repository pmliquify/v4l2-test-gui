// Copyright (c) 2026 Peter Martienssen
// SPDX-License-Identifier: MIT

#pragma once

#include <QtGui>
#include <opencv2/opencv.hpp>
#include <memory>
#include "propertyhost.hpp"

class Function : public PropertyHost
{
public:
    Function(const QString &typeId);
    virtual ~Function() = default;
    
    // Widget management for functions that need UI components
    virtual QString title() const = 0;
    virtual QWidget* widget() { return nullptr; }
    
    // Pure virtual methods that must be implemented by derived classes
    virtual void calculate(const cv::Mat &image, const cv::Rect &roi) = 0;
    virtual void draw(QPainter &painter, double scaleFactor, const QPoint &imageOffset) const = 0;
    virtual bool containsPoint(const QPoint &point) const = 0;
    virtual QJsonObject toJson() const = 0;
    virtual void fromJson(const QJsonObject &json) = 0;
    virtual std::unique_ptr<Function> clone() const = 0;
    
    // Common properties
    QString typeId() const;
    QPoint position() const;
    void setPosition(const QPoint &position);
    
    // Operators
    virtual bool operator==(const Function &other) const;
    virtual bool operator!=(const Function &other) const;

protected:
    QString         m_typeId;
    QPoint          m_position;
    QWidget*        m_widget;
};
