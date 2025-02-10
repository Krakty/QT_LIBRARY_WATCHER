#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <QString>
#include <nlohmann/json.hpp>

// Global struct for configuration
struct GlobalConfig
{
    QString downloadDir;
    QString backupDir;
    QString user;
    QString group;
};

extern GlobalConfig globalConfig;

class ConfigManager {
public:
    explicit ConfigManager(const QString& configFilePath);
    void loadConfig();
    static void createDefaultConfig(const QString& configFilePath);
    static void createServiceFile();  // New function to create the systemd service file

private:
    nlohmann::json config;
    QString configFilePath;
    static QString resolveGroupToGID(const QString& groupName);
};

#endif // CONFIGMANAGER_H
