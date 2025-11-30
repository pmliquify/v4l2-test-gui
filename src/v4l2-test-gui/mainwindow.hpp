#pragma once

#include <QtWidgets>
#include <opencv2/opencv.hpp>
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

private slots:
    void onImageReceived(const Image &image);
    void onDisconnected();
    void setShowRawImage(bool checked);
    void saveImage();
    void setAllwaysOnTop(bool checked);
    void increaseStride();
    void decreaseStride();
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void openNewWindow();
    void updateImageNavigationUI(int current, int max);
    void updateStatusBarImageInfo(int index, unsigned int sequence, unsigned long timestamp);

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
    int             m_strideOffset;
    ImageWidget*    m_imageWidget;
    QString         m_lastDir;
    QString         m_currentProjectFile;
    QSlider*        m_imageSlider;
    QSpinBox*       m_imageCountSpinBox;

    void setupStatusBar();
    void loadSettings();
    void saveSettings();
    void updateImageInfo(const Image &image);
    void updateConnectionStatus(bool connected);
    void loadLastProject();
    void createFunctionDock(QWidget* functionWidget, const QString& title);
    void removeFunctionDock(QWidget* functionWidget);
    void setupImageNavigationBar();

protected:
    void closeEvent(QCloseEvent *event) override;
};