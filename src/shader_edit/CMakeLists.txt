target_sources(torch PRIVATE
    Compiler.cpp
    CompileConfiguration.cpp
    ConfigParserJson.cpp
    Document.cpp
    Parser.cpp
    StringUtil.cpp
    VariableValue.cpp
)


# Compiler executable

find_package(nlohmann_json REQUIRED)

add_executable(compiler ${CMAKE_CURRENT_LIST_DIR}/shader_compiler.cpp)
target_include_directories(compiler PUBLIC ${CMAKE_CURRENT_LIST_DIR}/../../include/trc/shader_edit)
target_link_libraries(compiler PUBLIC torch nlohmann_json::nlohmann_json)
