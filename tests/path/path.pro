include(../tests.pri)

TARGET = path-test

QT += \
    testlib

LIBS += -lMappero

SOURCES += \
    path-test.cpp

HEADERS += \
    path-test.h

RESOURCES += \
    paths.qrc

check.commands = ./path-test
check.depends = path-test
QMAKE_EXTRA_TARGETS += check
