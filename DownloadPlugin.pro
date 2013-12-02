QT       += network
TEMPLATE = lib
TARGET = DownloadPlugin
CONFIG += plugin
CONFIG -= app_bundle
version = 1.0.0

DESTDIR = ../Output
SOURCES += downloadplugin.cpp
HEADERS += downloadplugin.h \
    downloadinterface.h

OTHER_FILES +=
