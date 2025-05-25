#include "trc/material/ShaderCache.h"

#include <SQLiteCpp/SQLiteCpp.h>

#include "trc/base/Logging.h"



namespace trc
{

struct ShaderCache::Impl
{
    SQLite::Database db;
};

ShaderCache::ShaderCache(const fs::path& cacheFile)
{
    try {
        impl = std::make_shared<Impl>(
            SQLite::Database{
                cacheFile,
                SQLite::OPEN_CREATE | SQLite::OPEN_READWRITE | SQLite::OPEN_FULLMUTEX
            }
        );
        tryCreateTable();
    }
    catch (const std::exception& err) {
        log::error << "[ShaderCache] Error when opening cache: " << err.what();
    }
}

auto ShaderCache::query(const std::string& glsl) -> std::optional<Entry>
{
    try {
        SQLite::Statement query{
            impl->db,
            "SELECT spirv, timestamp FROM spirv_cache WHERE key = ?"
        };
        query.bind(1, glsl);

        while (query.executeStep())
        {
            auto spirvCode = query.getColumn("spirv").getBlob();
            auto spirvSize = query.getColumn("spirv").getBytes();
            auto timestamp = query.getColumn("timestamp").getText();

            // Extract binary data
            assert(spirvSize % 4 == 0);
            if (spirvSize == 0) {
                return std::nullopt;
            }
            std::vector<ui32> spirv(spirvSize / 4);
            memcpy(spirv.data(), reinterpret_cast<const ui8*>(spirvCode), spirvSize);

            // Parse timestamp
            std::stringstream ss{ timestamp };
            std::chrono::system_clock::time_point tp;
            std::chrono::from_stream(ss, "%FT%TZ", tp);

            // Create result
            return Entry{
                .spirvCode=std::move(spirv),
                .timestamp=tp
            };
        }
    }
    catch (const std::exception& err) {
        log::error << "[ShaderCache] Error when querying cache entry: " << err.what();
    }

    return std::nullopt;
}

void ShaderCache::store(const std::string& glsl, const Entry& entry)
{
    try {
        SQLite::Statement query{
            impl->db,
            "INSERT OR REPLACE INTO spirv_cache"
            "(key, spirv, timestamp) VALUES (?, ?, ?)"
        };
        query.bind(1, glsl);
        query.bind(2, entry.spirvCode.data(),
                      entry.spirvCode.size() * sizeof(decltype(entry.spirvCode)::value_type));
        query.bind(3, std::format("{:%FT%TZ}", entry.timestamp));

        query.exec();
    }
    catch (const std::exception& err) {
        log::error << "[ShaderCache] Error when storing cache entry: " << err.what();
    }
}

void ShaderCache::clear()
{
    try {
        impl->db.exec("DROP TABLE IF EXISTS spirv_cache");
        tryCreateTable();
    }
    catch (const std::exception& err) {
        log::error << "[ShaderCache] Error when clearing cache: " << err.what();
    }
}

void ShaderCache::tryCreateTable()
{
    impl->db.exec(
        "CREATE TABLE IF NOT EXISTS spirv_cache("
            "key TEXT PRIMARY KEY,"
            "spirv BLOB,"
            "timestamp DATETIME NOT NULL"
        ")"
    );
}

} // namespace trc
