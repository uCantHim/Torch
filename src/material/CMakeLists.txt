add_subdirectory(shader)

target_sources(
    torch
    PRIVATE
        FragmentShader.cpp
        MaterialProgram.cpp
        MaterialSpecialization.cpp
        ShaderCache.cpp
        ShaderStageInputLinker.cpp
        TorchMaterialSettings.cpp
        VertexShader.cpp
)
