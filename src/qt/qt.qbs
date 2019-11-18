import qbs 1.0
import qbs.FileInfo

QtGuiApplication {
    property string libPath: qbs.targetOS.contains("darwin") ?
        "../Frameworks" : "../lib"

    name: "mappero"
    version: project.version
    install: true

    cpp.defines: {
        var defines = [
            'MAPPERO_VERSION="' + project.version + '"',
            'PLUGIN_MANIFEST_DIR="' + project.pluginManifestDir + '"',
        ]
        if (qbs.targetOS.contains("unix"))
            defines.push('XDG_THUMBNAILS')
        if (exiv2.present)
            defines.push('GEOTAGGING_ENABLED')
        if (Qt.core.staticBuild)
            defines.push('STATIC_BUILD')
        return defines
    }
    cpp.cxxLanguageVersion: "c++11"
    cpp.rpaths: cpp.rpathOrigin + "/" + libPath

    qbs.installPrefix: {
        if (qbs.targetOS.contains("darwin"))
            return ""
        return original
    }

    files: [
        "application.cpp",
        "application.h",
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
        "../../data/MapperoInfo.plist",
        "../../data/icons/scalable/icons.qrc",
        "../../data/icons/windows/mappero-geotagger.rc",
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
            "qml/GeoTagPage.qml",
            "qml/ToolBar.qml",
            "qml/geotag.qrc",
            "../../data/icons/scalable/icons-geotag.qrc",
            "../../data/mappero-geotagger.desktop",
            "../../data/it.mardy.mappero-geotagger.metainfo.xml",
        ]
    }

    Group {
        name: "qml files"
        prefix: "qml/"
        files: [
            "mappero.qml",
            "qml.qrc",
            "qmldir",
        ]
    }

    Group {
        condition: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("darwin")
        files: [
            "../../data/icons/scalable/mappero-geotagger.svg",
        ]
        qbs.install: product.install
        qbs.installDir: freedesktop.appIconDir
    }

    Depends { name: "buildconfig" }
    Depends { name: "cpp" }
    Depends { name: "freedesktop" }
    Depends { name: "pkgconfig" }
    Depends { name: "Qt.core" }
    Depends { name: "Qt.quick" }
    Depends { name: "Qt.svg"; condition: Qt.core.staticBuild }
    Depends { name: "Qt.widgets"; condition: qbs.targetOS.contains("linux") }
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

    Properties {
        condition: Qt.core.staticBuild
        pkgconfig.staticMode: true
    }

    freedesktop.desktopKeys: ({
        'Exec': FileInfo.joinPaths(qbs.installPrefix,
                                   product.installDir,
                                   product.targetName) + ' --geotag',
        'X-Application-Version': product.version,
    })

    FileTagger {
        patterns: [ "*.plist" ]
        fileTags: "infoplist"
    }
}
