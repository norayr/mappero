include(../../common-config.pri)

TEMPLATE = lib
TARGET = MapperoUi

# Error on undefined symbols
QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

QT += \
    qml \
    quick

DEFINES += \
    BUILDING_LIBMAPPEROUI

SOURCES += \
    types.cpp

private_headers = \
    debug.h

public_headers = \
    types.h

HEADERS += \
    $$private_headers \
    $$public_headers

QML_SOURCES = \
    qml/BusyIndicator.qml \
    qml/Button.qml \
    qml/ButtonWithIcon.qml \
    qml/ImageButton.qml \
    qml/LabelLayout.qml \
    qml/ListBrowser.qml \
    qml/MenuButton.qml \
    qml/OsmButton.qml \
    qml/Pane.qml \
    qml/PaneBackground.qml \
    qml/Popup.qml \
    qml/Selector.qml \
    qml/SelectorView.qml \
    qml/SimpleSelector.qml \
    qml/TextField.qml \
    qml/UIConstants.js \
    qml/libmappero.qrc \
    qml/qmldir

OTHER_FILES += \
    $${QML_SOURCES}

RESOURCES += \
    qml/mappero-ui.qrc

target.path = $${INSTALL_LIBDIR}
INSTALLS += target

pkgconfig.CONFIG = no_check_exist
pkgconfig.files = $${TARGET}.pc
pkgconfig.path = $${INSTALL_LIBDIR}/pkgconfig
QMAKE_EXTRA_TARGETS += pkgconfig
INSTALLS += pkgconfig

QMAKE_SUBSTITUTES += $${pkgconfig.files}.in

QMAKE_CLEAN += $${pkgconfig.files}
