include(../../common-config.pri)

TEMPLATE = lib
TARGET = Mappero

DEFINES += \
    PLUGIN_QML_DIR=\\\"$${PLUGIN_QML_DIR}\\\"

QT += \
    qml

SOURCES += \
    plugin.cpp \
    qml-search-model.cpp \
    search-plugin.cpp

private_headers = \
    debug.h \
    qml-search-model.h

public_headers = \
    plugin.h Plugin \
    search-plugin.h SearchPlugin

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
