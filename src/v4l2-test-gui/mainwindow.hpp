#pragma once

#include "window.hpp"
#include "socketserver.hpp"

namespace Ui {
    class MainWindow;
}

class MainWindow : public Window 
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

protected:
    void paintEvent(QPaintEvent *event) override;

private slots:
    void onImageReceived(const Image &image);
    void setShowRawImage(bool checked);
    void saveImage();

private:
    Ui::MainWindow* ui;
    SocketServer    m_server;
    int             m_fpsSequence;
    int             m_fpsTimestamp;
    double          m_fps;
    bool            m_imageReceived;
    bool            m_showRawImage;
};