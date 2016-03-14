include(../tests.pri)

TARGET = geotag-test

QT += \
    concurrent \
    network \
    quick \
    testlib

CONFIG += link_pkgconfig
PKGCONFIG += exiv2

DEFINES += \
    TEST_DATA_DIR=\\\"$${PWD}/\\\"

SRC = ../../src/qt

INCLUDEPATH += \
    $${SRC}

SOURCES += \
    $${SRC}/taggable.cpp \
    geotag-test.cpp

HEADERS += \
    $${SRC}/taggable.h \
    geotag-test.h

check.commands = ./geotag-test
check.depends = geotag-test
QMAKE_EXTRA_TARGETS += check
