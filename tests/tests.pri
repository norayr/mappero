include(../common-config.pri)

INCLUDEPATH += \
    $${TOP_SRC_DIR}/lib

QMAKE_LIBDIR += \
    $${TOP_BUILD_DIR}/lib/Mappero
    $${TOP_BUILD_DIR}/lib/MapperoUi
QMAKE_RPATHDIR = $${QMAKE_LIBDIR}
