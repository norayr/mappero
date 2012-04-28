TARGET = path-test

QT += \
    declarative \
    network \
    testlib \
    xml

SRC = ../../src/qt

INCLUDEPATH += \
    $${SRC}

SOURCES += \
    $${SRC}/kml.cpp \
    $${SRC}/path.cpp \
    $${SRC}/projection.cpp \
    $${SRC}/types.cpp \
    path-test.cpp

HEADERS += \
    $${SRC}/path.h \
    $${SRC}/projection.h \
    $${SRC}/types.h \
    path-test.h

RESOURCES += \
    paths.qrc

check.commands = ./path-test
check.depends = path-test
QMAKE_EXTRA_TARGETS += check

contains(MEEGO_EDITION,harmattan) {
    target.path = /opt/path/bin
    INSTALLS += target
}
