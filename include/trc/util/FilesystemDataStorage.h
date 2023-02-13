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
        /**
         * @throw std::invalid_argument if `rootDir` is not a directory
         */
        explicit FilesystemDataStorage(const fs::path& rootDir);

        auto read(const path& path) -> s_ptr<std::istream> override;
        auto write(const path& path) -> s_ptr<std::ostream> override;

        bool remove(const path& path) override;

        auto begin() -> iterator override;
        auto end() -> iterator override;

    private:
        class FileIterator : public DataStorage::EntryIterator
        {
        public:
            using value_type = path;
            using reference = path&;
            using const_reference = const path&;
            using pointer = path*;
            using const_pointer = const path*;
            using size_type = size_t;

            explicit FileIterator(const fs::path& dir);

            auto operator*() -> reference override;
            auto operator*() const -> const_reference override;
            auto operator->() -> pointer override;
            auto operator->() const -> const_pointer override;

            auto operator++() -> EntryIterator& override;

            bool operator==(const EntryIterator& other) const override;

            static auto makeEnd(const fs::path& rootDir) -> FileIterator;

        private:
            FileIterator(const fs::path& rootDir, fs::recursive_directory_iterator iter);

            fs::path rootDir;
            fs::recursive_directory_iterator iter;
            std::optional<util::Pathlet> current;
        };

        const fs::path rootDir;
    };
} // namespace trc
