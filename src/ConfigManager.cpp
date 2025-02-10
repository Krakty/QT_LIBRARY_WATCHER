#include "ConfigManager.h"
#include <fstream>
#include <QDebug>
#include <QCoreApplication>
#include <QDir>
#include <QProcess>

GlobalConfig globalConfig;

ConfigManager::ConfigManager(const QString &configFilePath)
    : configFilePath(configFilePath)
{}

void ConfigManager::loadConfig() {
    std::ifstream file(configFilePath.toStdString());
    if (file.is_open()) {
        file >> config;

        globalConfig.downloadDir = QString::fromStdString(
            config.value("DownloadDir", "TEMP_DOWNLOAD_DIR_NOT_SET"));  // 🚀 Temporary Default
        qDebug() << "Download Directory Loaded:" << globalConfig.downloadDir;

        globalConfig.backupDir = QString::fromStdString(
            config.value("BackupDir", "TEMP_BACKUP_DIR_NOT_SET"));      // 🚀 Temporary Default
        qDebug() << "Backup Directory Loaded:" << globalConfig.backupDir;

        globalConfig.user = QString::fromStdString(
            config.value("User", "TEMP_USER_NOT_SET"));                 // 🚀 Temporary Default
        qDebug() << "User Loaded:" << globalConfig.user;

        globalConfig.group = QString::fromStdString(
            config.value("Group", "TEMP_GROUP_NOT_SET"));               // 🚀 Temporary Default
        qDebug() << "Group Loaded:" << globalConfig.group;

        qDebug() << "Configuration loaded successfully.";
    } else {
        qCritical() << "Error: Unable to open configuration file:" << configFilePath;
        qCritical() << "Run the program with '-Defaults' to generate a default configuration.";
        QCoreApplication::exit(1);
    }
}

void ConfigManager::createDefaultConfig(const QString &configFilePath) {
    nlohmann::json defaultConfig = {
        {"DownloadDir", "/home/tlindell/Downloads/New_Library/"},
        {"BackupDir", "/home/tlindell/Downloads/Backup_Library/"},
        {"User", "tlindell"},
        {"Group", "domain users"}
    };

    std::ofstream file(configFilePath.toStdString());
    if (file.is_open()) {
        file << defaultConfig.dump(4);
        file.close();
        qDebug() << "Default configuration file created at" << configFilePath;
    } else {
        qCritical() << "Error: Unable to create default configuration file.";
    }
}

// ✅ Private Function: Resolve Group Name to GID
QString ConfigManager::resolveGroupToGID(const QString& groupName) {
    QProcess process;
    process.start("getent", QStringList() << "group" << groupName);
    process.waitForFinished();
    QString output = process.readAllStandardOutput().trimmed();

    if (!output.isEmpty()) {
        QStringList parts = output.split(":");
        if (parts.size() >= 3) {
            return parts.at(2);  // ✅ Return GID (third field)
        }
    }
    return groupName;  // ✅ Fallback to group name if GID not found
}

void ConfigManager::createServiceFile() {
    QString serviceFilePath = QDir::currentPath() + "/qt_library_watcher.service";

    std::ofstream serviceFile(serviceFilePath.toStdString());
    if (!serviceFile.is_open()) {
        qCritical() << "Error: Unable to create service file at" << serviceFilePath;
        return;
    }

    serviceFile << "[Unit]\n";
    serviceFile << "Description=QT Library Watcher Daemon\n";
    serviceFile << "After=network.target\n\n";
    serviceFile << "[Service]\n";
    serviceFile << "ExecStart=/usr/local/bin/QT_LIBRARY_WATCHER/QT_LIBRARY_WATCHER\n";
    serviceFile << "WorkingDirectory=/usr/local/bin/QT_LIBRARY_WATCHER/\n";
    serviceFile << "Restart=on-failure\n";

    // ✅ User
    qDebug() << "User=" << globalConfig.user;
    serviceFile << "User=" << globalConfig.user.toStdString() << "\n";

    // ✅ Resolve Group to GID (Private Function)
    QString resolvedGroup = resolveGroupToGID(globalConfig.group);
    qDebug() << "Group=" << resolvedGroup;
    serviceFile << "Group=" << resolvedGroup.toStdString() << "\n";

    serviceFile << "StandardOutput=syslog\n";
    serviceFile << "StandardError=syslog\n";
    serviceFile << "SyslogIdentifier=qt_library_watcher\n\n";
    serviceFile << "[Install]\n";
    serviceFile << "WantedBy=multi-user.target\n";

    serviceFile.close();
    qDebug() << "Service file created at" << serviceFilePath;

    // ✅ Systemctl Instructions
    qDebug() << "\nTo install and manage the service, run the following commands:";
    qDebug() << "-------------------------------------------------------------";
    qDebug() << "sudo cp qt_library_watcher.service /etc/systemd/system/";
    qDebug() << "sudo systemctl daemon-reload";
    qDebug() << "sudo systemctl enable qt_library_watcher";
    qDebug() << "sudo systemctl start qt_library_watcher";
    qDebug() << "\nTo check the service status:";
    qDebug() << "sudo systemctl status qt_library_watcher";
    qDebug() << "-------------------------------------------------------------";
}
