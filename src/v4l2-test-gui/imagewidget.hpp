#pragma once
#include <QtWidgets>
#include "roi.hpp"
#include "projectmanager.hpp"


class ImageWidget : public QWidget 
{
    Q_OBJECT
public:
    explicit ImageWidget(QWidget *parent = nullptr);

    QImage image() const;
    void setImage(const QImage &image);
    void setImageReceived(bool received);
    void setImageConverted(bool converted);
    bool isAutoFit() const;

signals:
    void autoFitChanged(bool enabled);

public slots:
    void fitImageToWidget();
    void saveProject(const QString &filePath);
    void loadProject(const QString &filePath);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QImage          m_image;
    double          m_scaleFactor;
    bool            m_dragging;
    QPoint          m_lastMousePos;
    QPoint          m_imageOffset;
    bool            m_drawingRoi;
    QRect           m_roiWidget;           // Current ROI in widget coordinates (while drawing)
    QList<Roi>      m_rois;                // List of all ROIs with their functions
    int             m_nextRoiId;           // Counter for unique IDs
    int             m_movingRoiIndex;      // Index of ROI being moved (-1 if none)
    int             m_draggingFunctionIndex; // Index of function being dragged (-1 if none)
    int             m_resizingRoiIndex;    // Index of ROI being resized (-1 if none)
    int             m_resizeHandle;        // Which resize handle is being dragged
    QPoint          m_roiMoveStart;
    QRect           m_roiOriginal;         // Original ROI position for moving
    QPoint          m_functionDragStart;
    bool            m_imageReceived;
    bool            m_imageConverted;
    bool            m_autoFit;
    ProjectManager  m_projectManager;   // Project file manager
    
    // Resize handle constants
    enum ResizeHandle {
        None = 0,
        TopLeft = 1,
        TopRight = 2,
        BottomLeft = 3,
        BottomRight = 4,
        Top = 5,
        Bottom = 6,
        Left = 7,
        Right = 8
    };
    
    QRect rasterizedImageRect() const;
    QRect widgetRectFromImageRect(const QRect &imageRect) const;
    int findRoiAt(const QPoint &pos) const;
    int findFunctionAt(const QPoint &pos) const;
    QPoint getNextFunctionPosition() const;
    void removeRoi(int index);
    int findResizeHandle(const QPoint &pos, int roiIndex) const;
    QCursor getResizeCursor(int handle) const;
    void validateFunctionPositions();
};
