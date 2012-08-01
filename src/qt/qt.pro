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

load(mobilityconfig)
contains(MOBILITY_CONFIG, location) | maemo5 {
    message(QtMobility location is available)
    CONFIG += mobility
    MOBILITY += location
    DEFINES += HAS_QTM_LOCATION
} else {
    message(QtMobility location unavailable)
}

# Speed up launching on MeeGo/Harmattan when using applauncherd daemon
CONFIG += qdeclarative-boostable

QT += \
    declarative \
    network \
    opengl

SOURCES += \
    configuration.cpp \
    controller.cpp \
    gps.cpp \
    kml.cpp \
    layer.cpp \
    main.cpp \
    map.cpp \
    map-object.cpp \
    mark.cpp \
    path-layer.cpp \
    path.cpp \
    poi-item.cpp \
    poi-view.cpp \
    projection.cpp \
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
    map.h \
    map-object.h \
    mark.h \
    path-layer.h \
    path.h \
    poi-item.h \
    poi-view.h \
    tile-download.h \
    tracker.h \
    view.h

contains(MEEGO_EDITION,harmattan) {
    # Harmattan UI overrides
    RESOURCES += ../qml/harmattan.qrc
    DEFINES += MEEGO
} else {
    maemo5 {
        DEFINES += MAEMO5
    } else {
        # Desktop
        RESOURCES += ../qml/desktop.qrc
    }
}

system(pkg-config --exists exiv2) {
    message("libexiv2 is available")
    DEFINES += GEOTAGGING_ENABLED
    PKGCONFIG += exiv2
    SOURCES += \
        taggable-area.cpp \
        taggable-selection.cpp \
        taggable.cpp
    HEADERS += \
        taggable-area.h \
        taggable-selection.h \
        taggable.h
    RESOURCES += \
        ../qml/geotag.qrc \
        ../../data/icons/scalable/icons-geotag.qrc
} else {
    message("libexiv2 not found, geotagging disabled")
}

win32 {
    LIBS += -static-libgcc "-static-libstdc++"
}

unix {
    DEFINES += XDG_THUMBNAILS
}

RESOURCES += \
    ../qml/qml.qrc \
    ../../data/icons/scalable/icons.qrc

contains(MEEGO_EDITION,harmattan) {
    target.path = /opt/qt/bin
    INSTALLS += target
}

maemo5 {
    target.path = /opt/qt/bin
    INSTALLS += target
}
