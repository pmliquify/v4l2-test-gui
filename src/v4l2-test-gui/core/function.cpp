#include "function.hpp"

Function::Function(const QString &typeId) :
    m_typeId(typeId),
    m_position(10, 10),
    m_widget(nullptr)
{
}

QString Function::typeId() const
{
    return m_typeId;
}

QPoint Function::position() const
{
    return m_position;
}

void Function::setPosition(const QPoint &position)
{
    m_position = position;
}

bool Function::operator==(const Function &other) const
{
    return m_typeId == other.m_typeId && 
           m_position == other.m_position;
}

bool Function::operator!=(const Function &other) const
{
    return !(*this == other);
}
