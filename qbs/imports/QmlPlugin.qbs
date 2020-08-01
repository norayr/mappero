import qbs 1.0

Product {
    type: embedAsResource ? "staticlibrary" : "qmlplugin"

    property bool embedAsResource: project.embedPlugins

    Depends { name: "Qt.core" }

    FileTagger {
        patterns: "*.manifest"
        fileTags: embedAsResource ? ["manifest", "qt.core.resource_data"] : ["manifest"]
    }

    FileTagger {
        patterns: [ "*.png", "*.gif", "*.svg" ]
        fileTags: embedAsResource ? ["image", "qt.core.resource_data"] : ["image"]
    }

    FileTagger {
        patterns: "*.qml"
        fileTags: embedAsResource ? ["qml", "qt.core.resource_data"] : ["qml"]
    }

    Group {
        fileTagsFilter: "manifest"
        Qt.core.resourcePrefix: "/" + project.relativePluginManifestDir
        qbs.install: !embedAsResource
        qbs.installDir: "share/" + project.relativePluginManifestDir
    }

    Group {
        fileTagsFilter: [ "qml", "image" ]
        Qt.core.resourcePrefix: "/" + project.relativePluginDir
        Qt.core.resourceSourceBase: "."
        qbs.install: !embedAsResource
        qbs.installDir: "share/" + project.relativePluginDir
        qbs.installSourceBase: "."
    }

    Export {
        Depends { name: "cpp" }
        cpp.defines: product.embedAsResource ? "STATIC_PLUGINS" : ""
    }
}
