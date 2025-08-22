#include <iostream>
#include <filesystem>
#include <format>
#include <fstream>

#include <argparse/argparse.hpp>
#include <trc/assets/import/AssetImport.h>
#include <trc/assets/import/InternalFormat.h>
#include <trc/text/Font.h>
#include <trc/util/TorchDirectories.h>

namespace fs = std::filesystem;

enum class FileType
{
    eGeometry,
    eTexture,
    eFont,
};

constexpr auto kGeoFileExt  = ".geo.ta";
constexpr auto kMatFileExt  = ".mat.ta";
constexpr auto kRigFileExt  = ".rig.ta";
constexpr auto kAnimFileExt = ".anim.ta";
constexpr auto kTexFileExt{ ".tex.ta" };
constexpr auto kFontFileExt{ ".font.ta" };

constexpr auto kDefaultFontSize{ 18 };

constexpr auto kInvalidUsageExitcode{ 64 };

auto parseFileTypeString(const std::string& typeStr, const fs::path& inputFile) -> FileType;

void convertGeometry(const fs::path& input, const fs::path& outPath, argparse::ArgumentParser&);
void convertTexture(const fs::path& input, const fs::path& outPath, argparse::ArgumentParser&);
void convertFont(const fs::path& input, const fs::path& outPath, argparse::ArgumentParser&);

template<trc::AssetBaseType T>
bool writeTo(const trc::AssetData<T>& data, const fs::path& fromPath, const fs::path& dstPath)
{
    std::ofstream file(dstPath);
    if (!file.is_open())
    {
        std::cout << "Error: Unable to write to file " << dstPath << ". Skipping.\n";
        return false;
    }

    trc::serializeAsset(data, file);
    std::cout << "Exported " << T::name() << " from " << fromPath << " to " << dstPath << ".\n";
    return true;
}

void printError(const trc::import::ImportError& err)
{
    std::cout << "Error when importing from " << err.filePath << ": " << err.msg << "\n";
}

int main(const int argc, const char** argv)
{
    argparse::ArgumentParser program;
    program.add_description("Convert common asset formats into Torch's asset format.");

    program.add_argument("file");

    program.add_argument("--type")
        .default_value("auto")
        .help("Allowed values are 'auto' (default), 'geometry', 'texture', 'font'. Note that the"
              " 'auto' feature, which tries to infer the file's format automatically, detects"
              " fewer file formats than can actually be processed by this tool. If you are sure"
              " that Torch's format converter supports your file type but it is not detected"
              " correctly, then specify its type manually.");

    program.add_argument("--dry-run", "-n")
        .default_value(false)
        .implicit_value(true)
        .help("Only print contents of input files, don't convert or output anything.");

    program.add_argument("-o")
        .help("Name of the output file. Used as a name for the output directory if"
              " multiple assets are exported from a single file; this is the case with"
              " geometry file formats.");

    program.add_argument("-a", "--all")
        .help("Export all assets found in geometry/scene files, not just geometries.")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--rigs")
        .help("Also export skeletal rigs from geometry file types.")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--animations")
        .help("Also export animations from geometry file types.")
        .default_value(false)
        .implicit_value(true);
    program.add_argument("--materials")
        .help("Also export materials from geometry file types.")
        .default_value(false)
        .implicit_value(true);

    program.add_argument("--font-size")
        .default_value(uint{kDefaultFontSize})
        .scan<'i', uint>()
        .help("If importing fonts (--type font), specify which font size to import.");

    // Parse command-line args
    try {
        program.parse_args(argc, argv);
    }
    catch (const std::runtime_error& err) {
        std::cout << program;
        exit(kInvalidUsageExitcode);
    }

    // Run
    try {
        const fs::path input = program.get("file");
        if (!fs::is_regular_file(input))
        {
            std::cout << "Error: " << input << " is not a file\n";
            exit(1);
        }

        const auto type = parseFileTypeString(program.get("type"), input);

        fs::path output = program.present<std::string>("o").value_or("");
        if (output.empty())
        {
            // Geometry files don't get a custom extension because the file name
            // is interpreted as a directory instead.
            output = input.filename().replace_extension(
                  type == FileType::eTexture ? kTexFileExt
                : type == FileType::eFont ? kFontFileExt
                : ".out"
                );
        }

        if (output.has_parent_path()) {
            fs::create_directories(output.parent_path());
        }

        // Import the appropriate file type
        switch (type)
        {
        case FileType::eGeometry:
            convertGeometry(input, output, program);
            break;
        case FileType::eTexture:
            convertTexture(input, output, program);
            break;
        case FileType::eFont:
            convertFont(input, output, program);
            break;
        }
    }
    catch (const std::exception& err)
    {
        std::cout << "Error: " << err.what() << "\n";
        exit(1);
    }

    return 0;
}

auto parseFileTypeString(const std::string& typeStr, const fs::path& inputFile) -> FileType
{
    using Set = std::unordered_set<std::string>;

    if (Set{ "geo", "geometry", "geometries" }.contains(typeStr)) {
        return FileType::eGeometry;
    }
    if (Set{ "tex", "texture" }.contains(typeStr)) {
        return FileType::eTexture;
    }
    if (Set{ "font" }.contains(typeStr)) {
        return FileType::eFont;
    }
    if (Set{ "auto", "default" }.contains(typeStr))
    {
        const auto ext = fs::path{inputFile}.extension().string();
        if (Set{ ".fbx", ".obj", ".dae" }.contains(ext)) {
            return FileType::eGeometry;
        }
        if (Set{ ".png", ".jpg", ".jpeg", ".tif", ".bmp" }.contains(ext)) {
            return FileType::eTexture;
        }
        if (Set{ ".ttf", ".otf" }.contains(ext)) {
            return FileType::eFont;
        }
        // Fallback: try to parse files with unknown extensions as scene file types
        return FileType::eGeometry;
    }

    throw std::runtime_error(std::format("Value not allowed for '--type' argument: {}.", typeStr));
}

void convertGeometry(
    const fs::path& input,
    const fs::path& outPath,
    argparse::ArgumentParser& args)
{
    const bool dryRun = args.get<bool>("dry-run");

    const bool exportAll        = args.get<bool>("all");
    const bool exportRigs       = exportAll || args.get<bool>("rigs");
    const bool exportAnimations = exportAll || args.get<bool>("animations");
    const bool exportMaterials  = exportAll || args.get<bool>("materials");

    auto data = trc::importAssets(input);
    if (!data) {
        throw std::runtime_error(data.error().msg);
    }

    if (data->geometries.empty())
    {
        std::cout << "Nothing to import. Exiting.";
        return;
    }

    if (dryRun)
    {
        std::cout << input << " contains " << data->geometries.size() << " meshes:\n";
        for (const auto& [idx, geo] : data->geometries | std::views::enumerate)
        {
            const bool hasRig = data->refs.geoToRig.contains(trc::import::GeoID{ idx });
            std::cout << " - " << geo.name << " ("
                      << (hasRig ? "1 rig" : "no rig")
                      << ")\n";
        }
        for (const auto& [idx, rig] : data->rigs | std::views::enumerate)
        {
            const trc::import::RigID rigId{ idx };
            const bool hasAnims = data->refs.rigToAnimations.contains(rigId);
            std::cout << " - " << rig.name << " (";
            if (hasAnims) {
                std::cout << data->refs.rigToAnimations[rigId].size() << " animations";
            }
            std::cout << ")\n";
        }
        return;
    }

    // Output logic for geometries is a bit complex because one file might
    // contain multiple geometries.
    try {
        fs::create_directories(outPath);
    }
    catch (const fs::filesystem_error& err) {
        throw std::runtime_error(std::format("Unable to create output directory {}: {}",
                                             outPath.string(), err.what()));
    }

    for (auto [_meshIdx, mesh] : data->geometries | std::views::enumerate)
    {
        auto tryWrite = [&]<typename T>(const trc::AssetData<T>& data, const fs::path& fileName)
            -> std::optional<trc::AssetPath>
        {
            const auto filePath = outPath / fileName;
            if (writeTo(data, input, fileName)) {
                return trc::AssetPath{ fileName };
            }
            return std::nullopt;
        };

        const trc::import::GeoID meshIdx{ _meshIdx };
        auto rig = data->getRig(meshIdx);

        // Export additional assets if enabled
        if (exportAnimations)
        {
            // Clear references to animations and replace them with asset
            // paths.
            if (rig) {
                rig->data.animations.clear();
            }

            for (const auto& anim : data->animations)
            {
                auto path = tryWrite(anim.data, anim.name + kAnimFileExt);
                if (path && rig) {
                    rig->data.animations.emplace_back(*path);
                }
            }
        }
        if (exportRigs && rig)
        {
            if (auto path = tryWrite(rig->data, rig->name + kRigFileExt)) {
                mesh.data.rig = *path;
            }
        }
        if (exportMaterials)
        {
            for (const auto& mat : data->materials) {
                tryWrite(trc::makeMaterial(mat.data), mat.name + kMatFileExt);
            }
        }

        // Always export the geometry
        tryWrite(mesh.data, mesh.name + kGeoFileExt);
    }

    std::cout << "Exported data from " << input << " to " << outPath << ".\n";
}

void convertTexture(
    const fs::path& input,
    const fs::path& outPath,
    argparse::ArgumentParser& args)
{
    const bool dryRun = args.get<bool>("dry-run");

    const auto tex = trc::importTexture(input);
    if (!tex) {
        printError(tex.error());
        return;
    }

    if (dryRun)
    {
        std::cout << input << " contains an image of size "
                  << tex->size.x << "x" << tex->size.y << ".\n";
        return;
    }

    writeTo(*tex, input, outPath);
}

void convertFont(const fs::path& input, const fs::path& outPath, argparse::ArgumentParser& args)
{
    const bool dryRun = args.get<bool>("dry-run");
    const uint size = args.get<uint>("font-size");

    const auto font = trc::importFont(input, size);
    if (dryRun)
    {
        std::cout << input << " contains a font.\n";
        return;
    }

    writeTo(*font, input, outPath);
}
