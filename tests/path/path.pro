include(../../common-config.pri)

TARGET = path-test

QT += \
    declarative \
    network \
    testlib \
    xml

INCLUDEPATH += \
    ../../lib/Mappero

QMAKE_LIBDIR = $${TOP_BUILD_DIR}/lib/Mappero
QMAKE_RPATHDIR = $${QMAKE_LIBDIR}
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
