// Copyright (c) 2026 Peter Martienssen
// SPDX-License-Identifier: MIT

#pragma once

#include <QtCore>
#include <QtPlugin>
#include <memory>

// Forward declaration
class Function;

/**
 * @brief Interface for function plugins that can be registered at runtime
 * 
 * This interface defines the contract for function plugins. Each plugin must
 * provide metadata (ID, display name, menu text) and be able to create instances
 * of its function type.
 * 
 * This interface supports both static linking (via PluginRegistrar) and dynamic
 * loading (via QPluginLoader). For dynamic plugins, use Q_PLUGIN_METADATA in your
 * implementation.
 */
class FunctionPlugin
{
public:
    virtual ~FunctionPlugin() = default;
    
    /**
     * @brief Unique identifier for this function type (e.g., "histogram", "colorchecker")
     */
    virtual QString id() const = 0;
    
    /**
     * @brief Display name for this function (e.g., "Histogram", "ColorChecker")
     * Also used as text in context menus.
     */
    virtual QString displayName() const = 0;
    
    /**
     * @brief Create a new instance of this function
     */
    virtual std::unique_ptr<Function> createInstance() const = 0;
};

// Declare the interface for Qt's plugin system
#define FunctionPlugin_iid "com.v4l2testgui.FunctionPlugin/1.0"
Q_DECLARE_INTERFACE(FunctionPlugin, FunctionPlugin_iid)

/**
 * @brief Singleton registry for function plugins
 * 
 * This registry manages all available function plugins and provides factory
 * methods to create function instances by ID or from JSON.
 */
class FunctionRegistry
{
public:
    /**
     * @brief Get the singleton instance
     */
    static FunctionRegistry& instance();
    
    /**
     * @brief Register a new function plugin
     */
    void registerPlugin(std::shared_ptr<FunctionPlugin> plugin);
    
    /**
     * @brief Load all plugins from a directory
     * Searches for Qt plugin files (.dylib, .so, .dll) and attempts to load them
     * @param directory Path to the directory containing plugin files
     */
    void loadPluginsFromDirectory(const QString &directory);
    
    /**
     * @brief Get all registered plugin IDs
     */
    QStringList availablePlugins() const;
    
    /**
     * @brief Get a specific plugin by ID
     */
    std::shared_ptr<FunctionPlugin> getPlugin(const QString &id) const;
    
    /**
     * @brief Create a function instance by plugin ID
     */
    std::unique_ptr<Function> createFunction(const QString &id) const;
    
    /**
     * @brief Create a function instance from JSON data
     * The JSON object must contain a "type" field with the plugin ID
     */
    std::unique_ptr<Function> createFunctionFromJson(const QJsonObject &json) const;
    
private:
    FunctionRegistry() = default;
    ~FunctionRegistry() = default;
    
    // Disable copy/move
    FunctionRegistry(const FunctionRegistry&) = delete;
    FunctionRegistry& operator=(const FunctionRegistry&) = delete;
    
    QMap<QString, std::shared_ptr<FunctionPlugin>> m_plugins;
};
