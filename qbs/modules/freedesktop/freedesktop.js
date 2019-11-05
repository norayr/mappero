var TextFile = require("qbs.TextFile");

function parseDesktopFile(filePath) {
    var file = new TextFile(filePath);
    var content = file.readAll();
    file.close();
    var lines = content.split('\n');
    var fileValues = {};
    var currentSection = {};
    var sectionRex = /\[(.+)\]/g;
    for (var i = 0; i < lines.length; i++) {
        var line = lines[i];
        if (line.length == 0) continue;
        if (line[0] == '#') continue;
        if (match = sectionRex.exec(line)) {
            var currentSectionName = match[1];
            fileValues[currentSectionName] = {};
            currentSection = fileValues[currentSectionName];
            continue;
        }
        var parts = line.split('=', 2);
        currentSection[parts[0]] = parts[1]
    }
    return fileValues
}

function dumpDesktopFile(filePath, keys) {
    file = new TextFile(filePath, TextFile.WriteOnly);
    for (sectionName in keys) {
        file.writeLine('[' + sectionName + ']');
        var section = keys[sectionName];
        for (key in section) {
            var line = key + '=' + section[key];
            file.writeLine(line);
        }
        // Write an empty line between sections (and before EOF)
        file.writeLine('');
    }
    file.close();
}
