#
# Copyright (C) 2006-2009 John Costigan.
# Copyright (C) 2009-2010 Alberto Mardegan.
#
# This file is part of Maemo Mapper.
#
# Maemo Mapper is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# Maemo Mapper is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with Maemo Mapper.  If not, see <http://www.gnu.org/licenses/>.
#

AC_INIT(Makefile.am)
AM_INIT_AUTOMAKE(maemo-mapper, 3.1)

AC_PROG_CXX
AC_PROG_CPP
AC_CONFIG_HEADERS(config.h)
AC_PROG_INSTALL
AC_PROG_LIBTOOL

# If API has been removed or changed since last release, change MAP_API (which
# is part of the directory name under /usr/include) to the version that
# changed it
MAP_API_VERSION=1.0
AC_SUBST([MAP_API_VERSION])

# If ABI has been removed or changed since last release
#     increment current
#     set age and revision to 0
# else if ABI has been added since last release
#     increment age and current
#     set revision to 0
# else if library source has changed since last release
#     increment revision
MAP_LT_CURRENT=0
MAP_LT_REVISION=0
MAP_LT_AGE=0
AC_SUBST([MAP_LT_CURRENT])
AC_SUBST([MAP_LT_REVISION])
AC_SUBST([MAP_LT_AGE])
# The ABI version appears in the name of the plugin directory
MAP_ABI_VERSION=`expr ${MAP_LT_CURRENT} - ${MAP_LT_AGE}`
AC_SUBST([MAP_ABI_VERSION])

if $PKG_CONFIG --exists libosso ; then
	echo "Building for Maemo"
	QT_EXTRA_MODULES=QtMaemo5

	AC_SUBST(OSSO_LIBS)
	AC_SUBST(OSSO_CFLAGS)
fi

PKG_CHECK_MODULES(QT, QtDeclarative QtNetwork QtOpenGL $QT_EXTRA_MODULES)
AC_SUBST(QT_LIBS)
AC_SUBST(QT_CFLAGS)

# Debug
AC_ARG_ENABLE(debug,
    AS_HELP_STRING([--disable-debug], [compile without debug code]),
    [enable_debug=${enableval}], enable_debug=yes )
if test "x$enable_debug" = "xyes"; then
    CFLAGS="$CFLAGS -DENABLE_DEBUG"
fi

CFLAGS="-std=c99 -Wall -Werror -Wno-switch $CFLAGS"

AC_DEFINE_UNQUOTED([LOCALEDIR], "${localedir}", [Runtime locale catalog files path])
AC_DEFINE_UNQUOTED([PIXMAPDIR], "${pixmapdir}", [Runtime pixmap files path])


# Produce output files
AC_OUTPUT(
          Makefile \
          src/Makefile \
          src/qt/Makefile \
          data/Makefile \
          data/sounds/Makefile
)

