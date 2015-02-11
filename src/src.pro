#-------------------------------------------------
#
# Project created by QtCreator 2015-02-09T18:59:22
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++11

TARGET = opengl-edu-tool
TEMPLATE = app

DESTDIR = ../bin
RCC_DIR = ../build/rcc
MOC_DIR = ../build/moc
UI_DIR = ../build/ui
OBJECTS_DIR = ../build/obj

INCLUDEPATH += ../glm

SOURCES += main.cpp\
        mainwindow.cpp \
    scenewidget.cpp

HEADERS  += mainwindow.h \
    scenewidget.h

FORMS    += mainwindow.ui
