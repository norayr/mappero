import qbs

Product {
    id: product

    files: ["**"]
    type: ["application", "autotest"]

    cpp.cxxFlags: {
        if (project.enableCoverage) {
            return ["--coverage"]
        }
    }
    cpp.cxxLanguageVersion: "c++11"
    cpp.debugInformation: true
    cpp.dynamicLibraries: project.enableCoverage ? ["gcov"] : undefined

    Depends { name: 'cpp' }
    Depends { name: 'Qt.core' }
    Depends { name: 'Qt.gui' }
    Depends { name: 'Qt.test' }
}
