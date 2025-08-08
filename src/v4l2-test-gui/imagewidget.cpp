#include "imagewidget.hpp"
#include <QtNetwork>
#include <QtCharts>
#include <opencv2/opencv.hpp>


ImageWidget::ImageWidget(QWidget *parent)
    : QWidget(parent),
      m_scaleFactor(1.0),
      m_dragging(false),
      m_imageOffset(0, 0),
      m_drawingRoi(false),
      m_nextRoiId(0),
      m_movingRoiIndex(-1),
      m_draggingFunctionIndex(-1),
      m_resizingRoiIndex(-1),
      m_resizeHandle(None),
      m_imageReceived(false),
      m_imageConverted(false),
      m_autoFit(true)
{
}

QImage ImageWidget::image() const 
{
    return m_image;
}

void ImageWidget::setImage(const QImage &image) 
{
    bool fitToWindow = m_image.size() != image.size();
    bool wasFirstImage = m_image.isNull();
    m_image = image;
    if (fitToWindow) {
        fitImageToWidget();
    } else {
        update();
    }
    
    // If this is the first image and we have loaded ROIs, validate function positions
    if (wasFirstImage && !m_rois.isEmpty()) {
        validateFunctionPositions();
    }
}

void ImageWidget::fitImageToWidget() 
{
    double widthRatio = (double)width() / m_image.width();
    double heightRatio = (double)height() / m_image.height();
    m_scaleFactor = qMin(widthRatio, heightRatio);
    QSize scaledSize = m_image.size() * m_scaleFactor;
    int offsetX = (width() - scaledSize.width()) / 2;
    int offsetY = (height() - scaledSize.height()) / 2;
    m_imageOffset = QPoint(offsetX, offsetY);

    m_autoFit = true;
    emit autoFitChanged(true);

    update();
}

void ImageWidget::resizeEvent(QResizeEvent *event)
{
    if (m_autoFit) {
        fitImageToWidget();
    } else {
        QWidget::resizeEvent(event);
    }
    
        // Only validate function positions if we have received an image
    if (m_imageReceived) {
        validateFunctionPositions();
    }
}

void ImageWidget::setImageReceived(bool received)
{
    m_imageReceived = received;
    update();
}

void ImageWidget::setImageConverted(bool converted)
{
    m_imageConverted = converted;
    update();
}

bool ImageWidget::isAutoFit() const
{ 
        return m_autoFit; 
}

void ImageWidget::paintEvent(QPaintEvent *event) 
{
    Q_UNUSED(event);
    QPainter painter(this);
    if (!m_imageReceived || !m_imageConverted) {
        painter.setFont(QFont("Arial", 20));
        if (m_imageReceived && !m_imageConverted) {
            painter.setPen(Qt::darkRed);
            painter.drawText(rect(), Qt::AlignCenter, tr("Unable to convert Image.\nPixelformat not supported!"));
        } else {
            painter.setPen(Qt::darkGray);
            QString hostName = QHostInfo::localHostName();
            painter.drawText(rect(), Qt::AlignCenter, tr("./v4l2-test client --ip %1").arg(hostName));
        }
        return;
    }
    painter.translate(m_imageOffset);
    painter.scale(m_scaleFactor, m_scaleFactor);
    painter.drawImage(0, 0, m_image);

    // Draw all persistent ROIs and their functions
    for (int i = 0; i < m_rois.size(); ++i) {
        const Roi &roi = m_rois[i];
        QRect imageRoi = roi.rect();
        if (imageRoi.height() > 0) {
            imageRoi.setHeight(imageRoi.height() - 1);
        }
        QRect widgetRoi = widgetRectFromImageRect(imageRoi);
        painter.resetTransform();
        painter.setPen(QPen(Qt::yellow, 2));
        painter.drawRect(widgetRoi);
        
        // Calculate and draw functions for this ROI
        if (imageRoi.width() > 0 && imageRoi.height() > 0) {
            // Create a mutable copy to update function data
            Roi& mutableRoi = const_cast<Roi&>(roi);
            mutableRoi.calculateFunctions(m_image, imageRoi);
            mutableRoi.drawFunctions(painter);
        }
    }
    
    // Draw current drawing ROI if actively drawing
    if (m_drawingRoi) {
        QRect normalizedWidget = m_roiWidget.normalized();
        QPointF imgStart = (normalizedWidget.topLeft() - m_imageOffset) / m_scaleFactor;
        QPointF imgEnd   = (normalizedWidget.bottomRight() - m_imageOffset) / m_scaleFactor;
        QRect imageRoi = QRect(imgStart.toPoint(), imgEnd.toPoint()).normalized();
        if (imageRoi.height() > 0) {
            imageRoi.setHeight(imageRoi.height() - 1);
        }
        QRect widgetRoi = widgetRectFromImageRect(imageRoi);
        painter.resetTransform();
        painter.setPen(QPen(Qt::red, 2)); // Use red color for the new ROI being drawn
        painter.drawRect(widgetRoi);
    }
}

void ImageWidget::wheelEvent(QWheelEvent *event)
{
    int delta = event->angleDelta().y();
    double factor = (delta > 0) ? 1.1 : 0.9;
    m_scaleFactor *= factor;
    QPoint mousePos = event->position().toPoint();
    m_imageOffset = (m_imageOffset - mousePos) * factor + mousePos;
    
    m_autoFit = false;
    emit autoFitChanged(false);

    update();
}

void ImageWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        // Right click: check if we're clicking on a ROI to delete it, or drag the image
        int roiIndex = findRoiAt(event->pos());
        if (roiIndex >= 0) {
            removeRoi(roiIndex);
            update();
        } else {
            m_dragging = true;
            m_lastMousePos = event->pos();
            m_autoFit = false;
            emit autoFitChanged(false);
        }
    }
    if (event->button() == Qt::LeftButton) {
        // Check if clicking on a function
        int funcIndex = findFunctionAt(event->pos());
        if (funcIndex >= 0) {
            m_draggingFunctionIndex = funcIndex;
            // For now, only support histogram dragging
            if (m_rois[funcIndex].hasHistogram()) {
                m_functionDragStart = event->pos() - m_rois[funcIndex].histogramPos();
            }
        }
        // Check if clicking inside existing ROI
        else {
            int roiIndex = findRoiAt(event->pos());
            if (roiIndex >= 0) {
                // Check if clicking on a resize handle first
                int resizeHandle = findResizeHandle(event->pos(), roiIndex);
                if (resizeHandle != None) {
                    m_resizingRoiIndex = roiIndex;
                    m_resizeHandle = resizeHandle;
                    m_roiOriginal = m_rois[roiIndex].rect();
                    m_roiMoveStart = event->pos();
                    setCursor(getResizeCursor(resizeHandle));
                } else {
                    // Normal move operation
                    m_movingRoiIndex = roiIndex;
                    m_roiMoveStart = event->pos();
                    m_roiOriginal = m_rois[roiIndex].rect();
                }
            } else {
                // Start drawing new ROI
                m_drawingRoi = true;
                m_roiWidget = QRect(event->pos(), event->pos());
            }
        }
    }
    QWidget::mousePressEvent(event);
}

void ImageWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_dragging) {
        QPoint delta = event->pos() - m_lastMousePos;
        m_imageOffset += delta;
        m_lastMousePos = event->pos();
        update();
    }
    if (m_draggingFunctionIndex >= 0) {
        QPoint newPosition = event->pos() - m_functionDragStart;
        
        // Clamp function position to stay within widget bounds
        const int functionWidth = 256;
        const int functionHeight = 100;
        
        newPosition.setX(qMax(0, qMin(newPosition.x(), width() - functionWidth)));
        newPosition.setY(qMax(0, qMin(newPosition.y(), height() - functionHeight)));
        
        // For now, only support histogram dragging via the convenience method
        if (m_rois[m_draggingFunctionIndex].hasHistogram()) {
            m_rois[m_draggingFunctionIndex].setHistogramPos(newPosition);
        }
        update();
    }
    if (m_movingRoiIndex >= 0) {
        // Calculate total delta from the start of the move operation
        QPoint totalDelta = event->pos() - m_roiMoveStart;
        QPointF imageDelta = QPointF(totalDelta) / m_scaleFactor;
        
        // Apply delta to original ROI position
        m_rois[m_movingRoiIndex].setRoi(m_roiOriginal.translated(imageDelta.toPoint()));
        update();
    }
    if (m_resizingRoiIndex >= 0) {
        // Calculate delta from start of resize operation
        QPoint totalDelta = event->pos() - m_roiMoveStart;
        QPointF imageDelta = QPointF(totalDelta) / m_scaleFactor;
        
        QRect newRoi = m_roiOriginal;
        
        // Only handle bottom-right resize
        if (m_resizeHandle == BottomRight) {
            newRoi.setBottomRight(newRoi.bottomRight() + imageDelta.toPoint());
        }
        
        // Ensure minimum size
        if (newRoi.width() < 10) {
            newRoi.setRight(newRoi.left() + 10);
        }
        if (newRoi.height() < 10) {
            newRoi.setBottom(newRoi.top() + 10);
        }
        
        m_rois[m_resizingRoiIndex].setRoi(newRoi.normalized());
        
        // Show tooltip with ROI size during resize
        QToolTip::showText(event->globalPosition().toPoint(),
                           QString("%1x%2").arg(newRoi.width()).arg(newRoi.height()), this);
        
        update();
    }
    if (m_drawingRoi) {
        m_roiWidget.setBottomRight(event->pos());
        QRect normalizedRoi = m_roiWidget.normalized();
        QPointF imgStart = (normalizedRoi.topLeft() - m_imageOffset) / m_scaleFactor;
        QPointF imgEnd   = (normalizedRoi.bottomRight() - m_imageOffset) / m_scaleFactor;
        QRect imageRoi = QRect(imgStart.toPoint(), imgEnd.toPoint()).normalized();
        QToolTip::showText(event->globalPosition().toPoint(),
                           QString("%1x%2").arg(imageRoi.width()).arg(imageRoi.height()), this);
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void ImageWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton && m_dragging) {
        m_dragging = false;
    }
    if (event->button() == Qt::LeftButton && m_draggingFunctionIndex >= 0) {
        m_draggingFunctionIndex = -1;
    }
    if (event->button() == Qt::LeftButton && m_movingRoiIndex >= 0) {
        m_movingRoiIndex = -1;
    }
    if (event->button() == Qt::LeftButton && m_resizingRoiIndex >= 0) {
        m_resizingRoiIndex = -1;
        m_resizeHandle = None;
        setCursor(Qt::ArrowCursor);
        QToolTip::hideText(); // Hide tooltip when resize is finished
    }
    if (event->button() == Qt::LeftButton && m_drawingRoi) {
        m_drawingRoi = false;
        QToolTip::hideText();
        
        // Check if this was a simple click (no drag) or a drag operation
        QPoint dragDistance = event->pos() - m_roiWidget.topLeft();
        bool wasSimpleClick = (qAbs(dragDistance.x()) < 3 && qAbs(dragDistance.y()) < 3);
        
        if (!wasSimpleClick) {
            // Drag operation - create new ROI
            QRect normalizedWidget = m_roiWidget.normalized();
            QPointF imgStart = (normalizedWidget.topLeft() - m_imageOffset) / m_scaleFactor;
            QPointF imgEnd   = (normalizedWidget.bottomRight() - m_imageOffset) / m_scaleFactor;
            QRect imageRoi = QRect(imgStart.toPoint(), imgEnd.toPoint()).normalized();
            
            // Create new ROI data
            Roi newRoi;
            newRoi.setRoi(imageRoi);
            newRoi.setId(m_nextRoiId++);
            newRoi.enableHistogram(getNextFunctionPosition());
            
            m_rois.append(newRoi);
        }
        
        update();
    }
    QWidget::mouseReleaseEvent(event);
}

QRect ImageWidget::rasterizedImageRect() const 
{
    if (!m_rois.isEmpty()) {
        // Return the first ROI for compatibility
        QRect roi = m_rois.first().rect();
        if (roi.height() > 0) {
            roi.setHeight(roi.height() - 1);
        }
        return roi;
    } else {
        // Use current drawing coordinates when actively drawing
        QRect normalizedWidget = m_roiWidget.normalized();
        QPointF imgStart = (normalizedWidget.topLeft() - m_imageOffset) / m_scaleFactor;
        QPointF imgEnd   = (normalizedWidget.bottomRight() - m_imageOffset) / m_scaleFactor;
        QRect roi = QRect(imgStart.toPoint(), imgEnd.toPoint()).normalized();
        if (roi.height() > 0) {
            roi.setHeight(roi.height() - 1);
        }
        return roi;
    }
}

QRect ImageWidget::widgetRectFromImageRect(const QRect &imageRect) const 
{
    QPoint p1(qRound(imageRect.x() * m_scaleFactor) + m_imageOffset.x(),
              qRound(imageRect.y() * m_scaleFactor) + m_imageOffset.y());
    QPoint p2(qRound((imageRect.x() + imageRect.width()) * m_scaleFactor) + m_imageOffset.x(),
              qRound((imageRect.y() + imageRect.height()) * m_scaleFactor) + m_imageOffset.y());
    return QRect(p1, p2);
}

int ImageWidget::findRoiAt(const QPoint &pos) const
{
    for (int i = 0; i < m_rois.size(); ++i) {
        QRect widgetRoi = widgetRectFromImageRect(m_rois[i].rect());
        if (widgetRoi.contains(pos)) {
            return i;
        }
    }
    return -1;
}

int ImageWidget::findFunctionAt(const QPoint &pos) const
{
    for (int i = 0; i < m_rois.size(); ++i) {
        const Function* function = m_rois[i].findFunctionAt(pos);
        if (function) {
            return i;
        }
    }
    return -1;
}

QPoint ImageWidget::getNextFunctionPosition() const
{
    // Start position for new functions
    QPoint basePos(10, 10);
    
    // Check if this position is free
    const int functionWidth = 256;
    const int functionHeight = 100;
    
    for (int i = 0; i < m_rois.size(); ++i) {
        if (m_rois[i].hasHistogram()) {
            QRect existingRect(m_rois[i].histogramPos(), QSize(functionWidth, functionHeight));
            QRect newRect(basePos, QSize(functionWidth, functionHeight));
            
            if (existingRect.intersects(newRect)) {
                // Move down by function height + spacing
                basePos.setY(basePos.y() + functionHeight + 10);
                i = -1; // Restart check from beginning
            }
        }
    }
    
    // Ensure position is within widget bounds
    if (basePos.y() + functionHeight > height()) {
        basePos.setY(10);
        basePos.setX(basePos.x() + functionWidth + 10);
    }
    
    return basePos;
}

void ImageWidget::removeRoi(int index)
{
    if (index >= 0 && index < m_rois.size()) {
        m_rois.removeAt(index);
        
        // Update active indices if they point to removed or shifted ROIs
        if (m_movingRoiIndex == index) {
            m_movingRoiIndex = -1;
        } else if (m_movingRoiIndex > index) {
            m_movingRoiIndex--;
        }
        
        if (m_draggingFunctionIndex == index) {
            m_draggingFunctionIndex = -1;
        } else if (m_draggingFunctionIndex > index) {
            m_draggingFunctionIndex--;
        }
        
        if (m_resizingRoiIndex == index) {
            m_resizingRoiIndex = -1;
        } else if (m_resizingRoiIndex > index) {
            m_resizingRoiIndex--;
        }
    }
}

int ImageWidget::findResizeHandle(const QPoint &pos, int roiIndex) const
{
    if (roiIndex < 0 || roiIndex >= m_rois.size()) {
        return None;
    }
    
    QRect widgetRoi = widgetRectFromImageRect(m_rois[roiIndex].rect());
    const int handleSize = 12; // Larger area for easier clicking without visible handle
    
    // Check bottom-right corner - center the detection area on the corner
    QPoint cornerPos = widgetRoi.bottomRight();
    QRect handleRect(cornerPos - QPoint(handleSize/2, handleSize/2), QSize(handleSize, handleSize));
    if (handleRect.contains(pos)) {
        return BottomRight;
    }
    
    return None;
}

QCursor ImageWidget::getResizeCursor(int handle) const
{
    if (handle == BottomRight) {
        return Qt::SizeFDiagCursor;
    }
    return Qt::ArrowCursor;
}

void ImageWidget::validateFunctionPositions()
{
    if (width() < 300 || height() < 200) {
        return; // Widget too small to validate
    }
    
    const int functionWidth = 256;
    const int functionHeight = 135; // Height + title + x-axis space
    
    for (auto &roi : m_rois) {
        // For now, only validate histogram positions
        if (roi.hasHistogram()) {
            QPoint pos = roi.histogramPos();
            
            // Only adjust if the function would be completely outside the widget
            int maxX = width() - functionWidth;
            int maxY = height() - functionHeight;
            
            if (pos.x() > maxX) {
                pos.setX(maxX);
            }
            if (pos.y() > maxY) {
                pos.setY(maxY);
            }
            
            // Ensure minimum position (not negative)
            if (pos.x() < 0) {
                pos.setX(0);
            }
            if (pos.y() < 0) {
                pos.setY(0);
            }
            
            // Update the function position
            roi.setHistogramPos(pos);
        }
    }
}

void ImageWidget::saveProject(const QString &filePath)
{
    if (!m_projectManager.saveProject(filePath, m_rois)) {
        // Handle error - could emit a signal or show a message
        qWarning() << "Failed to save project to" << filePath;
    }
}

void ImageWidget::loadProject(const QString &filePath)
{
    QList<Roi> loadedRois;
    int nextRoiId;
    if (m_projectManager.loadProject(filePath, loadedRois, nextRoiId)) {
        m_rois = loadedRois;
        m_nextRoiId = nextRoiId;
        
        // Only validate function positions if an image has been received
        // This prevents setting wrong positions when widget size is not yet stable
        if (m_imageReceived) {
            validateFunctionPositions();
        }
        
        update();
    } else {
        // Handle error - could emit a signal or show a message
        qWarning() << "Failed to load project from" << filePath;
    }
}
