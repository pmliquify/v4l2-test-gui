// Copyright (c) 2026 Peter Martienssen
// SPDX-License-Identifier: MIT

#include "mainwindow.hpp"
#include "functionplugin.hpp"
#include <QTranslator>
#include <QLocale>
#include <QLibraryInfo>
#include <QCoreApplication>
#include <QDir>

int main(int argc, char *argv[]) 
{
    QApplication app(argc, argv);
    
    QLocale systemLocale = QLocale::system();
    QLocale::setDefault(systemLocale);
    
    // Load plugins from plugins directory relative to executable
    QString pluginPath = QCoreApplication::applicationDirPath();
#ifdef Q_OS_MAC
    // On macOS, check multiple possible plugin locations
    QStringList possiblePaths = {
        QDir(pluginPath).filePath("../../../../../plugins"),  // Development build (from .app/Contents/MacOS)
        QDir(pluginPath).filePath("../PlugIns")                // Installed app bundle
    };
    
    bool foundPlugins = false;
    for (const QString &path : possiblePaths) {
        if (QDir(path).exists()) {
            pluginPath = QDir(path).absolutePath();
            foundPlugins = true;
            break;
        }
    }
    
    if (!foundPlugins) {
        qWarning() << "No plugin directory found. Tried:" << possiblePaths;
    }
#else
    pluginPath = QDir(pluginPath).filePath("plugins");
#endif
    FunctionRegistry::instance().loadPluginsFromDirectory(pluginPath);
    
    MainWindow window;
    window.show();
    
    return app.exec();
}
