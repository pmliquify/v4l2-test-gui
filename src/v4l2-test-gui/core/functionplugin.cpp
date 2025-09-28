#include "functionplugin.hpp"
#include "function.hpp"
#include <QPluginLoader>
#include <QDir>
#include <QDebug>

/**
 * @brief Returns the singleton instance of the FunctionRegistry
 * 
 * Uses the Meyer's Singleton pattern (static local variable) to ensure
 * thread-safe lazy initialization.
 * 
 * @return Reference to the singleton FunctionRegistry instance
 */
FunctionRegistry& FunctionRegistry::instance()
{
    static FunctionRegistry instance;
    return instance;
}

/**
 * @brief Registers a new function plugin with the registry
 * 
 * If a plugin with the same ID is already registered, it will be overwritten
 * and a warning is logged. Successfully registered plugins are logged with qDebug.
 * 
 * @param plugin Shared pointer to the plugin to register
 */
void FunctionRegistry::registerPlugin(std::shared_ptr<FunctionPlugin> plugin)
{
    if (plugin) {
        QString id = plugin->id();
        if (m_plugins.contains(id)) {
            qWarning() << "Plugin with ID" << id << "is already registered. Overwriting.";
        }
        m_plugins[id] = plugin;
        qDebug() << "Registered function plugin:" << id;
    }
}

/**
 * @brief Returns a list of all registered plugin IDs
 * 
 * @return QStringList containing the IDs of all registered plugins
 */
QStringList FunctionRegistry::availablePlugins() const
{
    return m_plugins.keys();
}

/**
 * @brief Retrieves a specific plugin by its ID
 * 
 * @param id The unique identifier of the plugin to retrieve
 * @return Shared pointer to the plugin, or nullptr if not found
 */
std::shared_ptr<FunctionPlugin> FunctionRegistry::getPlugin(const QString &id) const
{
    return m_plugins.value(id, nullptr);
}

/**
 * @brief Creates a new function instance from a registered plugin
 * 
 * Looks up the plugin by ID and calls its createInstance() method to create
 * a new function instance. The caller is responsible for setting the position
 * after creation if needed.
 * 
 * @param id The unique identifier of the plugin to use
 * @return Unique pointer to the created function, or nullptr if plugin not found
 */
std::unique_ptr<Function> FunctionRegistry::createFunction(const QString &id) const
{
    auto plugin = getPlugin(id);
    if (plugin) {
        return plugin->createInstance();
    }
    qWarning() << "Failed to create function: Plugin" << id << "not found";
    return nullptr;
}

/**
 * @brief Creates a function instance from JSON data
 * 
 * Extracts the plugin ID from the "type" field in the JSON object,
 * creates a new instance via the plugin, and then calls fromJson() on it
 * to restore its state. This is used for deserializing functions from project files.
 * 
 * @param json JSON object containing function data with a "type" field
 * @return Unique pointer to the created function, or nullptr if creation fails
 */
std::unique_ptr<Function> FunctionRegistry::createFunctionFromJson(const QJsonObject &json) const
{
    QString type = json["type"].toString();
    if (type.isEmpty()) {
        qWarning() << "Cannot create function from JSON: 'type' field missing";
        return nullptr;
    }
    
    auto plugin = getPlugin(type);
    if (plugin) {
        auto function = plugin->createInstance();
        if (function) {
            function->fromJson(json);
            return function;
        }
    }
    
    qWarning() << "Failed to create function from JSON: Plugin" << type << "not found";
    return nullptr;
}

/**
 * @brief Loads all plugins from the specified directory
 * 
 * Scans the directory for plugin files and attempts to load them using
 * QPluginLoader. Successfully loaded plugins are automatically registered.
 * 
 * @param directory Path to the directory containing plugin files
 */
void FunctionRegistry::loadPluginsFromDirectory(const QString &directory)
{
    int loadedCount = 0;
    QDir pluginDir(directory);
    
    if (!pluginDir.exists()) {
        qWarning() << "Plugin directory does not exist:" << directory;
    }
    
    qDebug() << "Loading plugins from:" << pluginDir.absolutePath();
    
    // Get all files in the directory
    QStringList filters;
#ifdef Q_OS_MAC
    filters << "*.dylib" << "*.so";  // CMake MODULE creates .so files even on macOS
#elif defined(Q_OS_WIN)
    filters << "*.dll";
#else
    filters << "*.so";
#endif
    
    pluginDir.setNameFilters(filters);
    QStringList pluginFiles = pluginDir.entryList(QDir::Files);
    
    for (const QString &fileName : pluginFiles) {
        QString filePath = pluginDir.absoluteFilePath(fileName);
        QPluginLoader loader(filePath);
        
        qDebug() << "Attempting to load plugin:" << fileName;
        
        QObject *pluginObject = loader.instance();
        if (pluginObject) {
            FunctionPlugin *plugin = qobject_cast<FunctionPlugin*>(pluginObject);
            if (plugin) {
                // Wrap the raw pointer in a shared_ptr with custom deleter that does nothing
                // because Qt manages the plugin lifetime
                std::shared_ptr<FunctionPlugin> sharedPlugin(plugin, [](FunctionPlugin*){});
                registerPlugin(sharedPlugin);
                loadedCount++;
                qDebug() << "Successfully loaded plugin:" << plugin->id();
            } else {
                qWarning() << "Plugin does not implement FunctionPlugin interface:" << fileName;
            }
        } else {
            qWarning() << "Failed to load plugin:" << fileName;
            qWarning() << "Error:" << loader.errorString();
        }
    }
    
    qDebug() << "Loaded" << loadedCount << "plugins from" << directory;
}
