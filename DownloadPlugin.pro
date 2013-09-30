QT       += network
TEMPLATE = lib
TARGET = DownloadPlugin
CONFIG += plugin
CONFIG -= app_bundle
version = 1.0.0

ROOT = ../../../
BIN = ../../bin

INCLUDEPATH = $$ROOT/interfaces
DESTDIR = $$BIN
MOC_DIR = moc
OBJECTS_DIR = obj
SOURCES += downloadplugin.cpp \
    json.cpp
HEADERS += downloadplugin.h \
    $$ROOT/interfaces/downloader.h \
    json.h
