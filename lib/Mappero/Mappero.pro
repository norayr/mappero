include(../../common-config.pri)

TEMPLATE = lib
TARGET = Mappero

# Error on undefined symbols
QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

DEFINES += \
    PLUGIN_QML_DIR=\\\"$${PLUGIN_QML_DIR}\\\"

QT += \
    qml

SOURCES += \
    model-aggregator.cpp \
    gpx.cpp \
    kml.cpp \
    path.cpp \
    path-builder.cpp \
    plugin.cpp \
    projection.cpp \
    qml-search-model.cpp \
    search-plugin.cpp \
    types.cpp

private_headers = \
    model-aggregator.h \
    debug.h \
    gpx.h \
    kml.h \
    path-builder.h \
    qml-search-model.h

public_headers = \
    path.h Path \
    plugin.h Plugin \
    projection.h Projection \
    search-plugin.h SearchPlugin \
    types.h

HEADERS += \
    $$private_headers \
    $$public_headers

QML_SOURCES = \
    qml/DefaultResultsDelegate.qml

OTHER_FILES += \
    $${QML_SOURCES}

RESOURCES += \
    libmappero.qrc

target.path = $${INSTALL_LIBDIR}
INSTALLS += target

pkgconfig.CONFIG = no_check_exist
pkgconfig.files = $${TARGET}.pc
pkgconfig.path = $${INSTALL_LIBDIR}/pkgconfig
QMAKE_EXTRA_TARGETS += pkgconfig
INSTALLS += pkgconfig

QMAKE_SUBSTITUTES += $${pkgconfig.files}.in

QMAKE_CLEAN += $${pkgconfig.files}
