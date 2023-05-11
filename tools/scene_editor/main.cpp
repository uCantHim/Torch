#include <filesystem>
#include <string>
namespace fs = std::filesystem;

#include <argparse/argparse.hpp>

#include "App.h"

int main(int argc, char* argv[])
{
    // Process command-line arguments
    argparse::ArgumentParser parser;
    parser.add_argument("--project-root")
        .default_value(std::string{ "." })
        .required()
        .help("Root directory of the current project.");
    parser.parse_args(argc, argv);

    const fs::path projectDir{ parser.get("--project-root") };

    // Run scene editor
    App app{ projectDir };
    app.run();

    return 0;
}
