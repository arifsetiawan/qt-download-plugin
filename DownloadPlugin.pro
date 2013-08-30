#-------------------------------------------------
#
# Project created by QtCreator 2013-08-17T23:09:57
#
#-------------------------------------------------

QT       += network
TEMPLATE = lib
TARGET = DownloadPlugin
CONFIG += plugin
CONFIG -= app_bundle
version = 1.0.0

INCLUDEPATH = ../DownloadHost/interfaces
DESTDIR = ../DownloadHost/plugins

SOURCES += downloadplugin.cpp \
    json.cpp
HEADERS += downloadplugin.h \
    ../DownloadHost/interfaces/downloader.h \
    json.h
