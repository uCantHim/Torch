#include <filesystem>
namespace fs = std::filesystem;

#include <argparse/argparse.hpp>
#include <trc/util/TorchDirectories.h>

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

    trc::util::setProjectDirectory(parser.get("--project-root"));
    fs::create_directories(trc::util::getAssetStorageDirectory());

    // Run scene editor
    App app(argc, argv);
    app.run();

    return 0;
}
