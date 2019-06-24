import qbs

Product {
    name: "coverage-clean"

    builtByDefault: false
    files: ["**"]
    type: ["coverage-clean"]

    Rule {
        multiplex: true
        outputFileTags: "coverage-clean"
        requiresInputs: false
        prepare: {
            var cleanCmd = new Command("find", [
                project.sourceDirectory,
                "-type", "f",
                "(",
                    "-name", "*.gcda",
                    "-o",
                    "-name", "coverage.info",
                ")",
                "-delete",
            ]);
            cleanCmd.description = "Clearing coverage data";
            cleanCmd.highlight = "coverage";
            cleanCmd.silent = false;

            return [cleanCmd];
        }
    }
}
