#pragma once

#include <cstring>
#include <functional>
#include <string>

namespace trc
{
    class Capability
    {
    public:
        Capability() = delete;
        constexpr Capability(const char* str) : str(str) {}

        auto getString() const -> std::string {
            return str;
        }

        bool operator==(const Capability& other) const {
            return strcmp(str, other.str) == 0;
        }

    private:
        const char* str;
    };

    struct FragmentCapability
    {
        static constexpr Capability kVertexWorldPos{ "frag_vertexWorldPos" };
        static constexpr Capability kVertexNormal{ "frag_vertexNormal" };
        static constexpr Capability kVertexUV{ "frag_vertexUV" };

        static constexpr Capability kTime{ "frag_currentTime" };
        static constexpr Capability kTimeDelta{ "frag_frameTime" };

        static constexpr Capability kTextureSample{ "frag_textureSample" };
    };
} // namespace trc

template<>
struct std::hash<trc::Capability>
{
    auto operator()(const trc::Capability& capability) const -> size_t
    {
        return hash<std::string>{}(capability.getString());
    }
};
