TEMPLATE = subdirs
SUBDIRS = src tests

OTHER_FILES += \
    qtc_packaging/debian_harmattan/rules \
    qtc_packaging/debian_harmattan/README \
    qtc_packaging/debian_harmattan/manifest.aegis \
    qtc_packaging/debian_harmattan/copyright \
    qtc_packaging/debian_harmattan/control \
    qtc_packaging/debian_harmattan/compat \
    qtc_packaging/debian_harmattan/changelog \
    qtc_packaging/debian_fremantle/rules \
    qtc_packaging/debian_fremantle/README \
    qtc_packaging/debian_fremantle/copyright \
    qtc_packaging/debian_fremantle/control \
    qtc_packaging/debian_fremantle/compat \
    qtc_packaging/debian_fremantle/changelog

contains(MEEGO_EDITION,harmattan) {
    desktopfile.files = data/mappero.desktop
    desktopfile.path = /usr/share/applications
    INSTALLS += desktopfile

    icon.files = data/mappero.png
    icon.path = /usr/share/icons/hicolor/80x80/apps
    INSTALLS += icon
}
