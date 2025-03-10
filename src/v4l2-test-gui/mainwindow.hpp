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
    void onDisconnected();
    void setShowRawImage(bool checked);
    void saveImage();
    void setAllwaysOnTop(bool checked);

private:
    Ui::MainWindow* ui;
    SocketServer    m_server;
    bool            m_connected;
    QLabel*         m_connectionStatus;
    QSpinBox*       m_port;
    bool            m_imageReceived;
    int             m_fpsSequence;
    int             m_fpsTimestamp;
    double          m_fps;
    bool            m_showRawImage;

    void setupPort();
    void setConnectionStatus(bool connected);
};