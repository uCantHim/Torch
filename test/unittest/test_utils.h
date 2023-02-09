#include <filesystem>
#include <string>

#include <trc/util/DataStorage.h>

namespace fs = std::filesystem;

static std::ostream nullStream{ nullptr };

inline auto makeTempDir() -> fs::path
{
    std::string templateStr{ fs::temp_directory_path() / "torch_unittest.XXXXXX" };
    char* dir = mkdtemp(templateStr.data());
    assert(dir != nullptr);
    return fs::path(dir);
}

class NullDataStorage : public trc::DataStorage
{
public:
    auto read(const path& path) -> std::shared_ptr<std::istream> override { return nullptr; }
    auto write(const path& path) -> std::shared_ptr<std::ostream> override { return nullptr; }
    bool remove(const path& path) override { return false; }
};
