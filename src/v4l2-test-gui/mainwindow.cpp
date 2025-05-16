#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include "convert.hpp"
#include <version.h>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    m_server(this),
    m_connected(false),
    m_connectionStatus(nullptr),
    m_port(nullptr),
    m_imageReceived(false),
    m_imageConverted(false),
    m_fpsSequence(0),
    m_fpsTimestamp(0),
    m_fps(0.0),
    m_showRawImage(false)
{
    ui->setupUi(this);
    setWindowTitle(tr("v4l2-test-gui (%1)").arg(V4L2TEST_VERSION));
    resize(800, 600);
    setupStatusBar();

    m_imageWidget = new ImageWidget(this);
    setCentralWidget(m_imageWidget);

    connect(ui->actionFitToWidget, &QAction::triggered, m_imageWidget, &ImageWidget::fitImageToWidget);
    connect(m_imageWidget, &ImageWidget::autoFitChanged, ui->actionFitToWidget, &QAction::setChecked);
    ui->actionFitToWidget->setChecked(m_imageWidget->isAutoFit());
    
    connect(ui->actionSaveImage, &QAction::triggered, this, &MainWindow::saveImage);
    connect(ui->actionShowRaw, &QAction::toggled, this, &MainWindow::setShowRawImage);
    connect(ui->actionAllwaysOnTop, &QAction::toggled, this, &MainWindow::setAllwaysOnTop);
    connect(&m_server, &SocketServer::imageReceived, this, &MainWindow::onImageReceived);
    connect(&m_server, &SocketServer::disconnected, this, &MainWindow::onDisconnected);
    
    updateConnectionStatus(false);
}

MainWindow::~MainWindow()
{
    delete ui;
}

QImage MainWindow::image() const 
{
    return m_imageWidget->image();
}

void MainWindow::setImage(const QImage &image) 
{
    m_imageWidget->setImage(image);
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
    m_imageReceived = true;
    cv::Mat cvImage = convert(image, m_showRawImage);
    m_imageConverted = !cvImage.empty();
    if (m_imageConverted) {
        setImage(cvMatToQImage(cvImage));
        
    } else {
        update();
    }
    
    if (!m_connected) {
        m_fpsSequence = image.sequence();
        m_fpsTimestamp = image.timestamp();
    }
    if (image.timestamp() - m_fpsTimestamp > 1000) {
        m_fps = (image.sequence() - m_fpsSequence) * 1000.0 / (image.timestamp() - m_fpsTimestamp);
        m_fpsSequence = image.sequence();
        m_fpsTimestamp = image.timestamp();
    }

    updateImageInfo(image);
    updateConnectionStatus(true);
    m_imageWidget->setImageReceived(m_imageReceived);
    m_imageWidget->setImageConverted(m_imageConverted);
}

void MainWindow::onDisconnected()
{
    updateConnectionStatus(false);
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

void MainWindow::setAllwaysOnTop(bool checked)
{
    Qt::WindowFlags flags = checked ? 
        windowFlags() | Qt::WindowStaysOnTopHint : 
        windowFlags() & ~Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);
    show();
}

void MainWindow::setupStatusBar()
{
    m_port = new QSpinBox(this);
    m_port->setPrefix(tr("Port: "));
    m_port->setFocusPolicy(Qt::NoFocus);
    m_port->setRange(1, 65535);
    m_port->setValue(9000);
    connect(m_port, QOverload<int>::of(&QSpinBox::valueChanged), 
        [this](int port) { m_server.listen(port); });
    statusBar()->addPermanentWidget(m_port);
    m_server.listen(m_port->value());

    m_connectionStatus = new QLabel(this);
    m_connectionStatus->setAlignment(Qt::AlignCenter);
    statusBar()->addPermanentWidget(m_connectionStatus);
}

void MainWindow::updateImageInfo(const Image &image)
{
    setWindowTitle(tr("%1x%2, %3, line: %4 bytes, size: %5 bytes")
        .arg(image.width()).arg(image.height()).arg(pixelFormat(image))
        .arg(image.bytesPerLine()).arg(image.imageSize()));
    statusBar()->showMessage(tr("%1 fps - #%2 - ts: %3 ms")
        .arg(m_fps, 0, 'f', 1)
        .arg(image.sequence(), 5, 10, QChar('0')).arg(image.timestamp(), 8));
}

void MainWindow::updateConnectionStatus(bool connected)
{
    m_connected = connected;
    m_connectionStatus->setText(connected ? tr("Connected") : tr("Disconnected"));
    m_connectionStatus->setPixmap(
        QPixmap(connected ? ":/icons/connected.png" : ":/icons/disconnected.png"));
}