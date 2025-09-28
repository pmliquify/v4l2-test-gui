#include "roi.hpp"
#include "functionplugin.hpp"


// =================================================================================================
// RoiAdapter implementation

class RoiAdapter : public PropertyAdapter 
{
public:
    explicit RoiAdapter(Roi* roi) : m_roi(roi) {}

    QString sectionName() const override { return "ROI"; }

    QList<PropertyInfo> availableProperties() const override
    {
        return {
            {"X", QMetaType::Int, 0, 10000, 1, "px"},
            {"Y", QMetaType::Int, 0, 10000, 1, "px"},
            {"Width", QMetaType::Int, 1, 10000, 1, "px"},
            {"Height", QMetaType::Int, 1, 10000, 1, "px"}
        };
    }

    QVariant getValue(const QString& name) const override
    {
        if (name == "X")        return m_roi->qRect().x();
        if (name == "Y")        return m_roi->qRect().y();
        if (name == "Width")    return m_roi->qRect().width();
        if (name == "Height")   return m_roi->qRect().height();
        return QVariant();
    }

    void setValue(const QString& name, const QVariant& value) override
    {
        if (name == "X") {
            QRect rect = m_roi->qRect();
            rect.moveLeft(value.toInt());
            m_roi->setRoi(rect);
        } if (name == "Y") {
            QRect rect = m_roi->qRect();
            rect.moveTop(value.toInt());
            m_roi->setRoi(rect);
        } if (name == "Width") {
            QRect rect = m_roi->qRect();
            rect.setWidth(value.toInt());
            m_roi->setRoi(rect);
        } if (name == "Height") {
            QRect rect = m_roi->qRect();
            rect.setHeight(value.toInt());
            m_roi->setRoi(rect);
        }
    }

private:
    Roi* m_roi;
};


// =================================================================================================
// Roi implementation

Roi::Roi() :
    m_propertyAdapter(new RoiAdapter(this)),
    m_id(-1)
{
}

Roi::Roi(const QRect &roiRect, int roiId) :
    m_propertyAdapter(new RoiAdapter(this)),
    m_roi(qRectToCvRect(roiRect)),
    m_id(roiId)
{
}

Roi::Roi(const cv::Rect &roiRect, int roiId) :
    m_propertyAdapter(new RoiAdapter(this)),
    m_roi(roiRect),
    m_id(roiId)
{
}

Roi::~Roi()
{
    delete m_propertyAdapter;
}

PropertyAdapter* Roi::propertyAdapter() const
{
    return m_propertyAdapter;
}

QRect Roi::qRect() const
{
    return cvRectToQRect(m_roi);
}

cv::Rect Roi::rect() const
{
    return m_roi;
}

int Roi::id() const
{
    return m_id;
}

int Roi::area() const
{
    return m_roi.area();
}

void Roi::setRoi(const QRect &roiRect)
{
    m_roi = qRectToCvRect(roiRect);
}

void Roi::setRoi(const cv::Rect &roiRect)
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
        removeFunction(function->typeId());
        m_functions.push_back(std::move(function));
    }
}

void Roi::removeFunction(const QString &typeId)
{
    m_functions.erase(
        std::remove_if(m_functions.begin(), m_functions.end(),
            [&typeId](const std::unique_ptr<Function>& func) {
                return func && func->typeId() == typeId;
            }),
        m_functions.end());
}

bool Roi::hasFunction(const QString &typeId) const
{
    return std::any_of(m_functions.begin(), m_functions.end(),
        [&typeId](const std::unique_ptr<Function>& func) {
            return func && func->typeId() == typeId;
        });
}

Function* Roi::getFunction(const QString &typeId)
{
    auto it = std::find_if(m_functions.begin(), m_functions.end(),
        [&typeId](const std::unique_ptr<Function>& func) {
            return func && func->typeId() == typeId;
        });
    return (it != m_functions.end()) ? it->get() : nullptr;
}

const Function* Roi::getFunction(const QString &typeId) const
{
    auto it = std::find_if(m_functions.begin(), m_functions.end(),
        [&typeId](const std::unique_ptr<Function>& func) {
            return func && func->typeId() == typeId;
        });
    return (it != m_functions.end()) ? it->get() : nullptr;
}

const std::vector<std::unique_ptr<Function>>& Roi::getFunctions() const
{
    return m_functions;
}

void Roi::calculateFunctions(const cv::Mat &image, const cv::Rect &roi)
{
    for (auto& function : m_functions) {
        if (function) {
            function->calculate(image, roi);
        }
    }
}

void Roi::drawFunctions(QPainter &painter, double scaleFactor, const QPoint &imageOffset) const
{
    for (const auto& function : m_functions) {
        if (function) {
            function->draw(painter, scaleFactor, imageOffset);
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
    json["x"] = m_roi.x;
    json["y"] = m_roi.y;
    json["width"] = m_roi.width;
    json["height"] = m_roi.height;
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
    m_roi = cv::Rect(x, y, width, height);
    m_id = json["id"].toInt();
    
    // Load functions
    if (json.contains("functions")) {
        QJsonArray functionsArray = json["functions"].toArray();
        for (const QJsonValue& value : functionsArray) {
            QJsonObject funcJson = value.toObject();
            auto function = FunctionRegistry::instance().createFunctionFromJson(funcJson);
            if (function) {
                m_functions.push_back(std::move(function));
            }
        }
    }
}

bool Roi::operator==(const Roi &other) const
{
    if (m_roi.x != other.m_roi.x || m_roi.y != other.m_roi.y ||
        m_roi.width != other.m_roi.width || m_roi.height != other.m_roi.height ||
        m_id != other.m_id || m_functions.size() != other.m_functions.size()) {
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

// Helper functions for conversion
cv::Rect Roi::qRectToCvRect(const QRect &qRect)
{
    return cv::Rect(qRect.x(), qRect.y(), qRect.width(), qRect.height());
}

QRect Roi::cvRectToQRect(const cv::Rect &cvRect)
{
    return QRect(cvRect.x, cvRect.y, cvRect.width, cvRect.height);
}

