TEMPLATE = subdirs
SUBDIRS = \
    path

check.target = check
check.CONFIG = recursive
QMAKE_EXTRA_TARGETS += check
