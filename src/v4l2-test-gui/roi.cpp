#include "roi.hpp"
#include "histogram.hpp"
#include <QJsonObject>
#include <QJsonArray>
#include <QPainter>

Roi::Roi()
    : m_id(-1)
{
}

Roi::Roi(const QRect &roiRect, int roiId)
    : m_roi(roiRect),
      m_id(roiId)
{
}

Roi::Roi(const Roi &other)
    : m_roi(other.m_roi),
      m_id(other.m_id)
{
    // Deep copy of functions
    for (const auto& function : other.m_functions) {
        if (function) {
            m_functions.push_back(function->clone());
        }
    }
}

Roi& Roi::operator=(const Roi &other)
{
    if (this != &other) {
        m_roi = other.m_roi;
        m_id = other.m_id;
        
        // Clear existing functions
        m_functions.clear();
        
        // Deep copy of functions
        for (const auto& function : other.m_functions) {
            if (function) {
                m_functions.push_back(function->clone());
            }
        }
    }
    return *this;
}

QRect Roi::rect() const
{
    return m_roi;
}

int Roi::id() const
{
    return m_id;
}

void Roi::setRoi(const QRect &roiRect)
{
    m_roi = roiRect;
}

void Roi::setId(int roiId)
{
    m_id = roiId;
}

void Roi::addFunction(std::unique_ptr<Function> function)
{
    if (function) {
        // Remove existing function of same type first
        removeFunction(function->type());
        m_functions.push_back(std::move(function));
    }
}

void Roi::removeFunction(FunctionType type)
{
    m_functions.erase(
        std::remove_if(m_functions.begin(), m_functions.end(),
            [type](const std::unique_ptr<Function>& func) {
                return func && func->type() == type;
            }),
        m_functions.end());
}

bool Roi::hasFunction(FunctionType type) const
{
    return std::any_of(m_functions.begin(), m_functions.end(),
        [type](const std::unique_ptr<Function>& func) {
            return func && func->type() == type;
        });
}

Function* Roi::getFunction(FunctionType type)
{
    auto it = std::find_if(m_functions.begin(), m_functions.end(),
        [type](const std::unique_ptr<Function>& func) {
            return func && func->type() == type;
        });
    return (it != m_functions.end()) ? it->get() : nullptr;
}

const Function* Roi::getFunction(FunctionType type) const
{
    auto it = std::find_if(m_functions.begin(), m_functions.end(),
        [type](const std::unique_ptr<Function>& func) {
            return func && func->type() == type;
        });
    return (it != m_functions.end()) ? it->get() : nullptr;
}

const std::vector<std::unique_ptr<Function>>& Roi::getFunctions() const
{
    return m_functions;
}

// Convenience methods for histogram
bool Roi::hasHistogram() const
{
    return hasFunction(FunctionType::Histogram);
}

Histogram* Roi::getHistogram()
{
    return dynamic_cast<Histogram*>(getFunction(FunctionType::Histogram));
}

const Histogram* Roi::getHistogram() const
{
    return dynamic_cast<const Histogram*>(getFunction(FunctionType::Histogram));
}

void Roi::enableHistogram(const QPoint &position)
{
    auto histogram = std::make_unique<Histogram>(position);
    addFunction(std::move(histogram));
}

void Roi::disableHistogram()
{
    removeFunction(FunctionType::Histogram);
}

QPoint Roi::histogramPos() const
{
    const Histogram* hist = getHistogram();
    return hist ? hist->position() : QPoint(10, 10);
}

void Roi::setHistogramPos(const QPoint &position)
{
    Histogram* hist = getHistogram();
    if (hist) {
        hist->setPosition(position);
    }
}

void Roi::calculateFunctions(const QImage &image, const QRect &roi)
{
    for (auto& function : m_functions) {
        if (function) {
            function->calculate(image, roi);
        }
    }
}

void Roi::drawFunctions(QPainter &painter) const
{
    for (const auto& function : m_functions) {
        if (function) {
            function->draw(painter);
        }
    }
}

Function* Roi::findFunctionAt(const QPoint &point)
{
    for (auto& function : m_functions) {
        if (function && function->containsPoint(point)) {
            return function.get();
        }
    }
    return nullptr;
}

const Function* Roi::findFunctionAt(const QPoint &point) const
{
    for (const auto& function : m_functions) {
        if (function && function->containsPoint(point)) {
            return function.get();
        }
    }
    return nullptr;
}

QJsonObject Roi::toJson() const
{
    QJsonObject json;
    json["x"] = m_roi.x();
    json["y"] = m_roi.y();
    json["width"] = m_roi.width();
    json["height"] = m_roi.height();
    json["id"] = m_id;
    
    // Save functions
    QJsonArray functionsArray;
    for (const auto& function : m_functions) {
        if (function) {
            functionsArray.append(function->toJson());
        }
    }
    json["functions"] = functionsArray;
    
    return json;
}

void Roi::fromJson(const QJsonObject &json)
{
    // Clear existing functions
    m_functions.clear();
    
    // Load basic properties
    int x = json["x"].toInt();
    int y = json["y"].toInt();
    int width = json["width"].toInt();
    int height = json["height"].toInt();
    m_roi = QRect(x, y, width, height);
    m_id = json["id"].toInt();
    
    // Load functions
    if (json.contains("functions")) {
        QJsonArray functionsArray = json["functions"].toArray();
        for (const QJsonValue& value : functionsArray) {
            QJsonObject funcJson = value.toObject();
            auto function = createFunctionFromJson(funcJson);
            if (function) {
                m_functions.push_back(std::move(function));
            }
        }
    }
    // Backward compatibility: load old histogram format
    else if (json.contains("hasHistogram") && json["hasHistogram"].toBool()) {
        auto histogram = std::make_unique<Histogram>();
        if (json.contains("histogram")) {
            histogram->fromJson(json["histogram"].toObject());
        }
        addFunction(std::move(histogram));
    }
}

bool Roi::operator==(const Roi &other) const
{
    if (m_roi != other.m_roi || m_id != other.m_id || 
        m_functions.size() != other.m_functions.size()) {
        return false;
    }
    
    for (size_t i = 0; i < m_functions.size(); ++i) {
        if (!m_functions[i] || !other.m_functions[i] ||
            *m_functions[i] != *other.m_functions[i]) {
            return false;
        }
    }
    
    return true;
}

bool Roi::operator!=(const Roi &other) const
{
    return !(*this == other);
}
