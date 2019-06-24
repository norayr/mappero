import qbs

Product {
    name: "coverage"

    property string outputDirectory: "coverage-html"
    property stringList extractPatterns: []

    builtByDefault: false
    files: ["**"]
    type: ["coverage.html"]

    Depends { productTypes: ["autotest-result"] }

    Rule {
        multiplex: true
        explicitlyDependsOnFromDependencies: ["autotest-result"]
        outputFileTags: "coverage.html"
        requiresInputs: false
        prepare: {
            var commands = []
            var captureCmd = new Command("lcov", [
                "--directory", project.sourceDirectory,
                "--capture",
                "--output-file", "coverage.info",
                "--no-checksum",
                "--compat-libtool",
            ]);
            captureCmd.description = "Collecting coverage data";
            captureCmd.highlight = "coverage";
            captureCmd.silent = false;
            commands.push(captureCmd);

            var extractArgs = []
            for (var i = 0; i < product.extractPatterns.length; i++) {
                extractArgs.push("--extract");
                extractArgs.push("coverage.info");
                extractArgs.push(product.extractPatterns[i]);
            }
            if (product.extractPatterns.length > 0) {
                extractArgs.push("-o");
                extractArgs.push("coverage.info");
                var extractCmd = new Command("lcov", extractArgs);
                extractCmd.description = "Extracting coverage data";
                extractCmd.highlight = "coverage";
                extractCmd.silent = false;
                commands.push(extractCmd);
            }

            var filterCmd = new Command("lcov", [
                "--remove", "coverage.info", 'moc_*.cpp',
                "--remove", "coverage.info", 'qrc_*.cpp',
                "--remove", "coverage.info", '*/tests/*',
                "-o", "coverage.info",
            ]);
            filterCmd.description = "Filtering coverage data";
            filterCmd.highlight = "coverage";
            filterCmd.silent = false;
            commands.push(filterCmd);

            var genhtmlCmd = new Command("genhtml", [
                "--prefix", project.sourceDirectory,
                "--output-directory", product.outputDirectory,
                "--title", "Code coverage",
                "--legend",
                "--show-details",
                "coverage.info",
            ]);
            genhtmlCmd.description = "Generate HTML coverage report";
            genhtmlCmd.highlight = "coverage";
            genhtmlCmd.silent = false;
            commands.push(genhtmlCmd);

            return commands;
        }
    }
}
