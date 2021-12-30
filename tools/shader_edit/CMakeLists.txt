# Dependencies
find_package(nlohmann_json REQUIRED)

if (NOT TARGET shaderc)
    FetchContent_Declare(
        shaderc
        GIT_REPOSITORY https://github.com/google/shaderc
        GIT_TAG        657c5ed2ba1714c0430895a274a94d6f2aeeab85
        PATCH_COMMAND  ${FETCHCONTENT_BASE_DIR}/shaderc-src/utils/git-sync-deps
    )
    option(SHADERC_SKIP_TESTS "" ON)
    option(SHADERC_SKIP_EXAMPLES "" ON)
    FetchContent_MakeAvailable(shaderc)
endif()

# Library
add_library(shader_compiler_lib STATIC)
torch_default_compile_options(shader_compiler_lib)

target_sources(shader_compiler_lib PRIVATE
    Compiler.cpp
    CompileConfiguration.cpp
    ConfigParserJson.cpp
    ShaderDocument.cpp
    ShaderDocumentParser.cpp
    StringUtil.cpp
    VariableValue.cpp
)
target_link_libraries(shader_compiler_lib PRIVATE
    torch_util
    shaderc
    nlohmann_json::nlohmann_json
)
target_include_directories(shader_compiler_lib INTERFACE ${CMAKE_CURRENT_LIST_DIR})
