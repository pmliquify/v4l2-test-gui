#include "mainwindow.hpp"
#include "ui_mainwindow.h"
#include "convert.hpp"
#include "roi.hpp"
#include "propertybrowser.hpp"
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
    m_showRawImage(false),
    m_strideOffset(0),
    m_imageWidget(nullptr),
    m_lastDir(QDir::homePath()),
    m_currentProjectFile(""),
    m_imageSlider(nullptr),
    m_imageCountSpinBox(nullptr),
    m_autoSaveEnabled(false),
    m_autoSaveDir(""),
    m_autoSaveStatus(nullptr)
{
    ui->setupUi(this);
    
    setWindowTitle(tr("v4l2-test-gui (%1)").arg(V4L2TEST_VERSION));
    resize(800, 600);
    setupStatusBar();

    m_imageWidget = new ImageWidget(this);
    setCentralWidget(m_imageWidget);
    
    // Setup navigation bar AFTER imageWidget is created
    setupImageNavigationBar();
    
    // Create property browser dock widget
    PropertyBrowser *propertyBrowser = new PropertyBrowser(this);
    QDockWidget *dock = new QDockWidget(tr("Properties"), this);
    dock->setObjectName("PropertiesDock");
    dock->setWidget(propertyBrowser);
    dock->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    connect(m_imageWidget, &ImageWidget::roiSelectionChanged, propertyBrowser, &PropertyBrowser::onRoiSelectionChanged);
    connect(propertyBrowser, &PropertyBrowser::propertyValueChanged, m_imageWidget, &ImageWidget::updateFunctions);
    connect(m_imageWidget, &ImageWidget::functionWidgetCreated, this, &MainWindow::createFunctionDock);
    connect(m_imageWidget, &ImageWidget::functionWidgetRemoved, this, &MainWindow::removeFunctionDock);
    connect(m_imageWidget, &ImageWidget::imageCountChanged, this, &MainWindow::updateImageNavigationUI);
    connect(m_imageWidget, &ImageWidget::currentImageIndexChanged, this, &MainWindow::updateStatusBarImageInfo);

    loadSettings();
    loadLastProject();

    connect(ui->actionFitToWidget, &QAction::triggered, m_imageWidget, &ImageWidget::fitImageToWidget);
    connect(m_imageWidget, &ImageWidget::autoFitChanged, ui->actionFitToWidget, &QAction::setChecked);
    ui->actionFitToWidget->setChecked(m_imageWidget->isAutoFit());
    
    connect(ui->actionNewWindow, &QAction::triggered, this, &MainWindow::openNewWindow);
    connect(ui->actionSaveImage, &QAction::triggered, this, &MainWindow::saveImage);
    connect(ui->actionAutoSaveImage, &QAction::toggled, this, &MainWindow::toggleAutoSaveImage);
    connect(ui->actionNewProject, &QAction::triggered, this, &MainWindow::newProject);
    connect(ui->actionOpenProject, &QAction::triggered, this, &MainWindow::openProject);
    connect(ui->actionSaveProject, &QAction::triggered, this, &MainWindow::saveProject);
    connect(ui->actionSaveProjectAs, &QAction::triggered, this, &MainWindow::saveProjectAs);
    connect(ui->actionShowRaw, &QAction::toggled, this, &MainWindow::setShowRawImage);
    connect(ui->actionAllwaysOnTop, &QAction::toggled, this, &MainWindow::setAllwaysOnTop);
    connect(ui->actionIncreaseStride, &QAction::triggered, this, &MainWindow::increaseStride);
    connect(ui->actionDecreaseStride, &QAction::triggered, this, &MainWindow::decreaseStride);
    connect(&m_server, &SocketServer::imageReceived, this, &MainWindow::onImageReceived);
    connect(&m_server, &SocketServer::disconnected, this, &MainWindow::onDisconnected);

    updateConnectionStatus(false);
}

void MainWindow::openNewWindow()
{
#ifdef Q_OS_MAC
    // Finde das .app-Bundle und öffne es mit 'open -n'
    QString appPath = QCoreApplication::applicationFilePath();
    QDir dir = QFileInfo(appPath).dir();
    while (!dir.isRoot() && dir.dirName() != "MacOS") {
        dir.cdUp();
    }
    if (dir.dirName() == "MacOS") {
        dir.cdUp(); // Contents
        dir.cdUp(); // .app-Bundle
        QString bundlePath = dir.absolutePath();
        QStringList args;
        args << "-n" << bundlePath;
        QProcess::startDetached("open", args);
    } else {
        // Fallback: wie bisher
        QProcess::startDetached(appPath);
    }
#else
    // Linux/Windows: Starte neue Instanz direkt
    QString appPath = QCoreApplication::applicationFilePath();
    QProcess::startDetached(appPath);
#endif
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadSettings()
{
    QSettings settings("v4l2-test-gui", "v4l2-test-gui");
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());
    m_lastDir = settings.value("lastDir", QDir::homePath()).toString();
    m_autoSaveEnabled = settings.value("autoSaveEnabled", false).toBool();
    m_autoSaveDir = settings.value("autoSaveDir", "").toString();
    
    // Restore auto-save state
    if (m_autoSaveEnabled && !m_autoSaveDir.isEmpty() && QDir(m_autoSaveDir).exists()) {
        ui->actionAutoSaveImage->setChecked(true);
        updateAutoSaveStatus();
    } else {
        m_autoSaveEnabled = false;
        m_autoSaveDir.clear();
    }
}

void MainWindow::saveSettings()
{
    QSettings settings("v4l2-test-gui", "v4l2-test-gui");
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());
    settings.setValue("lastDir", m_lastDir);
    settings.setValue("lastProject", m_currentProjectFile);
    settings.setValue("autoSaveEnabled", m_autoSaveEnabled);
    settings.setValue("autoSaveDir", m_autoSaveDir);
}

void MainWindow::loadLastProject()
{
    QSettings settings("v4l2-test-gui", "v4l2-test-gui");
    QString lastProject = settings.value("lastProject", "").toString();
    if (!lastProject.isEmpty() && QFile::exists(lastProject)) {
        m_currentProjectFile = lastProject;
        m_imageWidget->loadProject(lastProject);
    }
}

void MainWindow::newProject()
{
    // Clear current project
    m_currentProjectFile.clear();
    
    // Clear all ROIs (create new empty Roi list and load it)
    QList<Roi> emptyRois;
    QString tempFile = QDir::temp().filePath("empty_project.v4l2proj");
    
    // Create temporary empty project file
    QJsonObject projectData;
    projectData["version"] = "1.0";
    projectData["rois"] = QJsonArray();
    projectData["nextRoiId"] = 0;
    
    QJsonDocument doc(projectData);
    QFile file(tempFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(doc.toJson());
        file.close();
        m_imageWidget->loadProject(tempFile);
        QFile::remove(tempFile); // Clean up temporary file
    }
}

void MainWindow::openProject()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Project"), m_lastDir, tr("v4l2 Project Files (*.v4l2proj)"), nullptr,
        QFileDialog::DontUseNativeDialog);
    
    if (!fileName.isEmpty()) {
        QFileInfo fi(fileName);
        m_lastDir = fi.absolutePath();
        m_currentProjectFile = fileName;
        m_imageWidget->loadProject(fileName);
    }
}

void MainWindow::saveProject()
{
    if (m_currentProjectFile.isEmpty()) {
        saveProjectAs();
    } else {
        m_imageWidget->saveProject(m_currentProjectFile);
    }
}

void MainWindow::saveProjectAs()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Project"), m_lastDir, tr("v4l2 Project Files (*.v4l2proj)"), nullptr,
        QFileDialog::DontUseNativeDialog);
    
    if (!fileName.isEmpty()) {
        QFileInfo fi(fileName);
        m_lastDir = fi.absolutePath();
        m_currentProjectFile = fileName;
        m_imageWidget->saveProject(fileName);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    saveSettings();
    QMainWindow::closeEvent(event);
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
    cv::Mat cvImage = convert(image, m_strideOffset, m_showRawImage);
    m_imageConverted = !cvImage.empty();
    if (m_imageConverted) {
        m_imageWidget->setImage(cvImage, image.sequence(), image.timestamp());
        
        // Auto-save if enabled
        if (m_autoSaveEnabled && !m_autoSaveDir.isEmpty()) {
            QImage qImage = m_imageWidget->qImage();
            if (!qImage.isNull()) {
                // Build filename: sequencenumber_timestamp_YYYYMMDD_hhmmss.png
                QDateTime now = QDateTime::currentDateTime();
                QString fileName = QString("%1_%2_%3.png")
                    .arg(image.sequence(), 5, 10, QChar('0'))
                    .arg(image.timestamp(), 8, 10, QChar('0'))
                    .arg(now.toString("yyyyMMdd_hhmmss"));
                QString fullPath = QDir(m_autoSaveDir).filePath(fileName);
                saveImageToFile(qImage, fullPath);
            }
        }
    } else {
        update();
    }
    m_imageWidget->setImageReceived(m_imageReceived);
    m_imageWidget->setImageConverted(m_imageConverted);

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
    QImage image = m_imageWidget->qImage();
    if (image.isNull()) {
        return;
    }

    // TODO: Check if QFileDialog::DontUseNativeDialog is necessary
    //       In develop branch it is not!
    QString fileName = QFileDialog::getSaveFileName(this, 
        tr("Save Image"), m_lastDir, tr("Images (*.png *.jpg *.bmp)"), nullptr,
        QFileDialog::DontUseNativeDialog);
    if (fileName.isEmpty()) {
        return;
    }

    QFileInfo fi(fileName);
    m_lastDir = fi.absolutePath();
    saveImageToFile(image, fileName);
}

void MainWindow::toggleAutoSaveImage(bool checked)
{
    if (checked) {
        // Open folder selection dialog
        QString dir = QFileDialog::getExistingDirectory(this,
            tr("Select Auto-Save Directory"), 
            m_autoSaveDir.isEmpty() ? m_lastDir : m_autoSaveDir,
            QFileDialog::ShowDirsOnly | QFileDialog::DontUseNativeDialog);
        
        if (dir.isEmpty()) {
            // User cancelled - uncheck the action
            ui->actionAutoSaveImage->setChecked(false);
            return;
        }
        
        m_autoSaveEnabled = true;
        m_autoSaveDir = dir;
        m_lastDir = dir;
    } else {
        // Disable auto-save
        m_autoSaveEnabled = false;
    }
    
    updateAutoSaveStatus();
}

bool MainWindow::saveImageToFile(const QImage& image, const QString& filePath)
{
    return image.save(filePath);
}

void MainWindow::updateAutoSaveStatus()
{
    if (m_autoSaveEnabled && !m_autoSaveDir.isEmpty()) {
        m_autoSaveStatus->setText(tr("Autosave: %1").arg(m_autoSaveDir));
        m_autoSaveStatus->setVisible(true);
    } else {
        m_autoSaveStatus->setVisible(false);
    }
}

void MainWindow::setAllwaysOnTop(bool checked)
{
    Qt::WindowFlags flags = checked ? 
        windowFlags() | Qt::WindowStaysOnTopHint : 
        windowFlags() & ~Qt::WindowStaysOnTopHint;
    setWindowFlags(flags);
    show();
}

void MainWindow::increaseStride()
{
    m_strideOffset++;
    update();
}

void MainWindow::decreaseStride()
{
    m_strideOffset--;
    update();
}

void MainWindow::setupStatusBar()
{
    // Auto-save status label (before port, permanent)
    m_autoSaveStatus = new QLabel(this);
    m_autoSaveStatus->setVisible(false);
    statusBar()->addPermanentWidget(m_autoSaveStatus);
    
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
        .arg(image.bytesPerLine() + m_strideOffset).arg(image.size()));
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

void MainWindow::createFunctionDock(QWidget* functionWidget, const QString& title)
{
    if (qobject_cast<QDockWidget*>(functionWidget->parentWidget())) {
        return; // Widget already has a dock
    }
    
    // Create dock widget for the function widget
    QDockWidget *dockWidget = new QDockWidget(title, this);
    dockWidget->setObjectName(title + "Dock");
    dockWidget->setWidget(functionWidget);
    dockWidget->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
    dockWidget->setMinimumHeight(50);
    
    // Position it in the right dock area, below the property browser
    addDockWidget(Qt::RightDockWidgetArea, dockWidget);
    
    // Calculate half height of the available area for the right dock
    QList<QDockWidget*> rightDocks;
    for (QObject* obj : children()) {
        QDockWidget* dock = qobject_cast<QDockWidget*>(obj);
        if (dock && dockWidgetArea(dock) == Qt::RightDockWidgetArea) {
            rightDocks.append(dock);
        }
    }
    
    // Get the available height for the dock area
    int availableHeight = height() - statusBar()->height() - menuBar()->height();
    
    // Set the preferred height to half of the available height, distributed among docks
    if (!rightDocks.isEmpty()) {
        int preferredHeight = availableHeight / 2;
        
        // Resize the dock proportionally
        QList<int> sizes;
        for (QDockWidget* dock : rightDocks) {
            sizes.append(preferredHeight / rightDocks.size());
        }
        resizeDocks(rightDocks, sizes, Qt::Vertical);
    }
}

void MainWindow::removeFunctionDock(QWidget* functionWidget)
{
    Q_ASSERT(functionWidget);

    QDockWidget* dockWidget = qobject_cast<QDockWidget*>(functionWidget->parentWidget());
    if (dockWidget) {
        // Remove widget from dock before deleting dock
        dockWidget->setWidget(nullptr);
        removeDockWidget(dockWidget);
        dockWidget->deleteLater();
    }
}

void MainWindow::setupImageNavigationBar()
{
    // Create a widget to hold the navigation controls
    QWidget *navWidget = new QWidget(this);
    QHBoxLayout *navLayout = new QHBoxLayout(navWidget);
    navLayout->setContentsMargins(5, 5, 5, 5);
    
    // Create spinbox for max image count (without label)
    m_imageCountSpinBox = new QSpinBox(navWidget);
    m_imageCountSpinBox->setRange(1, 100);
    m_imageCountSpinBox->setValue(10);
    m_imageCountSpinBox->setToolTip(tr("Maximale Anzahl der Bilder im Puffer"));
    navLayout->addWidget(m_imageCountSpinBox);
    
    // Create slider for image navigation
    m_imageSlider = new QSlider(Qt::Horizontal, navWidget);
    m_imageSlider->setRange(0, 0);
    m_imageSlider->setValue(0);
    m_imageSlider->setEnabled(false);
    m_imageSlider->setToolTip(tr("Bild auswählen"));
    navLayout->addWidget(m_imageSlider, 1);  // Give slider more space
    
    // Add navigation bar to bottom of main window
    QDockWidget *navDock = new QDockWidget(this);
    navDock->setObjectName("ImageNavigationDock");
    navDock->setWidget(navWidget);
    navDock->setFeatures(QDockWidget::NoDockWidgetFeatures);  // Fixed, non-closable
    navDock->setTitleBarWidget(new QWidget()); // Hide title bar
    addDockWidget(Qt::BottomDockWidgetArea, navDock);
    
    // Connect signals
    connect(m_imageCountSpinBox, QOverload<int>::of(&QSpinBox::valueChanged),
            m_imageWidget, &ImageWidget::setMaxImageCount);
    connect(m_imageSlider, &QSlider::valueChanged,
            m_imageWidget, &ImageWidget::selectImage);
}

void MainWindow::updateImageNavigationUI(int current, int max)
{
    // Block signals while updating to prevent recursive calls
    m_imageSlider->blockSignals(true);
    
    // Update slider range and enable/disable based on available images
    m_imageSlider->setRange(0, current > 0 ? current - 1 : 0);
    m_imageSlider->setEnabled(current > 0);
    
    // Update slider value to point to latest image if not manually changed
    if (current > 0) {
        m_imageSlider->setValue(current - 1);
    }
    
    m_imageSlider->blockSignals(false);
}

void MainWindow::updateStatusBarImageInfo(int index, unsigned int sequence, unsigned long timestamp)
{
    // Update status bar with current image info
    statusBar()->showMessage(tr("%1 fps - #%2 - ts: %3 ms")
        .arg(m_fps, 0, 'f', 1)
        .arg(sequence, 5, 10, QChar('0'))
        .arg(timestamp, 8));
}