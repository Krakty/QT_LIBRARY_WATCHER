TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle

QT += core

INCLUDEPATH += include \
               include/nlohmann

SOURCES += src/main.cpp \
           src/ConfigManager.cpp \
           src/DirectoryWatcher.cpp

TARGET = QT_LIBRARY_WATCHER

HEADERS += \
    include/DirectoryWatcher.h
