#pragma once
#include <QtWidgets>
#include <opencv2/opencv.hpp>
#include "roi.hpp"
#include "projectmanager.hpp"


class ImageWidget : public QWidget 
{
    Q_OBJECT
public:
    explicit ImageWidget(QWidget *parent = nullptr);

    cv::Mat cvImage() const;
    QImage qImage() const;
    void setImage(const cv::Mat &image);
    void setImageReceived(bool received);
    void setImageConverted(bool converted);
    bool isAutoFit() const;

signals:
    void autoFitChanged(bool enabled);
    void roiSelectionChanged(Roi *roi);
    void functionWidgetCreated(QWidget* widget, const QString& title);
    void functionWidgetRemoved(QWidget* widget);

public slots:
    void fitImageToWidget();
    void saveProject(const QString &filePath);
    void loadProject(const QString &filePath);
    void setImagePosition(const QPoint &offset, double scaleFactor);
    void updateFunctions();

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private:
    cv::Mat         m_image;          // OpenCV image for efficient processing
    mutable QImage  m_qImage;         // Cached QImage for display (created on demand)
    mutable bool    m_qImageValid;    // Whether the cached QImage is up to date
    double          m_scaleFactor;
    bool            m_dragging;
    bool            m_panModeActive;      // True when space key is held down
    QPoint          m_lastMousePos;
    QPoint          m_imageOffset;
    bool            m_drawingRoi;
    QRect           m_roiWidget;           // Current ROI in widget coordinates (while drawing)
    QList<Roi*>     m_rois;                // List of all ROIs with their functions
    int             m_nextRoiId;           // Counter for unique IDs
    int             m_movingRoiIndex;      // Index of ROI being moved (-1 if none)
    int             m_draggingFunctionIndex; // Index of function being dragged (-1 if none)
    Function*       m_draggingFunction;    // Pointer to the specific function being dragged
    int             m_resizingRoiIndex;    // Index of ROI being resized (-1 if none)
    int             m_resizeHandle;        // Which resize handle is being dragged
    QPoint          m_roiMoveStart;
    QRect           m_roiOriginal;         // Original ROI position for moving
    QPoint          m_functionDragStart;
    bool            m_imageReceived;
    bool            m_imageConverted;
    bool            m_autoFit;
    ProjectManager  m_projectManager;   // Project file manager
    int             m_contextMenuRoiIndex; // Index of ROI for context menu (-1 if none)
    QElapsedTimer   m_lastFunctionUpdate; // Timer to throttle function updates
    QElapsedTimer   m_lastPaintUpdate;    // Timer to throttle paint updates
    bool            m_updatePending;      // Flag to indicate if update is needed
    QPixmap         m_cachedBackground;   // Cached background with scaled image
    bool            m_backgroundDirty;    // Flag to indicate if background needs redraw
    bool            m_overlaysVisible;    // Flag to control visibility of ROI and function overlays
    int             m_selectedRoiIndex;   // Index of currently selected ROI (-1 if none)
    
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
    void showRoiContextMenu(const QPoint &pos, int roiIndex);
    void toggleFunction(int roiIndex, const QString &typeId);
    void initializeFunctionsAfterProjectLoad(); // Initialize functions after project load (e.g., open dialogs)
    void scheduleUpdate(); // Throttled update method
    void doUpdate();
    void updateBackgroundCache(); // Update the cached background
    QWidget* getTopLevelWidget() const; // Helper to find the top-level widget (MainWindow)
    void sortRoisBySize(); // Sort ROIs by area (smallest first) for better mouse interaction
    void setSelectedRoi(int index); // Set selected ROI and emit signal
};
