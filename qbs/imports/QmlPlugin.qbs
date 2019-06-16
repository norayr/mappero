import qbs 1.0

Product {
    type: "qmlplugin"

    FileTagger {
        patterns: "*.manifest"
        fileTags: "manifest"
    }

    FileTagger {
        patterns: [ "*.png", "*.gif", "*.svg" ]
        fileTags: "image"
    }

    FileTagger {
        patterns: "*.qml"
        fileTags: "qml"
    }

    Group {
        fileTagsFilter: "manifest"
        qbs.install: true
        qbs.installDir: project.pluginManifestDir
    }

    Group {
        fileTagsFilter: [ "qml", "image" ]
        qbs.install: true
        qbs.installDir: project.pluginDir
        qbs.installSourceBase: "../"
    }
}
