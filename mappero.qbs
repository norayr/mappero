import qbs 1.0

Project {
    name: "mappero"

    property string version: "1.9"
    property bool buildTests: false
    property bool enableCoverage: false
    property bool geotagger: true

    property string relativePluginDir: "mappero/qml-plugins"
    property string relativePluginManifestDir: "mappero/plugins"
    property bool embedPlugins: qbs.targetOS.contains("android")

    qbsSearchPaths: "qbs"

    references: [
        "lib/Mappero/Mappero.qbs",
        "lib/MapperoUi/MapperoUi.qbs",
        "src/qt/qt.qbs",
        "src/plugins/plugins.qbs",
        "tests/tests.qbs",
    ]

    AutotestRunner {
        name: "check"
        environment: [ "QT_QPA_PLATFORM=offscreen" ]
        Depends { productTypes: ["coverage-clean"] }
    }
}
