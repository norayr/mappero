import qbs 1.0

Test {
    name: "path-test"

    files: [
        "path-test.cpp",
        "path-test.h",
        "paths.qrc",
    ]

    Depends { name: "Mappero" }
    cpp.rpaths: cpp.libraryPaths
}

