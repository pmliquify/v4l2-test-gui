#include "projectmanager.hpp"

ProjectManager::ProjectManager()
{
}

bool ProjectManager::saveProject(const QString &filePath, const QList<Roi*> &rois, 
                                bool autoFit, const QPoint &imageOffset, double scaleFactor)
{
    m_lastError.clear();
    
    // Create project JSON
    QJsonObject projectJson = createProjectJson(rois, autoFit, imageOffset, scaleFactor);
    
    // Create JSON document
    QJsonDocument doc(projectJson);
    
    // Ensure directory exists
    QFileInfo fileInfo(filePath);
    QDir dir = fileInfo.absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(".")) {
            setError(QString("Failed to create directory: %1").arg(dir.absolutePath()));
            return false;
        }
    }
    
    // Write to file
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        setError(QString("Failed to open file for writing: %1").arg(file.errorString()));
        return false;
    }
    
    qint64 bytesWritten = file.write(doc.toJson());
    if (bytesWritten == -1) {
        setError(QString("Failed to write to file: %1").arg(file.errorString()));
        return false;
    }
    
    return true;
}

bool ProjectManager::loadProject(const QString &filePath, QList<Roi*> &rois, int &nextRoiId,
                                bool &autoFit, QPoint &imageOffset, double &scaleFactor)
{
    m_lastError.clear();
    qDeleteAll(rois);
    rois.clear();
    nextRoiId = 0;
    autoFit = true;
    imageOffset = QPoint(0, 0);
    scaleFactor = 1.0;
    
    // Check if file exists
    if (!QFile::exists(filePath)) {
        setError(QString("Project file does not exist: %1").arg(filePath));
        return false;
    }
    
    // Read file
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        setError(QString("Failed to open file for reading: %1").arg(file.errorString()));
        return false;
    }
    
    QByteArray data = file.readAll();
    
    // Parse JSON
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(data, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        setError(QString("JSON parse error: %1").arg(parseError.errorString()));
        return false;
    }
    
    if (!doc.isObject()) {
        setError("Invalid project file format: Root element is not an object");
        return false;
    }
    
    // Parse project data
    return parseProjectJson(doc.object(), rois, nextRoiId, autoFit, imageOffset, scaleFactor);
}

QString ProjectManager::getProjectFileFilter() const
{
    return "V4L2 Test GUI Project Files (*.v4l2proj);;All Files (*)";
}

QString ProjectManager::getDefaultProjectExtension() const
{
    return ".v4l2proj";
}

QString ProjectManager::lastError() const
{
    return m_lastError;
}

void ProjectManager::setError(const QString &error)
{
    m_lastError = error;
}

QJsonObject ProjectManager::createProjectJson(const QList<Roi*> &rois, bool autoFit, 
                                             const QPoint &imageOffset, double scaleFactor) const
{
    QJsonObject projectJson;
    
    // Project metadata
    projectJson["version"] = "1.0";
    projectJson["application"] = "v4l2-test-gui";
    projectJson["created"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    
    // Image view settings
    QJsonObject viewSettings;
    viewSettings["autoFit"] = autoFit;
    viewSettings["imageOffsetX"] = imageOffset.x();
    viewSettings["imageOffsetY"] = imageOffset.y();
    viewSettings["scaleFactor"] = scaleFactor;
    projectJson["viewSettings"] = viewSettings;
    
    // ROI data
    QJsonArray roiArray;
    for (const Roi *roi : rois) {
        roiArray.append(roi->toJson());
    }
    projectJson["rois"] = roiArray;
    
    return projectJson;
}

bool ProjectManager::parseProjectJson(const QJsonObject &json, QList<Roi*> &rois, int &nextRoiId,
                                     bool &autoFit, QPoint &imageOffset, double &scaleFactor)
{
    // Check version compatibility
    QString version = json["version"].toString();
    if (version != "1.0") {
        setError(QString("Unsupported project file version: %1").arg(version));
        return false;
    }
    
    // Parse view settings (optional for backward compatibility)
    if (json.contains("viewSettings")) {
        QJsonObject viewSettings = json["viewSettings"].toObject();
        autoFit = viewSettings["autoFit"].toBool(true);
        imageOffset.setX(viewSettings["imageOffsetX"].toInt(0));
        imageOffset.setY(viewSettings["imageOffsetY"].toInt(0));
        scaleFactor = viewSettings["scaleFactor"].toDouble(1.0);
    }
    
    // Parse ROIs
    if (!json.contains("rois") || !json["rois"].isArray()) {
        setError("Invalid project file format: Missing or invalid 'rois' array");
        return false;
    }
    
    QJsonArray roiArray = json["rois"].toArray();
    
    for (const QJsonValue &value : roiArray) {
        if (!value.isObject()) {
            setError("Invalid ROI data: Expected object");
            return false;
        }
        
        Roi *roi = new Roi();
        roi->fromJson(value.toObject());
        rois.append(roi);
        
        // Update next ID counter
        if (roi->id() >= nextRoiId) {
            nextRoiId = roi->id() + 1;
        }
    }
    
    return true;
}

// Backward compatibility overload
bool ProjectManager::loadProject(const QString &filePath, QList<Roi*> &rois, int &nextRoiId)
{
    bool autoFit;
    QPoint imageOffset;
    double scaleFactor;
    return loadProject(filePath, rois, nextRoiId, autoFit, imageOffset, scaleFactor);
}
