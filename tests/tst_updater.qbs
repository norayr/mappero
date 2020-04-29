import qbs 1.0

Test {
    name: "updater-test"

    property path srcDir: project.sourceDirectory + "/src/qt/"
    cpp.includePaths: [ srcDir ]
    cpp.rpaths: cpp.libraryPaths

    files: [
        "fake_network.h",
        "tst_updater.cpp",
    ]
    Group {
        prefix: srcDir
        files: [ "updater.cpp", "updater.h" ]
    }

    Depends { name: "Qt.network" }
    Depends { name: "Qt.qml" }
}

