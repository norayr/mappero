import qbs

Module {
    cpp.cxxFlags: project.enableCoverage ? ["--coverage"] : undefined
    cpp.dynamicLibraries: project.enableCoverage ? ["gcov"] : undefined

    Depends { name: "cpp" }
}
