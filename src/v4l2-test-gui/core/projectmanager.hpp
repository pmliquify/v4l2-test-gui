#pragma once

#include <QtCore>
#include "roi.hpp"

class ProjectManager
{
public:
    ProjectManager();
    
    // Project file operations
    bool saveProject(const QString &filePath, const QList<Roi*> &rois, 
                    bool autoFit = true, const QPoint &imageOffset = QPoint(0, 0), 
                    double scaleFactor = 1.0);
    bool loadProject(const QString &filePath, QList<Roi*> &rois, int &nextRoiId,
                    bool &autoFit, QPoint &imageOffset, double &scaleFactor);
    // Overload for backward compatibility
    bool loadProject(const QString &filePath, QList<Roi*> &rois, int &nextRoiId);
    
    // Utility functions
    QString getProjectFileFilter() const;
    QString getDefaultProjectExtension() const;
    
    // Error handling
    QString lastError() const;

private:
    QString m_lastError;
    
    void setError(const QString &error);
    QJsonObject createProjectJson(const QList<Roi*> &rois, bool autoFit, 
                                 const QPoint &imageOffset, double scaleFactor) const;
    bool parseProjectJson(const QJsonObject &json, QList<Roi*> &rois, int &nextRoiId,
                         bool &autoFit, QPoint &imageOffset, double &scaleFactor);
};
