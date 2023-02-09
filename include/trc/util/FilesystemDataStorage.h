#pragma once

#include <filesystem>

#include "trc/util/DataStorage.h"

namespace trc
{
    namespace fs = std::filesystem;

    /**
     * @brief DataStorage implementation that stores data as files on disk
     */
    class FilesystemDataStorage : public DataStorage
    {
    public:
        explicit FilesystemDataStorage(const fs::path& rootDir);

        auto read(const path& path) -> s_ptr<std::istream> override;
        auto write(const path& path) -> s_ptr<std::ostream> override;

        bool remove(const path& path) override;

    private:
        const fs::path rootDir;
    };
} // namespace trc
