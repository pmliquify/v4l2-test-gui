#include "window.hpp"


Window::Window(QWidget *parent)
    : QMainWindow(parent),
      m_scaleFactor(1.0),
      m_dragging(false),
      m_imageOffset(0, 0)
{
}

void Window::paintEvent(QPaintEvent *event) 
{
    Q_UNUSED(event);
    QPainter painter(this);
    // painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    painter.translate(m_imageOffset);
    painter.scale(m_scaleFactor, m_scaleFactor);
    painter.drawImage(0, 0, m_image);
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
    QMainWindow::mouseMoveEvent(event);
}

void Window::mousePressEvent(QMouseEvent *event) 
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = true;
        m_lastMousePos = event->pos();
    }
    QMainWindow::mousePressEvent(event);
}

void Window::mouseReleaseEvent(QMouseEvent *event) 
{
    if (event->button() == Qt::LeftButton) {
        m_dragging = false;
    }
    QMainWindow::mouseReleaseEvent(event);
}

QImage Window::image() const 
{
    return m_image;
}

void Window::setImage(const QImage &image) 
{
    m_image = image;
    update();
}

void Window::fitImageToWindow() 
{
    m_scaleFactor = (double)width() / m_image.width();
    m_imageOffset = QPoint(0, 0);
    update();
}
