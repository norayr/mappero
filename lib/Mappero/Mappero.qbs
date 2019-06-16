import qbs 1.0

DynamicLibrary {
    name: "Mappero"

    cpp.allowUnresolvedSymbols: false
    cpp.defines: [
        'PLUGIN_QML_DIR="' + project.pluginDir + '"',
    ]
    cpp.cxxLanguageVersion: "c++11"

    files: [
        "gpx.cpp",
        "gpx.h",
        "kml.cpp",
        "kml.h",
        "model-aggregator.cpp",
        "model-aggregator.h",
        "path-builder.cpp",
        "path-builder.h",
        "path.cpp",
        "path.h",
        "plugin.cpp",
        "plugin.h",
        "poi-model.cpp",
        "poi-model.h",
        "projection.cpp",
        "projection.h",
        "routing-model.cpp",
        "routing-model.h",
        "routing-plugin.cpp",
        "routing-plugin.h",
        "search-plugin.cpp",
        "search-plugin.h",
        "types.cpp",
        "types.h",
    ]

    Group {
        name: "qml files"
        prefix: "qml/"
        files: [
            "BaseRouteDelegate.qml",
            "BaseWayPointDelegate.qml",
            "DefaultResultsDelegate.qml",
            "DefaultRouteDelegate.qml",
            "DefaultWayPointDelegate.qml",
            "libmappero.qrc",
            "qmldir",
        ]
    }

    Depends { name: "cpp" }
    Depends { name: "Qt.gui" }
    Depends { name: "Qt.qml" }

    Group {
        fileTagsFilter: "dynamiclibrary"
        qbs.install: true
        qbs.installDir: "lib"
    }

    Export {
        Depends { name: "cpp" }
        cpp.includePaths: [product.sourceDirectory + '/..']
        cpp.libraryPaths: [product.buildDirectory]
    }
}
