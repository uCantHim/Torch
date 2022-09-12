# Dependencies
FetchContent_Declare(
    json
    GIT_REPOSITORY https://github.com/nlohmann/json
    GIT_TAG        master
)
FetchContent_MakeAvailable(json)

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
target_link_libraries(shader_compiler_lib
    PUBLIC
    torch_util
    nlohmann_json::nlohmann_json
)
target_include_directories(shader_compiler_lib INTERFACE ${CMAKE_CURRENT_LIST_DIR})
