include(../../common-config.pri)

# Add more folders to ship with the application, here
folder_01.source = ../qml
folder_01.target = qml
DEPLOYMENTFOLDERS = folder_01

TARGET = mappero

CONFIG += link_pkgconfig

# Additional import path used to resolve QML modules in Creator's code model
QML_IMPORT_PATH =

symbian:TARGET.UID3 = 0xE1BBB51C

# Smart Installer package's UID
# This UID is from the protected range and therefore the package will
# fail to install if self-signed. By default qmake uses the unprotected
# range value if unprotected UID is defined for the application and
# 0x2002CCCF value if protected UID is given to the application
#symbian:DEPLOYMENT.installer_header = 0x2002CCCF

# Allow network access on Symbian
symbian:TARGET.CAPABILITY += NetworkServices

QT += \
    network \
    quick

qtHaveModule(location) {
    QT += location
}

LIBS += -lMappero
QMAKE_LIBDIR += $${TOP_BUILD_DIR}/lib/Mappero
INCLUDEPATH += $${TOP_SRC_DIR}/lib
DEFINES += \
    PLUGIN_MANIFEST_DIR=\\\"$${PLUGIN_MANIFEST_DIR}\\\"

LIBS += -lMapperoUi
QMAKE_LIBDIR += $${TOP_BUILD_DIR}/lib/MapperoUi

QMAKE_LFLAGS += '-Wl,-rpath,\'\$$ORIGIN/../lib\''

SOURCES += \
    configuration.cpp \
    controller.cpp \
    gps.cpp \
    layer.cpp \
    main.cpp \
    map.cpp \
    map-object.cpp \
    mark.cpp \
    path-item.cpp \
    path-layer.cpp \
    plugin-manager.cpp \
    plugin-model.cpp \
    poi-item.cpp \
    poi-view.cpp \
    tile-cache.cpp \
    tile-download.cpp \
    tile.cpp \
    tiled-layer.cpp \
    tracker.cpp \
    types.cpp \
    view.cpp

HEADERS += \
    configuration.h \
    controller.h \
    gps.h \
    layer.h \
    map.h \
    map-object.h \
    mark.h \
    path-item.h \
    path-layer.h \
    plugin-manager.h \
    plugin-model.h \
    poi-item.h \
    poi-view.h \
    tile-download.h \
    tiled-layer.h \
    tracker.h \
    view.h

system(pkg-config --exists exiv2) {
    message("libexiv2 is available")
    DEFINES += GEOTAGGING_ENABLED
    QT += concurrent
    PKGCONFIG += exiv2
    SOURCES += \
        taggable-area.cpp \
        taggable-selection.cpp \
        taggable.cpp \
        ticks.cpp
    HEADERS += \
        taggable-area.h \
        taggable-selection.h \
        taggable.h \
        ticks.h
    RESOURCES += \
        ../qml/geotag.qrc \
        ../../data/icons/scalable/icons-geotag.qrc
} else {
    message("libexiv2 not found, geotagging disabled")
}

win32 {
    LIBS += -static-libgcc "-static-libstdc++"
    RC_FILE = $${TOP_SRC_DIR}/data/icons/windows/mappero-geotagger.rc
}

unix {
    DEFINES += XDG_THUMBNAILS
}

macx {
    QMAKE_INFO_PLIST = ../../data/MapperoInfo.plist
    QMAKE_CFLAGS += -gdwarf-2
    QMAKE_CXXFLAGS += -gdwarf-2
}

RESOURCES += \
    ../qml/qml.qrc \
    ../../data/icons/scalable/icons.qrc \
    ../../data/icons/scalable/directions/directions.qrc \
    ../../data/icons/scalable/transport/transport.qrc

unix {
    target.path = $${INSTALL_PREFIX}/bin
    INSTALLS += target
}
