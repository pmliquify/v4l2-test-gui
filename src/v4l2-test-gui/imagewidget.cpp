#include "imagewidget.hpp"


ImageWidget::ImageWidget(QWidget *parent)
    : QWidget(parent),
      m_scaleFactor(1.0),
      m_dragging(false),
      m_imageOffset(0, 0),
      m_drawingRect(false),
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
    m_image = image;
    if (fitToWindow) {
        fitImageToWidget();

    } else {
        update();
    }
}

void ImageWidget::fitImageToWidget() 
{
    m_autoFit = true;
    if (m_image.isNull()) {
        m_scaleFactor = 1.0;
        m_imageOffset = QPoint(0, 0);
        update();
        return;
    }
    double widthRatio = (double)width() / m_image.width();
    double heightRatio = (double)height() / m_image.height();
    m_scaleFactor = qMin(widthRatio, heightRatio);
    QSize scaledSize = m_image.size() * m_scaleFactor;
    int offsetX = (width() - scaledSize.width()) / 2;
    int offsetY = (height() - scaledSize.height()) / 2;
    m_imageOffset = QPoint(offsetX, offsetY);
    update();
}

void ImageWidget::resizeEvent(QResizeEvent *event)
{
    if (m_autoFit) {
        // fitImageToWidget setzt m_autoFit erneut, daher temporär deaktivieren
        bool oldAutoFit = m_autoFit;
        m_autoFit = false;
        fitImageToWidget();
        m_autoFit = oldAutoFit;
    } else {
        QWidget::resizeEvent(event);
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
            painter.drawText(rect(), Qt::AlignCenter, tr("./v4l2-test client --ip <host>"));
        }
        return;
    }
    painter.translate(m_imageOffset);
    painter.scale(m_scaleFactor, m_scaleFactor);
    painter.drawImage(0, 0, m_image);

    if (m_drawingRect) {
        QRect imageRect = rasterizedImageRect();
        QRect widgetRect = widgetRectFromImageRect(imageRect);
        painter.resetTransform();
        painter.setPen(QPen(Qt::yellow, 2));
        painter.drawRect(widgetRect);
    }
}

void ImageWidget::wheelEvent(QWheelEvent *event)
{
    m_autoFit = false;
    int delta = event->angleDelta().y();
    double factor = (delta > 0) ? 1.1 : 0.9;
    m_scaleFactor *= factor;
    QPoint mousePos = event->position().toPoint();
    m_imageOffset = (m_imageOffset - mousePos) * factor + mousePos;
    update();
}

void ImageWidget::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        m_autoFit = false;
        m_dragging = true;
        m_lastMousePos = event->pos();
    }
    if (event->button() == Qt::LeftButton) {
        m_drawingRect = true;
        m_rectStart = event->pos();
        m_rectEnd = m_rectStart;
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
    if (m_drawingRect) {
        m_rectEnd = event->pos();
        QRect imageRect = rasterizedImageRect();
        QToolTip::showText(event->globalPosition().toPoint(),
                           QString("%1x%2").arg(imageRect.width()).arg(imageRect.height()), this);
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void ImageWidget::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton && m_dragging) {
        m_dragging = false;
    }
    if (event->button() == Qt::LeftButton && m_drawingRect) {
        m_drawingRect = false;
        QToolTip::hideText();
        update();
    }
    QWidget::mouseReleaseEvent(event);
}

QRect ImageWidget::rasterizedImageRect() const 
{
    QPointF imgStart = (m_rectStart - m_imageOffset) / m_scaleFactor;
    QPointF imgEnd   = (m_rectEnd   - m_imageOffset) / m_scaleFactor;
    QRect rect = QRect(imgStart.toPoint(), imgEnd.toPoint()).normalized();
    if (rect.height() > 0) {
        rect.setHeight(rect.height() - 1);
    }
    return rect;
}

QRect ImageWidget::widgetRectFromImageRect(const QRect &imageRect) const 
{
    QPoint p1(qRound(imageRect.x() * m_scaleFactor) + m_imageOffset.x(),
              qRound(imageRect.y() * m_scaleFactor) + m_imageOffset.y());
    QPoint p2(qRound((imageRect.x() + imageRect.width()) * m_scaleFactor) + m_imageOffset.x(),
              qRound((imageRect.y() + imageRect.height()) * m_scaleFactor) + m_imageOffset.y());
    return QRect(p1, p2);
}
