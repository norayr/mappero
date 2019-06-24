import qbs 1.0

DynamicLibrary {
    name: "MapperoUi"
    cpp.allowUnresolvedSymbols: false
    cpp.cxxLanguageVersion: "c++11"

    files: [
        "types.cpp",
        "types.h",
    ]

    Group {
        name: "qml files"
        prefix: "qml/"
        files: [
            "BusyIndicator.qml",
            "Button.qml",
            "ButtonWithIcon.qml",
            "ImageButton.qml",
            "LabelLayout.qml",
            "ListBrowser.qml",
            "MenuButton.qml",
            "OsmButton.qml",
            "Pane.qml",
            "PaneBackground.qml",
            "Popup.qml",
            "Selector.qml",
            "SelectorView.qml",
            "SimpleSelector.qml",
            "TextField.qml",
            "UIConstants.js",
            "mappero-ui.qrc",
            "qmldir",
        ]
    }

    Depends { name: "buildconfig" }
    Depends { name: "cpp" }
    Depends { name: "Qt.quick" }

    Group {
        fileTagsFilter: "dynamiclibrary"
        qbs.install: true
        qbs.installDir: "lib"
    }
}

