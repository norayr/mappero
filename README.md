# Mappero

[![build status](https://gitlab.com/mardy/mappero/badges/master/build.svg)](https://gitlab.com/mardy/mappero/commits/master)
[![coverage report](https://gitlab.com/mardy/mappero/badges/master/coverage.svg)](https://gitlab.com/mardy/mappero/commits/master)

Mappero is a map application, [originally written for the Nokia
N900](http://www.mardy.it/mappero/), now coming to other devices.

The portability of the code has been already proven with the release of
[Mappero Geotagger](http://www.mardy.it/mappero-geotagger/), a desktop
application for Linux, Windows and MacOS, build from the very same Mappero
source code.

## Building instructions

Mappero is built with [Qbs](https://doc.qt.io/qbs/) by following these steps:

    qbs setup-toolchains --detect
    qbs setup-qt $(which qmake) qt5
    qbs config defaultProfile qt5

If you wish to build the unit tests, also run

    qbs resolve projects.mappero.buildTests:true

and if you are building under Linux (with gcc, and have `gcov` and `lcov`
installed) you can enable the test coverage report by adding
`projects.mappero.enableCoverage:true` to the previous command.

Finally, you can just run

    qbs

to build the application, and you can use `qbs -b check` to run the unit tests
or `qbs -p coverage` to generate the coverage report.


### Selecting geotag mode vs normal map

By default, geotagging is enabled if exiv2 is found. To explicitly decide
whether Mappero Geotagger should be built, use the `project.geotagger` option,
like this:

    qbs resolve project.geotagger:false


### Building for Android

QBS will happily build Mappero for Android, if a proper profile is selected;
the only thing requiring special care is SSL support (which must be enabled,
otherwise the plugins won't work): you will need to download `libcrypto_1_1.so`
and `libssl_1_1.so` from [this
repository](https://github.com/KDAB/android_openssl/tree/master/latest/arm)
and copy them into your Qt lib directory, that is `<path-to-qt>/android/lib`.
Then QBS should be able to find them and embed them into the `apk` file.
