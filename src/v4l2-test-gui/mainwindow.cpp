#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include "convert.hpp"
#include <version.h>


MainWindow::MainWindow(QWidget *parent) :
    Window(parent),
    ui(new Ui::MainWindow),
    m_imageReceived(false),
    m_showRawImage(false),
    m_fpsSequence(0),
    m_fpsTimestamp(0),
    m_fps(0.0)
{
    ui->setupUi(this);
    setWindowTitle(tr("v4l2-test-gui (%1)").arg(V4L2TEST_VERSION));
    resize(800, 600);

    connect(ui->actionSaveImage, &QAction::triggered, this, &MainWindow::saveImage);
    connect(ui->actionShowRaw, &QAction::toggled, this, &MainWindow::setShowRawImage);
    connect(ui->actionFitToWindow, &QAction::triggered, this, &Window::fitImageToWindow);
    connect(&m_server, &SocketServer::imageReceived, this, &MainWindow::onImageReceived);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::paintEvent(QPaintEvent *event) 
{
    Q_UNUSED(event);
    if (!m_imageReceived) {
        QPainter painter(this);
        painter.setPen(Qt::darkGray);
        painter.setFont(QFont("Arial", 20));
        painter.drawText(rect(), Qt::AlignCenter, tr("./v4l2-test client --ip <host>"));
        return;
    }

    Window::paintEvent(event);
}

QString pixelFormat(const Image &image)
{
    QString format;
    format += QChar(image.pixelformat() >> 0 & 0xFF);
    format += QChar(image.pixelformat() >> 8 & 0xFF);
    format += QChar(image.pixelformat() >> 16 & 0xFF);
    format += QChar(image.pixelformat() >> 24 & 0xFF);
    return format;
}

void MainWindow::onImageReceived(const Image &image) 
{
    cv::Mat cvImage = convert(image, m_showRawImage);
    setImage(cvMatToQImage(cvImage));
    
    if (!m_imageReceived) {
        ui->actionSaveImage->setEnabled(true);
        fitImageToWindow();
        m_fpsSequence = image.sequence();
        m_fpsTimestamp = image.timestamp();
    }
    if (image.timestamp() - m_fpsTimestamp > 1000) {
        m_fps = (image.sequence() - m_fpsSequence) * 1000.0 / (image.timestamp() - m_fpsTimestamp);
        m_fpsSequence = image.sequence();
        m_fpsTimestamp = image.timestamp();
    }

    statusBar()->showMessage(tr("(%1x%2) %3, line: %4 bytes, size: %5 bytes, %6 fps [#%7, ts: %8 ms]")
        .arg(image.width()).arg(image.height()).arg(pixelFormat(image))
        .arg(image.bytesPerLine()).arg(image.imageSize())
        .arg(m_fps, 0, 'f', 1)
        .arg(image.sequence(), 5, 10, QChar('0')).arg(image.timestamp(), 8));
    
    m_imageReceived = true;
}

void MainWindow::setShowRawImage(bool checked)
{
    m_showRawImage = checked;
    update();
}

void MainWindow::saveImage()
{
    if (image().isNull()) {
        return;
    }

    QString fileName = QFileDialog::getSaveFileName(this, 
        tr("Save Image"), QDir::homePath(), tr("Images (*.png *.jpg *.bmp)"));
    if (fileName.isEmpty()) {
        return;
    }

    image().save(fileName);
}