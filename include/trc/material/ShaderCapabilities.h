#pragma once

#include <functional>
#include <string>
#include <string_view>

namespace trc
{
    class Capability
    {
    public:
        Capability() = delete;
        constexpr Capability(const char* str) : str(str) {}

        auto getString() const -> std::string {
            return std::string(str);
        }

        bool operator==(const Capability& other) const {
            return str == other.str;
        }

    private:
        std::string_view str;
    };
} // namespace trc

template<>
struct std::hash<trc::Capability>
{
    auto operator()(const trc::Capability& capability) const -> size_t
    {
        return hash<std::string_view>{}(capability.getString());
    }
};
