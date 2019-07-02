import qbs

Module {
    cpp.driverFlags: project.enableCoverage ? ["--coverage"] : []

    Depends { name: "cpp" }
}
