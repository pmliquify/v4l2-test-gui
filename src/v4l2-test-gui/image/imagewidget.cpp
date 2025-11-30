#include "imagewidget.hpp"
#include "mainwindow.hpp"
#include "convert.hpp"
#include "functionplugin.hpp"
#include <opencv2/opencv.hpp>
#include <algorithm>
#include <numeric>


ImageWidget::ImageWidget(QWidget *parent)
    : QWidget(parent),
      m_qImageValid(false),
      m_scaleFactor(1.0),
      m_dragging(false),
      m_panModeActive(false),
      m_imageOffset(0, 0),
      m_drawingRoi(false),
      m_nextRoiId(0),
      m_movingRoiIndex(-1),
      m_draggingFunctionIndex(-1),
      m_draggingFunction(nullptr),
      m_resizingRoiIndex(-1),
      m_resizeHandle(None),
      m_imageReceived(false),
      m_imageConverted(false),
      m_autoFit(true),
      m_contextMenuRoiIndex(-1),
      m_updatePending(false),
      m_backgroundDirty(true),
      m_overlaysVisible(true),
      m_selectedRoiIndex(-1),
      m_maxImageCount(10),
      m_currentImageIndex(-1)
{
    m_lastFunctionUpdate.start();
    m_lastPaintUpdate.start();
    
    // Enable focus to receive key events
    setFocusPolicy(Qt::StrongFocus);
}

cv::Mat ImageWidget::cvImage() const 
{
    return m_image;
}

QImage ImageWidget::qImage() const 
{
    if (!m_qImageValid || m_qImage.isNull()) {
        if (!m_image.empty()) {
            if(m_image.type() == CV_8UC3) {
                // OpenCV manages colors as BGR, QImage expects RGB
                QImage image(m_image.data, m_image.cols, m_image.rows, static_cast<int>(m_image.step), QImage::Format_RGB888);
                m_qImage = image.rgbSwapped();
                m_qImage.setColorSpace(QColorSpace::SRgb);

            } else if(m_image.type() == CV_8UC1) {
                m_qImage = QImage(m_image.data, m_image.cols, m_image.rows, static_cast<int>(m_image.step), QImage::Format_Grayscale8);

            } else {
                qDebug() << "Unsupported cv::Mat format!";
                m_qImage = QImage();
            }
            m_qImageValid = true;
        }
    }
    return m_qImage;
}

void ImageWidget::setImage(const cv::Mat& image) 
{
    // Use setImage with default values for sequence and timestamp
    setImage(image, 0, 0);
}

void ImageWidget::setImage(const cv::Mat& image, unsigned int sequence, unsigned long timestamp)
{
    QSize oldSize = m_image.empty() ? QSize() : QSize(m_image.cols, m_image.rows);
    bool fitToWindow = oldSize != QSize(image.cols, image.rows);
    bool wasFirstImage = m_image.empty();
    
    // Add image to ring buffer
    ImageBufferEntry entry;
    entry.image = image.clone();
    entry.sequence = sequence;
    entry.timestamp = timestamp;
    
    m_imageBuffer.append(entry);
    
    // Remove oldest image if buffer exceeds max count
    while (m_imageBuffer.size() > m_maxImageCount) {
        m_imageBuffer.removeFirst();
        // Adjust current index if we're viewing an image that was removed
        if (m_currentImageIndex > 0) {
            m_currentImageIndex--;
        }
    }
    
    // Always select the newest image (last in buffer) when a new image arrives
    m_currentImageIndex = m_imageBuffer.size() - 1;
    
    // Update current display image to the newest one
    m_image = image.clone();
    m_qImageValid = false; // Invalidate cached QImage
    m_backgroundDirty = true; // Mark background as dirty when image changes
    
    // Emit signals
    emit imageCountChanged(m_imageBuffer.size(), m_maxImageCount);
    emit currentImageIndexChanged(m_currentImageIndex, sequence, timestamp);
    
    if (fitToWindow) {
        fitImageToWidget();
    }
    
    // If this is the first image and we have loaded ROIs, validate function positions
    if (wasFirstImage && !m_rois.isEmpty()) {
        validateFunctionPositions();
    }
    
    updateFunctions();
}

void ImageWidget::fitImageToWidget() 
{
    double widthRatio = (double)width() / m_image.cols;
    double heightRatio = (double)height() / m_image.rows;
    m_scaleFactor = qMin(widthRatio, heightRatio);
    QSize scaledSize = QSize(m_image.cols, m_image.rows) * m_scaleFactor;
    int offsetX = (width() - scaledSize.width()) / 2;
    int offsetY = (height() - scaledSize.height()) / 2;
    m_imageOffset = QPoint(offsetX, offsetY);

    m_autoFit = true;
    emit autoFitChanged(true);
    m_backgroundDirty = true; // Mark background as dirty when transform changes

    update();
}

void ImageWidget::resizeEvent(QResizeEvent *event)
{
    if (m_autoFit) {
        fitImageToWidget();
    } else {
        QWidget::resizeEvent(event);
    }
    
    // Invalidate background cache on resize
    m_backgroundDirty = true;
    
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
    
    // Update background cache if needed
    if (m_backgroundDirty || m_cachedBackground.size() != size()) {
        updateBackgroundCache();
    }
    
    // Draw cached background
    if (!m_cachedBackground.isNull()) {
        painter.drawPixmap(0, 0, m_cachedBackground);
    }

    // Draw all persistent ROIs and their functions (reverse order so smaller ROIs are drawn on top)
    // Only draw overlays if they are visible
    if (m_overlaysVisible) {
        for (int i = m_rois.size() - 1; i >= 0; --i) {
            Roi *roi = m_rois[i];
            QRect imageRoi = roi->qRect();
            if (imageRoi.height() > 0) {
                imageRoi.setHeight(imageRoi.height() - 1);
            }
            QRect widgetRoi = widgetRectFromImageRect(imageRoi);
        
            // Save painter state before drawing ROI
            painter.save();
            
            // Highlight selected ROI with different color and thickness
            if (i == m_selectedRoiIndex) {
                painter.setPen(QPen(Qt::green, 3));  // Green, thicker for selected ROI
            } else {
                painter.setPen(QPen(Qt::yellow, 2));  // Yellow for normal ROIs
            }
            
            painter.setBrush(Qt::NoBrush);
            painter.drawRect(widgetRoi);
            painter.restore();
            
            // Calculate and draw functions for this ROI
            if (imageRoi.width() > 0 && imageRoi.height() > 0) {
                // Throttle function updates during interactive ROI operations to reduce flicker
                bool isThisRoiBeingMoved = (m_movingRoiIndex == i || m_resizingRoiIndex == i);
                bool shouldUpdateFunctions = true;
                
                if (isThisRoiBeingMoved) {
                    // During ROI operations, only update every 50ms to reduce flicker
                    if (m_lastFunctionUpdate.elapsed() < 50) {
                        shouldUpdateFunctions = false;
                    }
                }
                
                if (shouldUpdateFunctions) {
                    // Convert QRect to cv::Rect for function calculation
                    cv::Rect cvRoi(imageRoi.x(), imageRoi.y(), imageRoi.width(), imageRoi.height());
                    roi->calculateFunctions(m_image, cvRoi);
                    if (isThisRoiBeingMoved) {
                        m_lastFunctionUpdate.restart();
                    }
                }
                
                // Always draw functions with transformation parameters
                roi->drawFunctions(painter, m_scaleFactor, m_imageOffset);
            }
        }
    }
    
    // Draw current drawing ROI if actively drawing and overlays are visible
    if (m_drawingRoi && m_overlaysVisible) {
        QRect normalizedWidget = m_roiWidget.normalized();
        QPointF imgStart = (normalizedWidget.topLeft() - m_imageOffset) / m_scaleFactor;
        QPointF imgEnd   = (normalizedWidget.bottomRight() - m_imageOffset) / m_scaleFactor;
        QRect imageRoi = QRect(imgStart.toPoint(), imgEnd.toPoint()).normalized();
        if (imageRoi.height() > 0) {
            imageRoi.setHeight(imageRoi.height() - 1);
        }
        QRect widgetRoi = widgetRectFromImageRect(imageRoi);
        
        // Save painter state before drawing new ROI
        painter.save();
        painter.setPen(QPen(Qt::red, 2));
        painter.setBrush(Qt::NoBrush);
        painter.drawRect(widgetRoi);
        painter.restore();
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
    m_backgroundDirty = true; // Mark background dirty when zooming

    update();
}

void ImageWidget::mousePressEvent(QMouseEvent *event)
{
    // Handle pan mode with left mouse when space is held
    if (event->button() == Qt::LeftButton && m_panModeActive) {
        m_dragging = true;
        m_lastMousePos = event->pos();
        m_autoFit = false;
        emit autoFitChanged(false);
        setCursor(Qt::ClosedHandCursor);
        return;
    }
    if (event->button() == Qt::LeftButton && m_overlaysVisible) {
        // Check if clicking on a function
        int funcIndex = findFunctionAt(event->pos());
        if (funcIndex >= 0) {
            setSelectedRoi(funcIndex);  // Select the ROI that owns this function
            m_draggingFunctionIndex = funcIndex;
            // Find the specific function being clicked
            m_draggingFunction = m_rois[funcIndex]->findFunctionAt(event->pos());
            if (m_draggingFunction) {
                m_functionDragStart = event->pos() - m_draggingFunction->position();
            }
        }
        // Check if clicking inside existing ROI
        else {
            int roiIndex = findRoiAt(event->pos());
            if (roiIndex >= 0) {
                setSelectedRoi(roiIndex);  // Select the clicked ROI
                // Check if clicking on a resize handle first
                int resizeHandle = findResizeHandle(event->pos(), roiIndex);
                if (resizeHandle != None) {
                    m_resizingRoiIndex = roiIndex;
                    m_resizeHandle = resizeHandle;
                    m_roiOriginal = m_rois[roiIndex]->qRect();
                    m_roiMoveStart = event->pos();
                    setCursor(getResizeCursor(resizeHandle));
                } else {
                    // Normal move operation
                    m_movingRoiIndex = roiIndex;
                    m_roiMoveStart = event->pos();
                    m_roiOriginal = m_rois[roiIndex]->qRect();
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
        m_backgroundDirty = true; // Mark background dirty when panning
        scheduleUpdate();
    }
    if (m_draggingFunctionIndex >= 0 && m_draggingFunction) {
        QPoint newPosition = event->pos() - m_functionDragStart;
        
        // Clamp function position to stay within widget bounds
        const int functionWidth = 256;
        const int functionHeight = 100;
        
        newPosition.setX(qMax(0, qMin(newPosition.x(), width() - functionWidth)));
        newPosition.setY(qMax(0, qMin(newPosition.y(), height() - functionHeight)));
        
        // Update the function position directly
        m_draggingFunction->setPosition(newPosition);
        scheduleUpdate();
    }
    if (m_movingRoiIndex >= 0) {
        // Calculate total delta from the start of the move operation
        QPoint totalDelta = event->pos() - m_roiMoveStart;
        QPointF imageDelta = QPointF(totalDelta) / m_scaleFactor;
        
        // Apply delta to original ROI position
        m_rois[m_movingRoiIndex]->setRoi(m_roiOriginal.translated(imageDelta.toPoint()));
        scheduleUpdate();
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
        
        m_rois[m_resizingRoiIndex]->setRoi(newRoi.normalized());
        
        // Show tooltip with ROI size during resize
        QToolTip::showText(event->globalPosition().toPoint(),
                           QString("%1x%2").arg(newRoi.width()).arg(newRoi.height()), this);
        
        scheduleUpdate();
    }
    if (m_drawingRoi) {
        m_roiWidget.setBottomRight(event->pos());
        QRect normalizedRoi = m_roiWidget.normalized();
        QPointF imgStart = (normalizedRoi.topLeft() - m_imageOffset) / m_scaleFactor;
        QPointF imgEnd   = (normalizedRoi.bottomRight() - m_imageOffset) / m_scaleFactor;
        QRect imageRoi = QRect(imgStart.toPoint(), imgEnd.toPoint()).normalized();
        QToolTip::showText(event->globalPosition().toPoint(),
                           QString("%1x%2").arg(imageRoi.width()).arg(imageRoi.height()), this);
        scheduleUpdate();
    }
    QWidget::mouseMoveEvent(event);
}

void ImageWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton && m_dragging && m_panModeActive) {
        m_dragging = false;
        setCursor(m_panModeActive ? Qt::OpenHandCursor : Qt::ArrowCursor);
    }
    if (event->button() == Qt::LeftButton && m_draggingFunctionIndex >= 0) {
        m_draggingFunctionIndex = -1;
        m_draggingFunction = nullptr;
    }
    if (event->button() == Qt::LeftButton && m_movingRoiIndex >= 0) {
        m_movingRoiIndex = -1;
    }
    if (event->button() == Qt::LeftButton && m_resizingRoiIndex >= 0) {
        m_resizingRoiIndex = -1;
        m_resizeHandle = None;
        setCursor(Qt::ArrowCursor);
        QToolTip::hideText(); // Hide tooltip when resize is finished
        
        // Sort ROIs by size after resize to maintain proper interaction order
        sortRoisBySize();
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
            
            // Create new ROI data (without any functions initially)
            Roi *roi = new Roi();
            roi->setRoi(imageRoi);
            roi->setId(m_nextRoiId++);
            m_rois.append(roi);
            
            // Sort ROIs by size to ensure smaller ones are clickable
            sortRoisBySize();
            
            // Select the newly created ROI
            // After sorting, find the new ROI by ID
            for (int i = 0; i < m_rois.size(); ++i) {
                if (m_rois[i]->id() == m_nextRoiId - 1) {
                    setSelectedRoi(i);
                    break;
                }
            }
        }
        
        update();
    }
    QWidget::mouseReleaseEvent(event);
}

QRect ImageWidget::rasterizedImageRect() const 
{
    if (!m_rois.isEmpty()) {
        // Return the first ROI for compatibility
        QRect roi = m_rois.first()->qRect();
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
    // Search from smallest to largest (forward order) to prioritize smaller ROIs for mouse interaction
    for (int i = 0; i < m_rois.size(); ++i) {
        QRect widgetRoi = widgetRectFromImageRect(m_rois[i]->qRect());
        if (widgetRoi.contains(pos)) {
            return i;
        }
    }
    return -1;
}

int ImageWidget::findFunctionAt(const QPoint &pos) const
{
    // Search from smallest to largest ROI (forward order) to prioritize smaller ROIs
    for (int i = 0; i < m_rois.size(); ++i) {
        const Function* function = m_rois[i]->findFunctionAt(pos);
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
        // Check all functions in this ROI
        for (const auto& function : m_rois[i]->getFunctions()) {
            if (function) {
                QRect existingRect(function->position(), QSize(functionWidth, functionHeight));
                QRect newRect(basePos, QSize(functionWidth, functionHeight));
                
                if (existingRect.intersects(newRect)) {
                    // Move down by function height + spacing
                    basePos.setY(basePos.y() + functionHeight + 10);
                    i = -1; // Restart check from beginning
                    break; // Break inner loop to restart outer loop
                }
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
        Roi* roi = m_rois[index];
        
        // Notify about widget removal for all functions before deleting ROI
        const auto& functions = roi->getFunctions();
        for (const auto& function : functions) {
            QWidget *widget = function->widget();
            if (widget) {
                emit functionWidgetRemoved(widget);
            }
        }
        
        m_rois.removeAt(index);
        
        // Update active indices if they point to removed or shifted ROIs
        if (m_movingRoiIndex == index) {
            m_movingRoiIndex = -1;
        } else if (m_movingRoiIndex > index) {
            m_movingRoiIndex--;
        }
        
        if (m_draggingFunctionIndex == index) {
            m_draggingFunctionIndex = -1;
            m_draggingFunction = nullptr;
        } else if (m_draggingFunctionIndex > index) {
            m_draggingFunctionIndex--;
        }
        
        if (m_resizingRoiIndex == index) {
            m_resizingRoiIndex = -1;
        } else if (m_resizingRoiIndex > index) {
            m_resizingRoiIndex--;
        }
        
        if (m_selectedRoiIndex == index) {
            setSelectedRoi(-1);  // Deselect if deleted
        } else if (m_selectedRoiIndex > index) {
            m_selectedRoiIndex--;  // Adjust index without emitting signal
        }
    }
}

int ImageWidget::findResizeHandle(const QPoint &pos, int roiIndex) const
{
    if (roiIndex < 0 || roiIndex >= m_rois.size()) {
        return None;
    }

    QRect widgetRoi = widgetRectFromImageRect(m_rois[roiIndex]->qRect());
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
        // Validate positions for all functions
        for (const auto& function : roi->getFunctions()) {
            if (function) {
                QPoint pos = function->position();
                
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
                function->setPosition(pos);
            }
        }
    }
}

void ImageWidget::saveProject(const QString &filePath)
{
    if (!m_projectManager.saveProject(filePath, m_rois, m_autoFit, m_imageOffset, m_scaleFactor, m_maxImageCount)) {
        // Handle error - could emit a signal or show a message
        qWarning() << "Failed to save project to" << filePath;
    }
}

void ImageWidget::loadProject(const QString &filePath)
{
    QList<Roi*> loadedRois;
    int nextRoiId;
    bool autoFit;
    QPoint imageOffset;
    double scaleFactor;
    int maxImageCount;
    
    if (m_projectManager.loadProject(filePath, loadedRois, nextRoiId, autoFit, imageOffset, scaleFactor, maxImageCount)) {
        m_rois = loadedRois;
        m_nextRoiId = nextRoiId;
        
        // Apply loaded view settings
        setImagePosition(imageOffset, scaleFactor);
        m_autoFit = autoFit;
        emit autoFitChanged(autoFit);
        
        // Apply loaded max image count
        setMaxImageCount(maxImageCount);
        
        // Only validate function positions if an image has been received
        // This prevents setting wrong positions when widget size is not yet stable
        if (m_imageReceived) {
            validateFunctionPositions();
        }
        
        // Sort ROIs by size for proper interaction
        sortRoisBySize();
        
        // Initialize functions after project load (e.g., open dialogs for functions with existing data)
        initializeFunctionsAfterProjectLoad();
        
        update();
    } else {
        // Handle error - could emit a signal or show a message
        qWarning() << "Failed to load project from" << filePath;
    }
}

void ImageWidget::contextMenuEvent(QContextMenuEvent *event)
{
    if (m_overlaysVisible) {
        int roiIndex = findRoiAt(event->pos());
        if (roiIndex >= 0) {
            showRoiContextMenu(event->globalPos(), roiIndex);
            return;
        }
    }
    QWidget::contextMenuEvent(event);
}

void ImageWidget::showRoiContextMenu(const QPoint &pos, int roiIndex)
{
    if (roiIndex < 0 || roiIndex >= m_rois.size()) {
        return;
    }
    
    m_contextMenuRoiIndex = roiIndex;
    Roi *roi = m_rois[roiIndex];
    
    QMenu contextMenu(this);
    
    // Dynamically create menu entries for all registered plugins
    FunctionRegistry &registry = FunctionRegistry::instance();
    QStringList pluginIds = registry.availablePlugins();
    
    for (const QString &pluginId : pluginIds) {
        auto plugin = registry.getPlugin(pluginId);
        if (plugin) {
            QAction *action = contextMenu.addAction(plugin->displayName());
            action->setCheckable(true);
            action->setChecked(roi->hasFunction(pluginId));
            
            // Capture pluginId by value in lambda
            connect(action, &QAction::triggered, [this, pluginId]() {
                bool wasChecked = m_rois[m_contextMenuRoiIndex]->hasFunction(pluginId);
                toggleFunction(m_contextMenuRoiIndex, pluginId);
                
                // Initialize newly added function (e.g., auto-open dialogs)
                if (!wasChecked && m_rois[m_contextMenuRoiIndex]->hasFunction(pluginId)) {
                    Function* function = m_rois[m_contextMenuRoiIndex]->getFunction(pluginId);
                    if (function) {
                        // Force immediate calculation to trigger any initialization behavior
                        cv::Rect roiRect = m_rois[m_contextMenuRoiIndex]->rect();
                        if (!m_image.empty() && !roiRect.empty()) {
                            function->calculate(m_image, roiRect);
                        }
                    }
                }
            });
        }
    }
    
    // Separator
    contextMenu.addSeparator();
    
    // Delete action
    QAction *deleteAction = contextMenu.addAction("Löschen");
    connect(deleteAction, &QAction::triggered, [this]() {
        removeRoi(m_contextMenuRoiIndex);
        update();
    });
    
    contextMenu.exec(pos);
    m_contextMenuRoiIndex = -1;
}

void ImageWidget::toggleFunction(int roiIndex, const QString &typeId)
{
    if (roiIndex < 0 || roiIndex >= m_rois.size()) {
        return;
    }
    
    Roi *roi = m_rois[roiIndex];
    
    if (roi->hasFunction(typeId)) {
        Function* function = roi->getFunction(typeId);
        if (function) {
            QWidget* widget = function->widget();
            if (widget) {
                emit functionWidgetRemoved(widget);
            }
        }
        roi->removeFunction(typeId);

    } else {
        // Add the function using the registry
        std::unique_ptr<Function> function = FunctionRegistry::instance().createFunction(typeId);
        if (function) {
            QPoint position = getNextFunctionPosition();
            function->setPosition(position);
            roi->addFunction(std::move(function));
            
            // Notify about widget creation if the function has one
            Function* addedFunction = roi->getFunction(typeId);
            if (addedFunction && addedFunction->widget()) {
                emit functionWidgetCreated(addedFunction->widget(), addedFunction->title());
            }
        }
    }
    
    setSelectedRoi(roiIndex);
    update();
}

void ImageWidget::setImagePosition(const QPoint &offset, double scaleFactor)
{
    m_imageOffset = offset;
    m_scaleFactor = scaleFactor;
    update();
}

void ImageWidget::updateBackgroundCache()
{
    if (m_image.empty() || width() <= 0 || height() <= 0) {
        return;
    }
    
    // Create cached background pixmap
    m_cachedBackground = QPixmap(size());
    m_cachedBackground.fill(palette().color(QPalette::Window));
    
    QPainter cachePainter(&m_cachedBackground);
    cachePainter.setRenderHint(QPainter::SmoothPixmapTransform);
    
    // Draw the scaled and positioned image to cache
    cachePainter.translate(m_imageOffset);
    cachePainter.scale(m_scaleFactor, m_scaleFactor);
    cachePainter.drawImage(0, 0, qImage());
    
    m_backgroundDirty = false;
}

void ImageWidget::scheduleUpdate()
{
    // Throttle updates during interactive operations to reduce flicker
    const int updateInterval = 16; // ~60 FPS max
    
    if (m_lastPaintUpdate.elapsed() >= updateInterval) {
        doUpdate();

    } else if (!m_updatePending) {
        // Schedule a delayed update
        m_updatePending = true;
        QTimer::singleShot(updateInterval - m_lastPaintUpdate.elapsed(), this, [this]() {
            if (m_updatePending) {
                doUpdate();
            }
        });
    }
}

void ImageWidget::doUpdate()
{
    update();
    setSelectedRoi(m_selectedRoiIndex); 
    m_lastPaintUpdate.restart();
    m_updatePending = false;
}

void ImageWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    
    // Check if double-click is on a function widget (prioritize smaller ROIs)
    for (int i = 0; i < m_rois.size(); ++i) {
        const Roi *roi = m_rois[i];
        
        for (const auto &function : roi->getFunctions()) {
            if (function->containsPoint(event->pos())) {
                // Handle different function types
                // Future: Add handling for function types that support interactive widgets
                return; // Exit early if we found a function to interact with
            }
        }
    }
    
    // If not handled by functions, call base implementation
    QWidget::mouseDoubleClickEvent(event);
}

QWidget* ImageWidget::getTopLevelWidget() const
{
    QWidget *topLevelWidget = const_cast<ImageWidget*>(this);
    while (topLevelWidget->parentWidget()) {
        topLevelWidget = topLevelWidget->parentWidget();
    }
    return topLevelWidget;
}

void ImageWidget::initializeFunctionsAfterProjectLoad()
{
    // Go through all ROIs and their functions to emit signals for widget creation
    for (int i = 0; i < m_rois.size(); ++i) {
        const Roi *roi = m_rois[i];

        for (const auto &function : roi->getFunctions()) {
            // Emit signal for widget creation if function has a widget
            if (function->widget()) {
                emit functionWidgetCreated(function->widget(), function->title());
            }
        }
    }
}

void ImageWidget::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_panModeActive = true;
        setCursor(Qt::OpenHandCursor);
        event->accept();
        return;
    }
    if (event->key() == Qt::Key_O && !event->isAutoRepeat()) {
        m_overlaysVisible = !m_overlaysVisible;
        update(); // Trigger repaint to show/hide overlays
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void ImageWidget::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        m_panModeActive = false;
        if (m_dragging) {
            m_dragging = false;
        }
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    QWidget::keyReleaseEvent(event);
}

void ImageWidget::sortRoisBySize()
{
    if (m_rois.size() <= 1) {
        return; // Nothing to sort
    }
    
    // Store current active indices to update them after sorting
    int originalMovingIndex = m_movingRoiIndex;
    int originalDraggingIndex = m_draggingFunctionIndex;
    int originalResizingIndex = m_resizingRoiIndex;
    int originalContextIndex = m_contextMenuRoiIndex;
    
    std::vector<int> indices(m_rois.size());
    std::iota(indices.begin(), indices.end(), 0);
    
    // Sort indices by ROI area (smallest first)
    std::sort(indices.begin(), indices.end(), [this](int a, int b) {
        return m_rois[a]->area() < m_rois[b]->area();
    });
    
    // Create mapping from old index to new index
    std::vector<int> indexMapping(m_rois.size());
    for (int newIndex = 0; newIndex < indices.size(); ++newIndex) {
        int oldIndex = indices[newIndex];
        indexMapping[oldIndex] = newIndex;
    }
    
    // Reorder ROIs by moving them to correct positions (no cloning!)
    QList<Roi*> tempRois(m_rois.size());
    for (int i = 0; i < indices.size(); ++i) {
        tempRois[i] = m_rois[indices[i]];
    }
    m_rois = tempRois;
    
    // Update active indices to point to the new positions
    if (originalMovingIndex >= 0 && originalMovingIndex < indexMapping.size()) {
        m_movingRoiIndex = indexMapping[originalMovingIndex];
    }
    if (originalDraggingIndex >= 0 && originalDraggingIndex < indexMapping.size()) {
        m_draggingFunctionIndex = indexMapping[originalDraggingIndex];
    }
    if (originalResizingIndex >= 0 && originalResizingIndex < indexMapping.size()) {
        m_resizingRoiIndex = indexMapping[originalResizingIndex];
    }
    if (originalContextIndex >= 0 && originalContextIndex < indexMapping.size()) {
        m_contextMenuRoiIndex = indexMapping[originalContextIndex];
    }
    if (m_selectedRoiIndex >= 0 && m_selectedRoiIndex < indexMapping.size()) {
        m_selectedRoiIndex = indexMapping[m_selectedRoiIndex];
    }
}

void ImageWidget::setSelectedRoi(int index)
{
    m_selectedRoiIndex = index;
    if (index >= 0 && index < m_rois.size()) {
        emit roiSelectionChanged(m_rois[index]);

    } else {
        emit roiSelectionChanged(nullptr);
    }
    update();
}

void ImageWidget::updateFunctions()
{
    // Skip if no image is available
    if (m_image.empty()) {
        return;
    }
    
    // Recalculate all ROI functions with current image
    for (int i = 0; i < m_rois.size(); ++i) {
        Roi* roi = m_rois[i];
        if (!roi) {
            continue;
        }
        
        // Get ROI rectangle in image coordinates
        cv::Rect imageRoi = roi->rect();
        
        // Ensure ROI is within image bounds and has valid dimensions
        if (imageRoi.width > 0 && imageRoi.height > 0) {
            // Recalculate all functions for this ROI
            roi->calculateFunctions(m_image, imageRoi);
        }
    }
    
    // Trigger visual update
    update();
}

void ImageWidget::setMaxImageCount(int count)
{
    if (count < 1) {
        count = 1;
    }
    
    m_maxImageCount = count;
    
    // Remove excess images if new max is smaller
    while (m_imageBuffer.size() > m_maxImageCount) {
        m_imageBuffer.removeFirst();
        
        // Adjust current index if needed
        if (m_currentImageIndex >= m_imageBuffer.size()) {
            m_currentImageIndex = m_imageBuffer.size() - 1;
        }
    }
    
    // Update current image from buffer if we have a valid selection
    if (m_currentImageIndex >= 0 && m_currentImageIndex < m_imageBuffer.size()) {
        const ImageBufferEntry &entry = m_imageBuffer[m_currentImageIndex];
        m_image = entry.image.clone();
        m_qImageValid = false;
        m_backgroundDirty = true;
        updateFunctions();
        update();
        emit currentImageIndexChanged(m_currentImageIndex, entry.sequence, entry.timestamp);
    }
    
    emit imageCountChanged(m_imageBuffer.size(), m_maxImageCount);
}

void ImageWidget::selectImage(int index)
{
    if (index < 0 || index >= m_imageBuffer.size()) {
        return;
    }
    
    m_currentImageIndex = index;
    
    const ImageBufferEntry &entry = m_imageBuffer[index];
    m_image = entry.image.clone();
    m_qImageValid = false;
    m_backgroundDirty = true;
    
    // Recalculate all functions with the selected image
    updateFunctions();
    
    // Trigger visual update
    update();
    
    emit currentImageIndexChanged(index, entry.sequence, entry.timestamp);
}

int ImageWidget::maxImageCount() const
{
    return m_maxImageCount;
}

int ImageWidget::currentImageIndex() const
{
    return m_currentImageIndex;
}

int ImageWidget::imageBufferSize() const
{
    return m_imageBuffer.size();
}

unsigned int ImageWidget::currentSequence() const
{
    if (m_currentImageIndex >= 0 && m_currentImageIndex < m_imageBuffer.size()) {
        return m_imageBuffer[m_currentImageIndex].sequence;
    }
    return 0;
}

unsigned long ImageWidget::currentTimestamp() const
{
    if (m_currentImageIndex >= 0 && m_currentImageIndex < m_imageBuffer.size()) {
        return m_imageBuffer[m_currentImageIndex].timestamp;
    }
    return 0;
}
