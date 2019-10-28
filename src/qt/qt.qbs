import qbs 1.0

QtGuiApplication {
    name: "mappero"

    cpp.defines: [
        'MAPPERO_VERSION="' + project.version + '"',
        'PLUGIN_MANIFEST_DIR="' + project.pluginManifestDir + '"',
    ]
    cpp.cxxLanguageVersion: "c++11"

    files: [
        "configuration.cpp",
        "configuration.h",
        "controller.cpp",
        "controller.h",
        "gps.cpp",
        "gps.h",
        "layer.cpp",
        "layer.h",
        "main.cpp",
        "map.cpp",
        "map.h",
        "map-object.cpp",
        "map-object.h",
        "mark.cpp",
        "mark.h",
        "path-item.cpp",
        "path-item.h",
        "path-layer.cpp",
        "path-layer.h",
        "plugin-manager.cpp",
        "plugin-manager.h",
        "plugin-model.cpp",
        "plugin-model.h",
        "poi-item.cpp",
        "poi-item.h",
        "poi-view.cpp",
        "poi-view.h",
        "tile-cache.cpp",
        "tile-cache.h",
        "tile-download.cpp",
        "tile-download.h",
        "tile.cpp",
        "tile.h",
        "tiled-layer.cpp",
        "tiled-layer.h",
        "tracker.cpp",
        "tracker.h",
        "types.cpp",
        "types.h",
        "../../data/icons/scalable/icons.qrc",
    ]

    Group {
        name: "geotagger"
        condition: exiv2.present
        files: [
            "taggable-area.cpp",
            "taggable-area.h",
            "taggable-selection.cpp",
            "taggable-selection.h",
            "taggable.cpp",
            "taggable.h",
            "ticks.cpp",
            "ticks.h",
            "qml/geotag.qrc",
            "../../data/icons/scalable/icons-geotag.qrc",
        ]
    }

    Group {
        name: "qml files"
        prefix: "qml/"
        files: [
            "qml.qrc",
            "qmldir",
        ]
    }

    Depends { name: "buildconfig" }
    Depends { name: "cpp" }
    Depends { name: "Qt.quick" }
    Depends { name: "Mappero" }
    Depends { name: "MapperoUi" }
    Depends {
        name: "exiv2"
        required: false
    }
    Depends { name: "Qt.concurrent"; condition: exiv2.present }

    Properties {
        condition: exiv2.present
        overrideListProperties: false
        cpp.defines: outer.concat([
            'GEOTAGGING_ENABLED',
        ])
    }

    Group {
        fileTagsFilter: "application"
        qbs.install: true
        qbs.installDir: "bin"
    }
}
