include(../../common-config.pri)

# Add more folders to ship with the application, here
folder_01.source = ./qml
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

win32:CONFIG(release, debug|release): BUILDD = "/release"
else:win32:CONFIG(debug, debug|release): BUILDD = "/debug"
else: BUILDD = ""

LIBS += -L$${TOP_BUILD_DIR}/lib/Mappero$${BUILDD} -lMappero
INCLUDEPATH += $${TOP_SRC_DIR}/lib
DEFINES += \
    MAPPERO_VERSION=\\\"$${PROJECT_VERSION}\\\" \
    PLUGIN_MANIFEST_DIR=\\\"$${PLUGIN_MANIFEST_DIR}\\\"

LIBS += -L$${TOP_BUILD_DIR}/lib/MapperoUi$${BUILDD} -lMapperoUi

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
    types.cpp

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
    tracker.h

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
        qml/geotag.qrc \
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
    QT += svg # so that macdeploysqt copyes the svg plugin
}

RESOURCES += \
    qml/qml.qrc \
    ../../data/icons/scalable/icons.qrc \
    ../../data/icons/scalable/directions/directions.qrc \
    ../../data/icons/scalable/transport/transport.qrc

unix {
    target.path = $${INSTALL_PREFIX}/bin
    INSTALLS += target
}

CONFIG(static) {
    DEFINES += STATIC_BUILD
    LIBS += \
        -L$$[QT_INSTALL_QML]/Qt/labs/folderlistmodel -lqmlfolderlistmodelplugin \
        -L$$[QT_INSTALL_QML]/Qt/labs/settings -lqmlsettingsplugin \
        -L$$[QT_INSTALL_QML]/QtQuick.2 \
        -L$$[QT_INSTALL_QML]/QtQuick/Controls -lqtquickcontrolsplugin \
        -L$$[QT_INSTALL_QML]/QtQuick/Dialogs -ldialogplugin \
        -L$$[QT_INSTALL_QML]/QtQuick/Dialogs/Private -ldialogsprivateplugin \
        -L$$[QT_INSTALL_QML]/QtQuick/Layouts -lqquicklayoutsplugin \
        -L$$[QT_INSTALL_QML]/QtQuick/Window.2 -lwindowplugin \
        -L$$[QT_INSTALL_PLUGINS]/imageformats
    QTPLUGIN += \
        QtQuick2Plugin \
        qjpeg \
        qsvg
}
