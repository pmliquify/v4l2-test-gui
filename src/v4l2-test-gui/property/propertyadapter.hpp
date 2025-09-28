#pragma once

#include <QtCore>


struct PropertyInfo 
{
    QString name;
    QMetaType::Type type;
    QVariant min;
    QVariant max;
    QVariant step;
    QString unit;
};

class PropertyAdapter 
{
public:
    virtual ~PropertyAdapter() = default;

    virtual QString sectionName() const = 0;
    virtual QList<PropertyInfo> availableProperties() const = 0;
    virtual QVariant getValue(const QString& name) const = 0;
    virtual void setValue(const QString& name, const QVariant& value) = 0;
};
