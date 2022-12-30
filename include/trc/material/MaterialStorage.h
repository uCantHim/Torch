#pragma once

#include <functional>
#include <unordered_map>
#include <vector>

#include "MaterialRuntime.h"
#include "MaterialShaderProgram.h"
#include "trc/FlagCombination.h"
#include "trc/Types.h"

namespace trc
{
    /**
     * The user-defined material information from which implementation-
     * specific specializations can be generated.
     */
    struct MaterialBaseInfo
    {
        ShaderModule fragmentModule;
        bool transparent;
    };

    /**
     * Information for *internal* specialization of materials that is not
     * exposed to the user, but performed automatically.
     */
    struct MaterialSpecializationInfo
    {
        bool animated;
    };

    /**
     * Can be created from MaterialSpecializationInfo or directly from its
     * flag combination type
     */
    struct MaterialKey
    {
        struct Flags
        {
            enum class Animated{ eFalse, eTrue, eMaxEnum };
        };

        using MaterialSpecializationFlags = FlagCombination<
            Flags::Animated
        >;

        struct Hash
        {
            constexpr auto operator()(const trc::MaterialKey& key) const -> size_t {
                return key.flags.toIndex();
            }
        };

        MaterialKey() = default;
        MaterialKey(MaterialSpecializationInfo info);
        MaterialKey(MaterialSpecializationFlags flags);

        bool operator==(const MaterialKey& rhs) const;

        MaterialSpecializationFlags flags;
    };

    /**
     * @brief Create a full shader program from basic material information
     */
    auto makeMaterialProgramSpecialization(ShaderModule fragmentModule,
                                           const MaterialKey& info)
        -> std::unordered_map<vk::ShaderStageFlagBits, ShaderModule>;

    class MaterialStorage
    {
    public:
        using MatID = ui32;

        explicit MaterialStorage(const ShaderDescriptorConfig& descriptorConfig);

        auto registerMaterial(MaterialBaseInfo info) -> MatID;
        void removeMaterial(MatID id);

        auto getBaseMaterial(MatID id) const -> const MaterialBaseInfo&;
        auto specialize(MatID id, MaterialKey key) -> MaterialRuntime;

    private:
        class MaterialSpecializer
        {
        public:
            MaterialSpecializer(MaterialSpecializer&&) = default;

            MaterialSpecializer(const MaterialStorage* storage, MaterialBaseInfo info);

            auto getBase() const -> const MaterialBaseInfo&;
            auto getOrMake(MaterialKey specialization) -> MaterialRuntime;

            /**
             * Free all material runtimes. Keep the create info in storage.
             */
            void clear();

        private:
            using Key = MaterialKey;
            using Hash = MaterialKey::Hash;

            const MaterialStorage* storage;
            MaterialBaseInfo baseMaterial;

            std::unordered_map<Key, u_ptr<MaterialShaderProgram>, Hash> specializations;
        };

        const ShaderDescriptorConfig descriptorConfig;

        std::vector<MaterialSpecializer> materialFactories;
    };
} // namespace trc
