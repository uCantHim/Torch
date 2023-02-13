#include <cstdlib>
#include <string>
#include <unordered_set>
#include <vector>

#include <gtest/gtest.h>
#include <trc/util/FilesystemDataStorage.h>

#include "test_utils.h"

class FilesystemDataStorageTest : public testing::Test
{
protected:
    FilesystemDataStorageTest() = default;

    static inline fs::path rootDir{ makeTempDir() };
};

TEST_F(FilesystemDataStorageTest, InvalidDirectoryThrows)
{
    ASSERT_THROW(trc::FilesystemDataStorage(rootDir / "does/not/exist"), std::invalid_argument);
    ASSERT_NO_THROW(trc::FilesystemDataStorage{rootDir});
}

TEST_F(FilesystemDataStorageTest, ReadWrite)
{
    trc::FilesystemDataStorage storage(rootDir);
    trc::util::Pathlet path("/foo");

    {
        auto os = storage.write(path);
        ASSERT_NE(os, nullptr);
        *os << "Hello, World!";
    }
    {
        auto is = storage.read(path);
        ASSERT_NE(is, nullptr);

        std::string str;
        str.resize(13);
        is->read(str.data(), 13);
        ASSERT_FALSE(is->eof());
        ASSERT_FALSE(is->fail());
        ASSERT_EQ(str, "Hello, World!");

        is->read(str.data(), 1);
        ASSERT_TRUE(is->eof());
        ASSERT_TRUE(is->fail());

        // Read a second time
        auto is2 = storage.read(path);
        ASSERT_NE(is2, nullptr);

        is2->read(str.data(), 13);
        ASSERT_FALSE(is2->eof());
        ASSERT_FALSE(is2->fail());
        ASSERT_EQ(str, "Hello, World!");
    }
}

TEST_F(FilesystemDataStorageTest, Erase)
{
    trc::FilesystemDataStorage storage(rootDir);

    trc::util::Pathlet path("/bar");
    storage.remove(path);
    auto is = storage.read(path);
    ASSERT_EQ(is, nullptr);

    *storage.write(path) << 1234 << " foo\n";
    std::string line;
    std::getline(*storage.read(path), line);
    ASSERT_EQ(line, "1234 foo");

    ASSERT_TRUE(storage.remove(path));
    ASSERT_EQ(storage.read(path), nullptr);
}

TEST_F(FilesystemDataStorageTest, NestedPaths)
{
    trc::FilesystemDataStorage storage(rootDir);

    trc::util::Pathlet path1("/nested/dir/foo.file");
    trc::util::Pathlet path2("/nested/dir/thing/bar.file");

    ASSERT_FALSE(storage.read(path1));
    ASSERT_FALSE(storage.read(path2));
    ASSERT_TRUE(storage.write(path1));
    ASSERT_TRUE(storage.write(path2));

    // foo.file is already a file, so treating it as a directory should fail
    ASSERT_FALSE(storage.write(trc::util::Pathlet("/nested/dir/foo.file/mydata")));
}

TEST_F(FilesystemDataStorageTest, Iterator)
{
    std::vector<trc::util::Pathlet> paths{
        trc::util::Pathlet{ "/foo" },
        trc::util::Pathlet{ "/bar" },
        trc::util::Pathlet{ "/dir/file" },
        trc::util::Pathlet{ "/dir/deep/nested/file2.ext" },
        trc::util::Pathlet{ "/foobar/file.3" },
        trc::util::Pathlet{ "/foobar/file.4" },
        trc::util::Pathlet{ "/baz" },
    };

    fs::create_directories(rootDir / "iterator_test");
    trc::FilesystemDataStorage storage(rootDir / "iterator_test");

    // Test empty
    ASSERT_EQ(storage.begin(), storage.end());
    ASSERT_NO_THROW(for (auto& _ : storage););

    // Populate storage
    for (const auto& path : paths) {
        *storage.write(path) << "Hello! This is a file at \"" + path.string() + "\".";
    }

    // Collect all files in storage
    std::vector<trc::util::Pathlet> collectedPaths;
    for (const trc::util::Pathlet& path : storage) {
        collectedPaths.emplace_back(path);
    }

    std::ranges::sort(paths);
    std::ranges::sort(collectedPaths);
    ASSERT_EQ(paths, collectedPaths);

    // A second time, with one removed element
    collectedPaths.clear();
    storage.remove(paths[2]);
    for (const trc::util::Pathlet& path : storage) {
        collectedPaths.emplace_back(path);
    }

    std::ranges::sort(paths);
    std::ranges::sort(collectedPaths);
    ASSERT_NE(paths, collectedPaths);
    paths.erase(paths.begin() + 2);
    ASSERT_EQ(paths, collectedPaths);
}
