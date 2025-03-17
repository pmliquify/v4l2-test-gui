#include "window.hpp"


Window::Window(QWidget *parent)
    : QMainWindow(parent),
      m_scaleFactor(1.0),
      m_dragging(false),
      m_imageOffset(0, 0),
      m_drawingRect(false)
{
}

void Window::paintEvent(QPaintEvent *event) 
{
    Q_UNUSED(event);
    QPainter painter(this);
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

void Window::wheelEvent(QWheelEvent *event) 
{
    int delta = event->angleDelta().y();
    double factor = (delta > 0) ? 1.1 : 0.9;
    m_scaleFactor *= factor;

    QPoint mousePos = event->position().toPoint();
    m_imageOffset = (m_imageOffset - mousePos) * factor + mousePos;

    update();
}

void Window::mouseMoveEvent(QMouseEvent *event) 
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
                           QString("%1x%2")
                           .arg(imageRect.width())
                           .arg(imageRect.height()), this);
        update();
    }
    QMainWindow::mouseMoveEvent(event);
}

void Window::mousePressEvent(QMouseEvent *event) 
{
    if (event->button() == Qt::RightButton) {
        m_dragging = true;
        m_lastMousePos = event->pos();
    }
    if (event->button() == Qt::LeftButton) {
        m_drawingRect = true;
        m_rectStart = event->pos();
        m_rectEnd = m_rectStart;
    }
    QMainWindow::mousePressEvent(event);
}

void Window::mouseReleaseEvent(QMouseEvent *event) 
{
    if (event->button() == Qt::RightButton && m_dragging) {
        m_dragging = false;
    }
    if (event->button() == Qt::LeftButton && m_drawingRect) {
        m_drawingRect = false;
        QToolTip::hideText();
        update();
    }
    QMainWindow::mouseReleaseEvent(event);
}

QImage Window::image() const 
{
    return m_image;
}

void Window::setImage(const QImage &image) 
{
    bool fitToWindow = m_image.size() != image.size();
    m_image = image;
    if (fitToWindow) {
        fitImageToWindow();
    } else {
        update();
    }
}

void Window::fitImageToWindow() 
{
    double widthRatio = (double)width() / m_image.width();
    double heightRatio = (double)height() / m_image.height();
    m_scaleFactor = qMin(widthRatio, heightRatio);
    m_imageOffset = QPoint(0, 0);
    update();
}

QRect Window::rasterizedImageRect() const 
{
    QPointF imgStart = (m_rectStart - m_imageOffset) / m_scaleFactor;
    QPointF imgEnd   = (m_rectEnd   - m_imageOffset) / m_scaleFactor;
    QRect rect = QRect(imgStart.toPoint(), imgEnd.toPoint()).normalized();
    if (rect.height() > 0) {
        rect.setHeight(rect.height() - 1);
    }
    return rect;
}

QRect Window::widgetRectFromImageRect(const QRect &imageRect) const 
{
    QPoint p1(qRound(imageRect.x() * m_scaleFactor) + m_imageOffset.x(),
              qRound(imageRect.y() * m_scaleFactor) + m_imageOffset.y());
    QPoint p2(qRound((imageRect.x() + imageRect.width()) * m_scaleFactor) + m_imageOffset.x(),
              qRound((imageRect.y() + imageRect.height()) * m_scaleFactor) + m_imageOffset.y());
    return QRect(p1, p2);
}
