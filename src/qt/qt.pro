# Add more folders to ship with the application, here
folder_01.source = ../qml
folder_01.target = qml
DEPLOYMENTFOLDERS = folder_01

TARGET = mappero

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

# If your application uses the Qt Mobility libraries, uncomment the following
# lines and add the respective components to the MOBILITY variable.
# CONFIG += mobility
# MOBILITY +=

# Speed up launching on MeeGo/Harmattan when using applauncherd daemon
CONFIG += qdeclarative-boostable

QT += \
    declarative \
    network \
    opengl

SOURCES += \
	configuration.cpp \
	controller.cpp \
	layer.cpp \
	main.cpp \
	map.cpp \
	projection.cpp \
	tile-cache.cpp \
	tile-download.cpp \
	tile.cpp \
	tiled-layer.cpp \
	types.cpp \
	view.cpp

HEADERS += \
	configuration.h \
	controller.h \
	map.h \
	tile-download.h \
	view.h

RESOURCES += \
    ../qml/qml.qrc \
    ../../data/icons/scalable/icons.qrc

contains(MEEGO_EDITION,harmattan) {
    target.path = /opt/qt/bin
    INSTALLS += target
}
