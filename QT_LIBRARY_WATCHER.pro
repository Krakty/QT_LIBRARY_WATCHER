# Project Settings
TEMPLATE = app               # Defines this as an application
CONFIG += console c++11      # Console application with C++11 support
CONFIG -= app_bundle         # Disable macOS app bundling (safe for Linux)

# Qt Modules
QT += core                   # Include the Qt Core module for non-GUI apps

# Source and Header Directories
INCLUDEPATH += include       # Include directory for header files
SOURCES += src/main.cpp      # Source files in the src directory
HEADERS += include/main.h    # Add headers here as you create them

# Compiler and Build Settings
TARGET = QT_LIBRARY_WATCHER  # Output binary name
