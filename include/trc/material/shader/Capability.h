#pragma once

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace trc::shader
{
    class Capability
    {
    public:
        Capability() = delete;

        Capability(Capability&& other) noexcept
            : name(std::make_shared<std::string>(*other.name)) {}

        Capability& operator=(Capability&& other) noexcept
        {
            if (this != &other) {
                name = std::make_shared<std::string>(*other.name);
            }
            return *this;
        }

        Capability(const Capability&) = default;
        Capability& operator=(const Capability&) = default;

        Capability(const char* str) : name(std::make_shared<std::string>(str)) {}
        Capability(std::string_view str) : name(std::make_shared<std::string>(str)) {}
        Capability(std::string str) : name(std::make_shared<std::string>(std::move(str))) {}

        auto getName() const -> std::string_view {
            return *name;
        }

        auto toString() const -> const std::string& {
            return *name;
        }

        bool operator==(const Capability& other) const {
            return *name == *other.name;
        }

    private:
        // I made a mistake early on - now Capability should be lightweight
        // enough to be copied around liberally.
        std::shared_ptr<std::string> name;
    };
} // namespace trc::shader

template<>
struct std::hash<trc::shader::Capability>
{
    constexpr auto operator()(const trc::shader::Capability& capability) const -> size_t
    {
        return hash<std::string_view>{}(capability.getName());
    }
};
