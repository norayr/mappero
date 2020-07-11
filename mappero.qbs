import qbs 1.0

Project {
    name: "mappero"

    property string version: "1.9"
    property bool buildTests: false
    property bool enableCoverage: false
    property bool geotagger: true

    property string relativePluginDir: "mappero/qml-plugins"
    property string relativePluginManifestDir: "mappero/plugins"

    qbsSearchPaths: "qbs"

    references: [
        "lib/Mappero/Mappero.qbs",
        "lib/MapperoUi/MapperoUi.qbs",
        "src/qt/qt.qbs",
        "src/plugins/google/plugin.qbs",
        "src/plugins/google-directions/plugin.qbs",
        "src/plugins/nominatim/plugin.qbs",
        "src/plugins/reittiopas/plugin.qbs",
        "src/plugins/yandex/plugin.qbs",
        "tests/tests.qbs",
    ]

    AutotestRunner {
        name: "check"
        environment: [ "QT_QPA_PLATFORM=offscreen" ]
        Depends { productTypes: ["coverage-clean"] }
    }
}
