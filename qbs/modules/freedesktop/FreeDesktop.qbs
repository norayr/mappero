import qbs
import qbs.ModUtils
import qbs.TextFile
import "freedesktop.js" as Fdo

Module {
    id: fdoModule

    property var desktopKeys

    readonly property var defaultDesktopKeys: {
        return {
            'Type': 'Application',
            'Name': product.targetName,
            'Exec': product.targetName,
            'Terminal': 'false',
            'Version': '1.1',
        }
    }
    property bool _fdoSupported: qbs.targetOS.contains("unix") && !qbs.targetOS.contains("darwin")

    FileTagger {
        patterns: [ "*.desktop" ]
        fileTags: [ "desktopfile_source" ]
    }

    Rule {
        condition: _fdoSupported

        inputs: [ "desktopfile_source" ]
        outputFileTags: [ "desktopfile", "application" ]
        outputArtifacts: [
            {
                fileTags: [ "desktopfile" ],
                filePath: input.fileName
            }
        ]

        prepare: {
            var cmd = new JavaScriptCommand();
            cmd.description = input.fileName + "->" + output.fileName;
            cmd.desktopKeys = ModUtils.moduleProperty(product, "desktopKeys") || {}
            cmd.highlight = "codegen";
            cmd.sourceCode = function() {
                var aggregateDesktopKeys = Fdo.parseDesktopFile(input.filePath);
                var mainSection = aggregateDesktopKeys['Desktop Entry'];
                for (key in desktopKeys) {
                    if (desktopKeys.hasOwnProperty(key)) {
                        mainSection[key] = desktopKeys[key];
                    }
                }

                var defaultValues = ModUtils.moduleProperty(product, "defaultDesktopKeys");
                for (key in defaultValues) {
                    if (defaultValues.hasOwnProperty(key) && !(key in mainSection)) {
                        mainSection[key] = defaultValues[key];
                    }
                }

                Fdo.dumpDesktopFile(output.filePath, aggregateDesktopKeys);
            }
            return [cmd];
        }
    }

    Group {
        condition: fdoModule._fdoSupported
        fileTagsFilter: [ "desktopfile" ]
        qbs.install: true
        qbs.installDir: "share/applications"
    }

    FileTagger {
        patterns: [ "*.metainfo.xml", "*.appdata.xml" ]
        fileTags: [ "appstream" ]
    }

    Group {
        condition: fdoModule._fdoSupported
        fileTagsFilter: [ "appstream" ]
        qbs.install: true
        qbs.installDir: "share/metainfo"
    }
}
