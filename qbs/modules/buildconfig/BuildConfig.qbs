import qbs

Module {
    cpp.driverFlags: project.enableCoverage ? ["--coverage"] : []

    Depends { name: "cpp" }

    /*
     * Android-specific section
     */
    Properties {
        condition: qbs.targetOS.contains("android")
        Android.ndk.platform: "android-21"
    }
    Depends {
        condition: qbs.targetOS.contains("android")
        name: "Android.ndk"
    }
}
