#pragma once

#include <functional>
#include <generator>
#include <utility>

#include "trc/FlagCombination.h"
#include "trc/material/shader/ShaderProgram.h"

namespace trc
{
    /**
     * The user-defined material information from which implementation-
     * specific specializations can be generated.
     */
    struct MaterialBaseInfo
    {
        shader::ShaderModule fragmentModule;
        bool transparent;
    };

    /**
     * Information for *internal* specialization of materials that is not
     * exposed to the user, but performed automatically.
     */
    struct MaterialSpecializationInfo
    {
        bool animated;
        vk::PrimitiveTopology primitiveTopology{ vk::PrimitiveTopology::eTriangleList };
   };

    /**
     * Can be created from MaterialSpecializationInfo or directly from its
     * flag combination type.
     */
    struct MaterialKey
    {
        struct Flags
        {
            enum class Animated{ eFalse, eTrue, eMaxEnum };

            enum class PrimitiveTopology
            {
                ePoints,
                eLines,
                eTriangles,
                eMaxEnum,
            };
        };


        using MaterialSpecializationFlags = FlagCombination<
            Flags::Animated,
            Flags::PrimitiveTopology
            //, ...
        >;

        struct Hash
        {
            constexpr auto operator()(const trc::MaterialKey& key) const -> size_t {
                return key.toUniqueIndex();
            }
        };

        constexpr MaterialKey(const MaterialSpecializationInfo& info)
        {
            if (info.animated) flags |= Flags::Animated::eTrue;
            switch (info.primitiveTopology)
            {
            case vk::PrimitiveTopology::ePointList:
                flags |= Flags::PrimitiveTopology::ePoints;
                break;
            case vk::PrimitiveTopology::eLineList:
            case vk::PrimitiveTopology::eLineStrip:
            case vk::PrimitiveTopology::eLineListWithAdjacency:
            case vk::PrimitiveTopology::eLineStripWithAdjacency:
                flags |= Flags::PrimitiveTopology::eLines;
                break;
            case vk::PrimitiveTopology::eTriangleList:
            case vk::PrimitiveTopology::eTriangleFan:
            case vk::PrimitiveTopology::eTriangleStrip:
            case vk::PrimitiveTopology::eTriangleListWithAdjacency:
            case vk::PrimitiveTopology::eTriangleStripWithAdjacency:
                flags |= Flags::PrimitiveTopology::eTriangles;
                break;
            case vk::PrimitiveTopology::ePatchList:
                throw std::invalid_argument{ "Not implemented: Primitive topology 'patch list'" };
            }
        }

        explicit
        constexpr MaterialKey(const MaterialSpecializationFlags& flags) : flags(flags) {}

        constexpr bool operator==(const MaterialKey& rhs) const {
            return flags.toIndex() == rhs.flags.toIndex();
        }

        constexpr auto toUniqueIndex() const -> ui32 {
            return flags.toIndex();
        }

        constexpr auto toSpecializationInfo() const -> MaterialSpecializationInfo
        {
            return {
                .animated=flags & Flags::Animated::eTrue,
                .primitiveTopology=[this]{
                    switch (flags.get<Flags::PrimitiveTopology>())
                    {
                    case Flags::PrimitiveTopology::ePoints: return vk::PrimitiveTopology::ePointList;
                    case Flags::PrimitiveTopology::eLines: return vk::PrimitiveTopology::eLineList;
                    case Flags::PrimitiveTopology::eTriangles: return vk::PrimitiveTopology::eTriangleList;
                    case Flags::PrimitiveTopology::eMaxEnum: assert(false && "unreachable");
                    }
                    std::unreachable();
                }()
            };
        }

        static constexpr auto fromUniqueIndex(ui32 index) -> MaterialKey {
            return MaterialKey{ MaterialSpecializationFlags::fromIndex(index) };
        }

        static constexpr auto fromSpecializationInfo(const MaterialSpecializationInfo& info)
            -> MaterialKey
        {
            return MaterialKey{ info };
        }

        MaterialSpecializationFlags flags;
    };

    /**
     * Create a full shader program for Torch's render algorithm from a fragment
     * shader and additional specialization information.
     */
    auto makeDeferredMaterialSpecialization(const shader::ShaderModule& fragmentModule,
                                            const MaterialSpecializationInfo& info)
        -> shader::ShaderProgramData;

    /**
     * Create a full shader program for Torch's render algorithm from a material
     * description and additional specialization information.
     */
    auto makeDeferredMaterialSpecialization(const MaterialBaseInfo& baseInfo,
                                            const MaterialSpecializationInfo& info)
        -> shader::ShaderProgramData;

    /**
     * @brief Manages material specializations.
     *
     * A "material specialization" is a compiled, executable shader program with
     * a corresponding runtime. Specializations are instantiations of "material
     * base descriptions" for specific target parameters (e.g. a geometry type
     * or a render algorithm).
     *
     * A base material description consists of a fragment shader and some
     * additional configurations parameters. See the `MaterialBaseInfo` struct.
     *
     * When creating a specialization cache from a base material description,
     * it creates specializations lazily. Serializing the specialization cache
     * requires all specializations to be pre-computed.
     */
    struct MaterialSpecializationCache
    {
    public:
        MaterialSpecializationCache(const MaterialSpecializationCache&) = default;
        MaterialSpecializationCache(MaterialSpecializationCache&&) noexcept = default;
        MaterialSpecializationCache& operator=(const MaterialSpecializationCache&) = default;
        MaterialSpecializationCache& operator=(MaterialSpecializationCache&&) noexcept = default;
        ~MaterialSpecializationCache() noexcept = default;

        explicit MaterialSpecializationCache(const MaterialBaseInfo& base);

        /**
         * @return nullptr if the cache has no base info.
         */
        auto getBaseInfo() const -> const MaterialBaseInfo&;

        auto getSpecialization(const MaterialKey& key) -> const shader::ShaderProgramData&;

        /**
         * Force all specializations to be computed immediately.
         */
        void createAllSpecializations();

        /**
         * Will force-create all lazy specializations.
         */
        auto iterSpecializations()
            -> std::generator<std::pair<MaterialKey, const shader::ShaderProgramData&>>;

    private:
        static constexpr size_t kNumSpecializations
            = MaterialKey::MaterialSpecializationFlags::size();

        template<std::default_initializable T>
        using PerSpecialization = std::array<T, kNumSpecializations>;

        static auto createSpecialization(const MaterialBaseInfo& info, const MaterialKey& key)
            -> shader::ShaderProgramData;

        auto getOrCreateSpecialization(const MaterialKey& key)
            -> shader::ShaderProgramData&;

        MaterialBaseInfo base;
        PerSpecialization<std::optional<shader::ShaderProgramData>> shaderPrograms;
    };
} // namespace trc

template<>
struct std::hash<::trc::MaterialKey> : ::trc::MaterialKey::Hash {};
