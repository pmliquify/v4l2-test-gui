#include "function.hpp"
#include "histogram.hpp"

Function::Function(FunctionType type)
    : m_type(type),
      m_position(10, 10),
      m_visible(true)
{
}

FunctionType Function::type() const
{
    return m_type;
}

QPoint Function::position() const
{
    return m_position;
}

bool Function::isVisible() const
{
    return m_visible;
}

void Function::setPosition(const QPoint &position)
{
    m_position = position;
}

void Function::setVisible(bool visible)
{
    m_visible = visible;
}

bool Function::operator==(const Function &other) const
{
    return m_type == other.m_type && 
           m_position == other.m_position && 
           m_visible == other.m_visible;
}

bool Function::operator!=(const Function &other) const
{
    return !(*this == other);
}

std::unique_ptr<Function> createFunctionFromJson(const QJsonObject &json)
{
    QString typeStr = json["type"].toString();
    
    if (typeStr == "histogram") {
        auto histogram = std::make_unique<Histogram>();
        histogram->fromJson(json);
        return histogram;
    }
    
    // Return nullptr if unknown type
    return nullptr;
}
