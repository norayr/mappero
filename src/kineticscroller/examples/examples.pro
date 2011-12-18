TEMPLATE      = subdirs
SUBDIRS       = graphicsview \
                wheel

!no-webkit:SUBDIRS += plot

# install
sources.files = *.pro
sources.path = $$[QT_INSTALL_EXAMPLES]/scroller
INSTALLS += sources
