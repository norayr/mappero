PROJECT_VERSION = 1.6

INSTALL_PREFIX = /usr

contains(MEEGO_EDITION,harmattan) {
    INSTALL_PREFIX = /opt/qt
}

maemo5 {
    INSTALL_PREFIX = /opt/qt
}

isEmpty(PREFIX) {
    message("====")
    message("==== NOTE: To override the installation path run: `qmake PREFIX=/custom/path'")
    message("==== (current installation path is `$${INSTALL_PREFIX}')")
} else {
    INSTALL_PREFIX = $${PREFIX}
    message("====")
    message("==== install prefix set to `$${INSTALL_PREFIX}'")
}

INSTALL_LIBDIR = "$${INSTALL_PREFIX}/lib"

isEmpty(LIBDIR) {
    message("====")
    message("==== NOTE: To override the library installation path run: `qmake LIBDIR=/custom/path'")
    message("==== (current installation path is `$${INSTALL_LIBDIR}')")
} else {
    INSTALL_LIBDIR = $${LIBDIR}
    message("====")
    message("==== install prefix set to `$${INSTALL_LIBDIR}'")
}

PLUGIN_MANIFEST_DIR_BASE = "share/mappero/plugins"
PLUGIN_QML_DIR_BASE="share/mappero/qml-plugins"
MAPPERO_QML_DIR_BASE="share/mappero/import"

PLUGIN_MANIFEST_DIR="$${INSTALL_PREFIX}/$${PLUGIN_MANIFEST_DIR_BASE}"
PLUGIN_QML_DIR="$${INSTALL_PREFIX}/$${PLUGIN_QML_DIR_BASE}"
MAPPERO_QML_DIR="$${INSTALL_PREFIX}/$${MAPPERO_QML_DIR_BASE}"
