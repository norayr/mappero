import qbs 1.0

Project {
    condition: project.buildTests

    references: [
        "path/path.qbs",
    ]
}
