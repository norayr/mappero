import qbs 1.0

Project {
    condition: project.buildTests

    references: [
        "path/path.qbs",
        "tst_updater.qbs",
    ]

    CoverageClean {
        condition: project.enableCoverage
    }

    CoverageReport {
        condition: project.enableCoverage
        extractPatterns: [ '*/src/*.cpp', '*/lib/*.cpp' ]
    }
}
