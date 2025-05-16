#pragma once
#include <QtWidgets>


class ImageWidget : public QWidget 
{
    Q_OBJECT
public:
    explicit ImageWidget(QWidget *parent = nullptr);

    QImage image() const;
    void setImage(const QImage &image);
    void setImageReceived(bool received);
    void setImageConverted(bool converted);

public slots:
    void fitImageToWidget();

protected:
    void paintEvent(QPaintEvent *event) override;
    void wheelEvent(QWheelEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QImage      m_image;
    double      m_scaleFactor;
    bool        m_dragging;
    QPoint      m_lastMousePos;
    QPoint      m_imageOffset;
    bool        m_drawingRect;
    QPoint      m_rectStart;
    QPoint      m_rectEnd;
    bool        m_imageReceived;
    bool        m_imageConverted;
    bool        m_autoFit;

    QRect rasterizedImageRect() const;
    QRect widgetRectFromImageRect(const QRect &imageRect) const;
};
