#pragma once

#include <iostream>

#include <shaderc/shaderc.hpp>

#include "CompileConfiguration.h"

namespace shader_edit
{
    inline auto shaderKindFromExtension(const fs::path& path)
    {
        auto ext = path.extension();
        if (ext == ".vert")
            return shaderc_shader_kind::shaderc_glsl_default_vertex_shader;
        if (ext == ".frag")
            return shaderc_shader_kind::shaderc_glsl_default_fragment_shader;
        if (ext == ".geom")
            return shaderc_shader_kind::shaderc_glsl_default_geometry_shader;
        if (ext == ".tesc")
            return shaderc_shader_kind::shaderc_glsl_default_tess_control_shader;
        if (ext == ".tese")
            return shaderc_shader_kind::shaderc_glsl_default_tess_evaluation_shader;
        if (ext == ".task")
            return shaderc_shader_kind::shaderc_glsl_default_task_shader;
        if (ext == ".mesh")
            return shaderc_shader_kind::shaderc_glsl_default_mesh_shader;
        if (ext == ".rgen")
            return shaderc_shader_kind::shaderc_glsl_default_raygen_shader;
        if (ext == ".rchit")
            return shaderc_shader_kind::shaderc_glsl_default_closesthit_shader;
        if (ext == ".rahit")
            return shaderc_shader_kind::shaderc_glsl_default_anyhit_shader;
        if (ext == ".rmiss")
            return shaderc_shader_kind::shaderc_glsl_default_miss_shader;
        if (ext == ".rcall")
            return shaderc_shader_kind::shaderc_glsl_default_callable_shader;

        throw std::logic_error("Shader file extension \"" + ext.string() + "\" is invalid.");
    }

    inline auto generateSpirv(const CompiledShaderFile& shader)
    {
        shaderc::Compiler compiler;
        auto result = compiler.CompileGlslToSpv(
            shader.code,
            shaderKindFromExtension(shader.filePath),
            shader.filePath.c_str()
        );

        return result;
    }
} // namespace shader_edit