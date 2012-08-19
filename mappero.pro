include(common-config.pri)

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
    desktopfile.files = data/harmattan/mappero.desktop
    desktopfile.path = /usr/share/applications
    INSTALLS += desktopfile

    icon.files = data/mappero.png
    icon.path = /usr/share/icons/hicolor/80x80/apps
    INSTALLS += icon
} else {
    unix {
        IN_FILES = \
            data/mappero-geotagger.desktop \
            data/mappero.desktop
        for(inFile, IN_FILES) {
            system(sed "s,INSTALL_PREFIX,$${INSTALL_PREFIX}," < $${inFile}.in \
                > $${inFile})
        }

        QMAKE_DISTCLEAN += $${IN_FILES}

        desktopfile.files = \
            data/mappero-geotagger.desktop \
            data/mappero.desktop
        desktopfile.path = /usr/share/applications
        INSTALLS += desktopfile

        icon.files = \
            data/icons/scalable/mappero-geotagger.svg \
            data/icons/scalable/mappero.svg
        icon.path = /usr/share/icons/hicolor/scalable/apps
        INSTALLS += icon
    }
}

DISTNAME = mappero-$${PROJECT_VERSION}
dist.commands = "git archive --format=tar --prefix=$${DISTNAME}/ HEAD | bzip2 -9 > $${DISTNAME}.tar.bz2"
QMAKE_EXTRA_TARGETS += dist
