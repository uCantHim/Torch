#include <iostream>
#include <filesystem>
#include <format>

#include <argparse/argparse.hpp>
#include <trc/assets/import/AssetImport.h>
#include <trc/assets/import/InternalFormat.h>
#include <trc/text/Font.h>
#include <trc/util/TorchDirectories.h>

namespace fs = std::filesystem;

using ConverterFunc = void(*)(const fs::path&, const fs::path&, argparse::ArgumentParser&);

void inferConversionType(const fs::path& input, const fs::path& outPath, argparse::ArgumentParser&);
void convertGeometry(const fs::path& input, const fs::path& outPath, argparse::ArgumentParser&);
void convertTexture(const fs::path& input, const fs::path& outPath, argparse::ArgumentParser&);
void convertFont(const fs::path& input, const fs::path& outPath, argparse::ArgumentParser&);

int main(const int argc, const char** argv)
{
    argparse::ArgumentParser program;
    program.add_argument("file");

    program.add_argument("--type")
        .default_value(inferConversionType)
        .action([&](const std::string& arg) -> ConverterFunc {
            using Set = std::unordered_set<std::string>;
            if (Set{ "geo", "geometry", "geometries" }.contains(arg)) {
                return convertGeometry;
            }
            if (Set{ "tex", "texture" }.contains(arg)) {
                return convertTexture;
            }
            if (Set{ "font" }.contains(arg)) {
                return convertFont;
            }
            else if (Set{ "auto", "default" }.contains(arg)) {
                return inferConversionType;
            }
            throw std::runtime_error(std::format("Value not allowed for '--type' argument: {}", arg));
        })
        .help("Allowed values are 'auto' (default), 'geometry', 'texture', 'font'. Note that the"
              " 'auto' feature, which tries to infer the file's format automatically, detects"
              " fewer file formats than can actually be processed by this tool. If you are sure"
              " that Torch's format converter supports your file type but it is not detected"
              " correctly, then specify its type manually.");

    program.add_argument("--font-size")
        .default_value(18)
        .scan<'i', uint>()
        .help("If importing fonts (--type font), specify which font size to import.");

    // Parse command-line args
    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cout << program;
        exit(64);
    }

    // Run
    try {
        const fs::path input = program.get("file");
        if (!fs::is_regular_file(input))
        {
            std::cout << "Error: " << input << " is not a file\n";
            exit(1);
        }

        fs::path output = "";
        if (output.empty()) output = input.filename();
        output.replace_extension("ta");

        if (auto dir = output.parent_path(); !dir.empty()) {
            fs::create_directories(dir);
        }

        auto converter = program.get<ConverterFunc>("--type");
        converter(input, output, program);
    }
    catch (const std::exception& err)
    {
        std::cout << "Error: " << err.what() << "\n";
        exit(1);
    }

    return 0;
}

void inferConversionType(
    const fs::path& input,
    const fs::path& outPath,
    argparse::ArgumentParser& args)
{
    using StrSet = std::unordered_set<std::string>;

    const auto ext = input.extension().string();
    if (StrSet{ ".fbx", ".obj", ".dae" }.contains(ext)) {
        convertGeometry(input, outPath, args);
    }
    else if (StrSet{ ".png", ".jpg", ".jpeg", ".tif", ".bmp" }.contains(ext)) {
        convertTexture(input, outPath, args);
    }
    else if (StrSet{ ".ttf", ".otf" }.contains(ext)) {
        convertFont(input, outPath, args);
    }
    else {
        throw std::invalid_argument(std::format("Unable to detect file type of {}.", input.string()));
    }
}

void convertGeometry(const fs::path& input, const fs::path& /*outPath*/, argparse::ArgumentParser&)
{
    trc::ThirdPartyFileImportData data;
    try {
        data = trc::loadAssets(input);
        if (data.meshes.empty())
        {
            std::cout << "Nothing to import. Exiting.";
            return;
        }
    }
    catch (const trc::DataImportError& err) {
        std::cout << "Unable to import geometries from " << input << ": " << err.what() << "\n";
        exit(1);
    }

    std::cout << "Loaded " << data.meshes.size() << " meshes:\n";
    for (const auto& mesh : data.meshes)
    {
        std::cout << " - " << mesh.name << " ("
                  << mesh.materials.size() << " materials, "
                  << mesh.animations.size() << " animations, "
                  << (mesh.rig.has_value() ? "1 rig" : "no rig")
                  << ")\n";
    }

    std::cout << "\nThis is a test build. No data was actually written to file.\n";
}

void convertTexture(const fs::path& input, const fs::path& outPath, argparse::ArgumentParser&)
{
    std::ofstream file(outPath);
    trc::loadTexture(input).serialize(file);
    std::cout << "Stored texture " << input << " at " << outPath << "\n";
}

void convertFont(const fs::path& input, const fs::path& outPath, argparse::ArgumentParser& args)
{
    const uint size = args.get<uint>("--font-size");

    std::ofstream file(outPath);
    trc::loadFont(input, size).serialize(file);
    std::cout << "Stored font " << input << " (size " << size << ") at " << outPath << "\n";
}
