import qbs 1.0

Project {
    name: "mappero"

    property string version: "1.6"
    property bool buildTests: false
    property bool enableCoverage: false

    property string pluginDir: "share/mappero/qml-plugins"
    property string pluginManifestDir: "share/mappero/plugins"

    qbsSearchPaths: "qbs"

    references: [
        "lib/Mappero/Mappero.qbs",
        "lib/MapperoUi/MapperoUi.qbs",
        "src/qt/qt.qbs",
        "src/plugins/google/plugin.qbs",
        "src/plugins/google-directions/plugin.qbs",
        "src/plugins/nominatim/plugin.qbs",
        "src/plugins/reittiopas/plugin.qbs",
        "tests/tests.qbs",
    ]

    AutotestRunner {
        name: "check"
        environment: [ "QT_QPA_PLATFORM=offscreen" ]
        Depends { productTypes: ["coverage-clean"] }
    }
}
