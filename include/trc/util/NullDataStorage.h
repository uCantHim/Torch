#pragma once

#include "trc/util/DataStorage.h"

namespace trc
{
    /**
     * A data storage implementation that does absolutely nothing. Used mainly
     * for testing.
     *
     * Always returns the valid value `nullptr` from `NullDataStorage::read`
     * that indicates that no data at the specified path exists.
     */
    class NullDataStorage : public DataStorage
    {
    public:
        auto read(const path&) -> s_ptr<std::istream> override { return nullptr; }
        auto write(const path&) -> s_ptr<std::ostream> override { return nullptr; }
        bool remove(const path&) override { return false; }
    };
} // namespace trc
