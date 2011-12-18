TEMPLATE = app
SOURCES = main.cpp

QTSCROLLER_OUT = $$OUT_PWD/../../
include(../../qtscroller.pri)

# install
target.path = $$[QT_INSTALL_EXAMPLES]/scroller/graphicsview
sources.files = $$SOURCES $$HEADERS $$RESOURCES $$FORMS graphicsview.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/scroller/graphicsview
INSTALLS += target sources
