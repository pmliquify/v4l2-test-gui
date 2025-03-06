#pragma once

#include <QtWidgets>

class Window : public QMainWindow 
{
    Q_OBJECT

public:
    explicit Window(QWidget *parent = nullptr);

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

    QImage image() const;
    void setImage(const QImage &image);
    
public slots:
    void fitImageToWindow();

private:
    QImage      m_image;
    double      m_scaleFactor;
    bool        m_dragging;
    QPoint      m_lastMousePos;
    QPoint      m_imageOffset;
};