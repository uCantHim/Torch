#pragma once

#include <expected>
#include <unordered_map>
#include <vector>

#include "ShaderResourceInterface.h"
#include "trc/Types.h"
#include "trc/VulkanInclude.h"

namespace trc::shader
{
    class ShaderModuleBuilder;
    class ShaderOutputInterface;

    /**
     * The result of successful shader stage input linking.
     */
    struct ShaderStageInputLinkResult
    {
        // Pairs [<old-loc>, <new-loc>] that declare corrected locations for
        // shader inputs. <old-loc> is the default location before linking,
        // while the corresponding <new-loc> is the corrected location at which
        // that input resides.
        std::unordered_map<vk::ShaderStageFlagBits, std::vector<std::pair<ui32, ui32>>> locationMap;
    };

    /**
     * Information about errors during shader stage input linking.
     */
    struct ShaderStageInputLinkError
    {
        // A list of shader inputs per shader stage that requested them. These
        // inputs remained unresolved after linking because no stage in the
        // pipeline was able to output them (e.g. not implemented).
        std::unordered_map<
            vk::ShaderStageFlagBits,
            std::vector<shader::ShaderResourceInterface::ShaderInputInfo>
        > unresolvedInputs;
    };

    /**
     * Input to the shader stage linker. The linker inserts output locations
     * into the shader module and output assignments into the output interface
     * as necessary.
     */
    struct ModuleLinkInfo
    {
        /**
         * The builder that is used to build the shader module.
         *
         * Used by the input linker algorithm to create output locations and
         * their corresponding values.
         *
         * Can be `nullptr` only for the last shader stage in pipeline order
         * (usually the fragment stage).
         */
        shader::ShaderModuleBuilder* builder;

        /**
         * The module's required resources.
         *
         * Used to determine which capabilities are requested as inputs from
         * earlier shader stages.
         *
         * This is usually just `builder.getResourceInterface()`. Can be
         * `nullptr` only for the first shader stage in pipeline order (usually
         * the vertex stage).
         */
        const shader::ShaderResourceInterface* resources;

        /**
         * The module's output interface.
         *
         * Used to create writes to output locations.
         *
         * Can be `nullptr` only for the last shader stage in pipeline order
         * (usually the fragment stage).
         */
        shader::ShaderOutputInterface* outputs;

        /**
         * The first input location that can be used to take data from previous
         * shader stages. Set this to the first input location that is not used
         * by the shader module.
         */
        ui32 nextInputLocation = 0;

        /**
         * The first output location that can be used to pass data to later
         * shader stages. Set this to the first output location that is not used
         * by the shader module.
         */
        ui32 nextOutputLocation = 0;
    };

    /**
     * Order shader stages of the rasterization pipeline with respect to each
     * other, insert shader stage outputs, and link them across shader modules
     * at their correct locations.
     *
     * @return A
     *
     * Detailed notes: We need to build the shader modules *in this function*
     * because we need two things to compute shader inputs/outputs:
     *
     *     1. The fully compiled shader resource interface, which declares the
     *     *inputs* that a shader module requires. These are requested from
     *     preceding modules in the pipeline.
     *
     *     2. Modifiable shader module code, such that writes to output
     *     locations can be inserted.
     *
     * These requirements can only be satisfied if the algorithm has knowledge
     * of all shader modules before *and* after compilation (except the last
     * one because it doesn't need to have its outputs modified).
     */
    auto linkShaderStageInputs(
        const std::unordered_map<vk::ShaderStageFlagBits, ModuleLinkInfo>& modules)
        -> std::expected<ShaderStageInputLinkResult, ShaderStageInputLinkError>;
} // namespace trc
