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
	$${SRC}/configuration.cpp \
	$${SRC}/controller.cpp \
	$${SRC}/gps.cpp \
	$${SRC}/layer.cpp \
	$${SRC}/map.cpp \
	$${SRC}/map-object.cpp \
	$${SRC}/mark.cpp \
	$${SRC}/path-item.cpp \
	$${SRC}/path.cpp \
	$${SRC}/projection.cpp \
	$${SRC}/tile-cache.cpp \
	$${SRC}/tile-download.cpp \
	$${SRC}/tile.cpp \
	$${SRC}/tiled-layer.cpp \
	$${SRC}/types.cpp \
    path-test.cpp

HEADERS += \
	$${SRC}/configuration.h \
	$${SRC}/controller.h \
	$${SRC}/gps.h \
	$${SRC}/layer.h \
	$${SRC}/map.h \
	$${SRC}/map-object.h \
	$${SRC}/mark.h \
	$${SRC}/path-item.h \
	$${SRC}/path.h \
	$${SRC}/projection.h \
	$${SRC}/tile-cache.h \
	$${SRC}/tile-download.h \
	$${SRC}/tile.h \
	$${SRC}/tiled-layer.h \
	$${SRC}/types.h \
    path-test.h

RESOURCES += \
    paths.qrc

check.commands = ./path-test
check.depends = path-test
QMAKE_EXTRA_TARGETS += check
