#include "DirectoryWatcher.h"
#include "ConfigManager.h"
#include <QDir>
#include <QFileInfo>
#include <QDebug>
#include <QProcess>
#include <QThread>
#include <QFile>
#include <QTimer>
#include <QEventLoop>
#include <QProcessEnvironment>

DirectoryWatcher::DirectoryWatcher(QObject* parent) : QObject(parent) {}

void DirectoryWatcher::startWatching(const QString& path) {
    watcher.addPath(path);

    qDebug() << "Started watching directory:" << path;
    qDebug() << "Currently watching directories:" << watcher.directories();
    qDebug() << "Currently watching files:" << watcher.files();

    QDir dir(path);
    QStringList files = dir.entryList(QStringList() << "*.zip", QDir::Files);

    for (int i = 0; i < files.size(); ++i) {
        const QString& file = files.at(i);
        QString filePath = dir.absoluteFilePath(file);

        if (!watchedFiles.contains(filePath)) {
            watcher.addPath(filePath);
            watchedFiles.insert(filePath);
            qDebug() << "Started watching file:" << filePath;
        }
    }

    connect(&watcher, &QFileSystemWatcher::directoryChanged,
            this, &DirectoryWatcher::onDirectoryChanged);

    connect(&watcher, &QFileSystemWatcher::fileChanged,
            this, &DirectoryWatcher::onFileChanged);

    connect(&periodicTimer, &QTimer::timeout, this, &DirectoryWatcher::periodicBackupCheck);
    periodicTimer.start(600000);  // 10 minutes
    qDebug() << "Periodic backup checker started (every 10 minutes).";
}

void DirectoryWatcher::onDirectoryChanged(const QString& path) {
    qDebug() << "Directory changed:" << path;

    QDir dir(path);
    QStringList files = dir.entryList(QStringList() << "*.zip", QDir::Files);

    for (const QString& file : files) {
        QString filePath = dir.absoluteFilePath(file);

        if (!watchedFiles.contains(filePath)) {
            watcher.addPath(filePath);            // ✅ Start watching the new file
            watchedFiles.insert(filePath);
            qDebug() << "Now watching new file:" << filePath;

            if (isFileStable(filePath)) {
                qDebug() << "Processing stable file:" << filePath;
                handleNewZipFile(filePath);       // ✅ Process immediately if stable
            } else {
                qDebug() << "File is still being copied, skipping for now:" << filePath;
            }
        }
    }
}

void DirectoryWatcher::onFileChanged(const QString& filePath) {
    qDebug() << "File modified:" << filePath;

    if (processedFiles.contains(filePath)) {
        qDebug() << "Skipping already processed file:" << filePath;
        return;
    }

    if (isFileStable(filePath)) {
        qDebug() << "Processing modified file:" << filePath;
        handleNewZipFile(filePath);
    } else {
        qDebug() << "File is still unstable after modification:" << filePath;
    }
}

void DirectoryWatcher::handleNewZipFile(const QString& filePath) {
    if (processedFiles.contains(filePath)) {
        qDebug() << "Skipping already processed file in handleNewZipFile:" << filePath;
        return;
    }

    qDebug() << "New ZIP file detected:" << filePath;
    processedFiles.insert(filePath);  // ✅ Mark as processed immediately

    passZipToLoader(filePath);

    QProcess::execute("touch", QStringList() << filePath);
    qDebug() << "Touched file:" << filePath;

    QString backupDir = globalConfig.backupDir;
    QDir().mkpath(backupDir);

    QString destinationPath = QDir(backupDir).filePath(QFileInfo(filePath).fileName());

    if (QFile::exists(destinationPath)) {
        qDebug() << "File already exists in backup. Overwriting:" << destinationPath;
        if (!QFile::remove(destinationPath)) {
            qWarning() << "Failed to remove existing file in backup directory:" << destinationPath;
            return;
        }
    }

    if (QFile::rename(filePath, destinationPath)) {
        qDebug() << "Moved file to backup directory:" << destinationPath;
        watcher.removePath(filePath);
    } else {
        qWarning() << "Failed to move file to backup directory.";
    }
}

void DirectoryWatcher::passZipToLoader(const QString& filePath) {
    QString loaderPath = "./QT_LIBRARY_LOADER";  // ✅ Relative to the working directory
    QString workingDir = "/usr/local/bin/QT_LIBRARY_LOADER";
    QStringList arguments;
    arguments << filePath;

    // ✅ Determine the user
    QString serviceUser = globalConfig.user.isEmpty() ? qgetenv("USER") : globalConfig.user;

    // ✅ Debug: Full command preview
    QString fullCommand = QString("cd %1 && sudo -u %2 %3 %4")
                              .arg(workingDir, serviceUser, loaderPath, arguments.join(" "));
    qDebug() << "🚀 Attempting to run command:" << fullCommand;

    QProcess process;
    process.setWorkingDirectory(workingDir);

    // ✅ Correctly pass the executable and arguments
    process.start("sudo", {"-u", serviceUser, loaderPath, filePath});

    if (!process.waitForStarted()) {
        qWarning() << "❌ Failed to start QT_LIBRARY_LOADER for:" << filePath;
        qWarning() << "Process error:" << process.errorString();
        return;
    }

    qDebug() << "✅ QT_LIBRARY_LOADER started for:" << filePath;

    if (!process.waitForFinished(-1)) {
        qWarning() << "❌ QT_LIBRARY_LOADER did not finish correctly for:" << filePath;
        qWarning() << "Process error:" << process.errorString();
        return;
    }

    QByteArray standardOutput = process.readAllStandardOutput();
    QByteArray standardError = process.readAllStandardError();

    if (!standardOutput.isEmpty()) {
        qDebug() << "✅ QT_LIBRARY_LOADER output:" << standardOutput;
    }

    if (!standardError.isEmpty()) {
        qWarning() << "❌ QT_LIBRARY_LOADER errors:" << standardError;
    }

    int exitCode = process.exitCode();
    QProcess::ExitStatus exitStatus = process.exitStatus();

    if (exitStatus == QProcess::NormalExit && exitCode == 0) {
        qDebug() << "✅ QT_LIBRARY_LOADER finished successfully for:" << filePath;
    } else {
        qWarning() << "❌ QT_LIBRARY_LOADER failed with exit code" << exitCode
                   << "and status" << exitStatus;
    }
}

bool DirectoryWatcher::isFileStable(const QString& filePath) {
    QFileInfo fileInfo(filePath);
    qint64 lastSize = fileInfo.size();

    QEventLoop loop;
    QTimer::singleShot(5000, &loop, &QEventLoop::quit);
    loop.exec();

    fileInfo.refresh();
    return fileInfo.size() == lastSize;
}

void DirectoryWatcher::periodicBackupCheck() {
    qDebug() << "Running periodic backup check...";

    QString downloadDir = globalConfig.downloadDir;
    QString backupDir = globalConfig.backupDir;

    QDir dir(downloadDir);
    QStringList files = dir.entryList(QStringList() << "*.zip", QDir::Files);

    for (int i = 0; i < files.size(); ++i) {
        const QString& file = files.at(i);
        QString sourcePath = dir.absoluteFilePath(file);
        QString backupPath = QDir(backupDir).filePath(file);

        if (processedFiles.contains(sourcePath)) {
            qDebug() << "Checking processed file:" << sourcePath;

            if (QFile::exists(backupPath)) {
                qDebug() << "Backup exists, overwriting:" << backupPath;

                if (!QFile::remove(backupPath)) {
                    qWarning() << "Failed to remove existing backup file:" << backupPath;
                    continue;
                }
            }

            if (QFile::rename(sourcePath, backupPath)) {
                qDebug() << "Moved processed file to backup:" << backupPath;
                watcher.removePath(sourcePath);
            } else {
                qWarning() << "Failed to move processed file to backup:" << sourcePath;
            }
        }
    }

    processedFiles.clear();
    qDebug() << "Processed files list cleared.";
    qDebug() << "Periodic backup check completed.";
}
