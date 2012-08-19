TOP_SRC_DIR     = $$PWD
TOP_BUILD_DIR   = $${TOP_SRC_DIR}/$(BUILD_DIR)

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

