#pragma once

#include <QtWidgets>
#include "socketserver.hpp"
#include "imagewidget.hpp"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow 
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

    QImage image() const;
    void setImage(const QImage &image);

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
    bool            m_imageConverted;
    int             m_fpsSequence;
    int             m_fpsTimestamp;
    double          m_fps;
    bool            m_showRawImage;
    ImageWidget*    m_imageWidget;
    QString         m_lastDir;
    
    void setupStatusBar();
    void loadSettings();
    void saveSettings();
    void updateImageInfo(const Image &image);
    void updateConnectionStatus(bool connected);

protected:
    void closeEvent(QCloseEvent *event) override;
};