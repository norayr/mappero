TEMPLATE = subdirs
SUBDIRS = \
    path

system(pkg-config --exists exiv2) {
    message("libexiv2 is available")
    SUBDIRS += geotagging
}

check.target = check
check.CONFIG = recursive
QMAKE_EXTRA_TARGETS += check
