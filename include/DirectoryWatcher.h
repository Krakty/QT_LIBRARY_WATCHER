#ifndef DIRECTORYWATCHER_H
#define DIRECTORYWATCHER_H

#include <QObject>
#include <QFileSystemWatcher>
#include <QSet>
#include <QTimer>

class DirectoryWatcher : public QObject {
    Q_OBJECT

public:
    explicit DirectoryWatcher(QObject* parent = nullptr);
    void startWatching(const QString& path);

private slots:
    void onDirectoryChanged(const QString& path);
    void onFileChanged(const QString& filePath);
    void periodicBackupCheck();

private:
    QFileSystemWatcher watcher;
    QSet<QString> processedFiles;
    QSet<QString> watchedFiles;
    QTimer periodicTimer;

    void handleNewZipFile(const QString& filePath);
    bool isFileStable(const QString& filePath);
    void passZipToLoader(const QString& filePath);  // ✅ New function
};

#endif // DIRECTORYWATCHER_H
