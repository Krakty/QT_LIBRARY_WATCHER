#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QFile>
#include <signal.h>

#include "ConfigManager.h"
#include "DirectoryWatcher.h"

// ✅ Graceful shutdown handler
void handleSignal(int signal) {
    qDebug() << "Received signal:" << signal << "- Shutting down gracefully.";
    QCoreApplication::quit();
}

int main(int argc, char *argv[]) {
    QCoreApplication app(argc, argv);

    // ✅ Handle termination signals
    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);

    const QString configFile = "QT_LIBRARY_WATCHER.json";

    // ✅ Handle the -Defaults argument
    for (int i = 1; i < argc; ++i) {
        if (QString(argv[i]) == "-Defaults") {
            ConfigManager::createDefaultConfig(configFile);  // Create default JSON

            // ✅ Load the newly created default config
            ConfigManager configManager(configFile);
            configManager.loadConfig();

            // ✅ Verify loaded values before creating the service file
            qDebug() << "Loaded User:" << globalConfig.user;
            qDebug() << "Loaded Group:" << globalConfig.group;

            configManager.createServiceFile();  // Now correctly uses globalConfig
            return 0;
        }
    }

    // ✅ Load the configuration file for normal operation
    ConfigManager configManager(configFile);
    configManager.loadConfig();

    qDebug() << "Download Directory:" << globalConfig.downloadDir;
    qDebug() << "Backup Directory:" << globalConfig.backupDir;

    // ✅ Start the directory watcher
    DirectoryWatcher watcher;
    watcher.startWatching(globalConfig.downloadDir);

    // ✅ Keep the daemon running
    return app.exec();
}
